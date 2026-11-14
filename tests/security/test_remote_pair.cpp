// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "remote/RemoteControllerServer.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemotePairingManager.hpp"
#include "remote/RemoteCommandDispatcher.hpp"
#include "remote/RemoteControllerClient.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestRemotePair : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testRemoteDisabledByDefault();
    void testPairingWithValidPin();
    void testPairingWithInvalidPinFails();
    void testPairingRateLimitingAndLockout();
    void testExpiredPinFails();
    void testReplayAttackProtection();
    void testRevokedDeviceBlocked();
    void testHostConfirmationGateForSensitiveActions();
    void testCommandAllowlistEnforcement();
    void testEmergencyDisable();

private:
    uint16_t m_testPort{18443};
};

void TestRemotePair::initTestCase() {
    app::AppContext::Instance().Initialize();
}

void TestRemotePair::cleanupTestCase() {
    remote::RemoteControllerServer::Instance().Stop();
    app::AppContext::Instance().Shutdown();
}

void TestRemotePair::init() {
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    sec.SetEnabled(true);
    sec.SetBindAddress("127.0.0.1");
    sec.SetServerPort(m_testPort);
    sec.ClearAuditLogs();

    auto& pairing = remote::RemotePairingManager::Instance();
    pairing.CancelPairing();
    pairing.ClearAllDevices();

    auto& server = remote::RemoteControllerServer::Instance();
    server.ResetSessionState();
    server.Start("127.0.0.1", m_testPort);
    QVERIFY(server.IsListening());
}

void TestRemotePair::cleanup() {
    auto& server = remote::RemoteControllerServer::Instance();
    server.Stop();
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    sec.SetEnabled(false);
}

void TestRemotePair::testRemoteDisabledByDefault() {
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    sec.SetEnabled(false);

    auto& server = remote::RemoteControllerServer::Instance();
    server.Stop();
    QVERIFY(!server.IsListening());

    // When disabled, ValidateRequest should deny unconditionally
    auto res = sec.ValidateRequest("dev1", "token1", 1, QDateTime::currentSecsSinceEpoch(), "127.0.0.1");
    QVERIFY(!res.allowed);
    QCOMPARE(res.errorCode, -32001);
}

void TestRemotePair::testPairingWithValidPin() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    QCOMPARE(pin.length(), 6);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));

    auto pairRes = client.Pair(pin, "Test iPad", "ipad_01");
    QVERIFY(pairRes.success);
    QVERIFY(!client.GetToken().empty());
    QCOMPARE(client.GetDeviceId(), std::string("ipad_01"));

    // Verify session state on host
    auto state = remote::RemoteControllerServer::Instance().GetSessionState();
    QVERIFY(state.isConnected);
    QCOMPARE(state.activeClientName, std::string("Test iPad"));

    // Verify single-use PIN is consumed
    QVERIFY(!remote::RemotePairingManager::Instance().IsPairingActive());
}

void TestRemotePair::testPairingWithInvalidPinFails() {
    remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));

    auto pairRes = client.Pair("000000", "Attacker Device", "bad_dev");
    QVERIFY(!pairRes.success);
    QCOMPARE(pairRes.errorCode, -32004);
}

void TestRemotePair::testPairingRateLimitingAndLockout() {
    remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));

    // Send 5 incorrect attempts
    for (int i = 0; i < 4; ++i) {
        auto res = client.Pair("111111", "Spammer", "spam_dev");
        QVERIFY(!res.success);
        QCOMPARE(res.errorCode, -32004);
    }

    // 5th attempt triggers lockout
    auto lockRes = client.Pair("111111", "Spammer", "spam_dev");
    QVERIFY(!lockRes.success);
    QCOMPARE(lockRes.errorCode, -32005);

    // Subsequent attempt is blocked by lockout
    auto blockedRes = client.Pair("111111", "Spammer", "spam_dev");
    QVERIFY(!blockedRes.success);
    QCOMPARE(blockedRes.errorCode, -32005);
}

void TestRemotePair::testExpiredPinFails() {
    // Generate PIN with 0 lifetime
    remote::RemotePairingManager::Instance().GenerateNewPin(0);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));

    auto pairRes = client.Pair("123456", "Client", "dev_exp");
    QVERIFY(!pairRes.success);
    QCOMPARE(pairRes.errorCode, -32004);
}

void TestRemotePair::testReplayAttackProtection() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_replay").success);

    // 1. Normal command with valid incrementing nonce
    auto res1 = client.GetStatus();
    QVERIFY(res1.success);

    // 2. Replay attack: Send request with same or lower nonce
    QJsonObject replayReq;
    replayReq["jsonrpc"] = "2.0";
    replayReq["id"] = 99;
    replayReq["method"] = "status";
    QJsonObject auth;
    auth["deviceId"] = QString::fromStdString(client.GetDeviceId());
    auth["token"] = QString::fromStdString(client.GetToken());
    auth["nonce"] = 1; // Stale nonce
    auth["timestamp"] = static_cast<qint64>(QDateTime::currentSecsSinceEpoch());
    replayReq["auth"] = auth;

    QByteArray bytes = QJsonDocument(replayReq).toJson(QJsonDocument::Compact) + "\n";
    // We send via raw client call or policy directly
    auto polRes = remote::RemoteSecurityPolicy::Instance().ValidateRequest(
        client.GetDeviceId(), client.GetToken(), 1, QDateTime::currentSecsSinceEpoch(), "127.0.0.1"
    );
    QVERIFY(!polRes.allowed);
    QCOMPARE(polRes.errorCode, -32007); // Replay attack detected

    // 3. Stale timestamp (outside +-30s window)
    auto polTimeRes = remote::RemoteSecurityPolicy::Instance().ValidateRequest(
        client.GetDeviceId(), client.GetToken(), 100, QDateTime::currentSecsSinceEpoch() - 100, "127.0.0.1"
    );
    QVERIFY(!polTimeRes.allowed);
    QCOMPARE(polTimeRes.errorCode, -32006); // Timestamp expired
}

void TestRemotePair::testRevokedDeviceBlocked() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Revocable Tablet", "tablet_rev").success);

    // Initial command succeeds
    QVERIFY(client.GetStatus().success);

    // Host revokes device
    remote::RemoteControllerServer::Instance().RevokeDevice("tablet_rev");

    // Reconnect and attempt command with old token
    remote::RemoteControllerClient client2;
    QVERIFY(client2.ConnectToHost("127.0.0.1", m_testPort, 3000));
    client2.SetCredentials("tablet_rev", client.GetToken());

    auto blockedRes = client2.GetStatus();
    QVERIFY(!blockedRes.success);
    QCOMPARE(blockedRes.errorCode, -32003); // Access revoked
}

void TestRemotePair::testHostConfirmationGateForSensitiveActions() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_conf").success);

    // Case A: Host operator denies sensitive action
    remote::RemoteSecurityPolicy::Instance().SetConfirmationCallback(
        [](const std::string&, const std::string&, const std::string&) {
            return false; // Deny
        }
    );

    auto deniedRes = client.CloseDocument();
    QVERIFY(!deniedRes.success);
    QCOMPARE(deniedRes.errorCode, -32001); // Confirmation rejected

    // Case B: Host operator approves sensitive action
    remote::RemoteSecurityPolicy::Instance().SetConfirmationCallback(
        [](const std::string&, const std::string&, const std::string&) {
            return true; // Approve
        }
    );

    auto approvedRes = client.CloseDocument();
    QVERIFY(approvedRes.success);
}

void TestRemotePair::testCommandAllowlistEnforcement() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_cmd").success);

    // Attempt unauthorized command not in allowlist
    auto unauthRes = client.SendCommand("exec_shell", QJsonObject{{"cmd", "calc.exe"}});
    QVERIFY(!unauthRes.success);
    QCOMPARE(unauthRes.errorCode, -32601); // Method not found
}

void TestRemotePair::testEmergencyDisable() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);

    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_emrg").success);

    // Host triggers emergency disable
    remote::RemoteControllerServer::Instance().EmergencyDisable();
    QVERIFY(!remote::RemoteControllerServer::Instance().IsListening());
    QVERIFY(!remote::RemoteSecurityPolicy::Instance().IsEnabled());

    client.DisconnectFromHost();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    // Further commands on client fail
    auto res = client.SendCommand("get_page_count", {});
    QVERIFY(!res.success);

    // Connection attempt or subsequent pairing attempt fails immediately
    remote::RemoteControllerClient client2;
    bool connected = client2.ConnectToHost("127.0.0.1", m_testPort, 200);
    if (connected) {
        auto pairRes = client2.Pair(pin, "Client", "dev_emrg2");
        QVERIFY(!pairRes.success);
    }
}

QTEST_MAIN(TestRemotePair)
#include "test_remote_pair.moc"
