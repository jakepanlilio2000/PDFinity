// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "remote/RemoteControllerServer.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemotePairingManager.hpp"
#include "remote/RemoteCommandDispatcher.hpp"
#include "remote/RemoteControllerClient.hpp"
#include "app/RemoteCliHandler.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestRemoteServer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testServerLifecycle();
    void testNavigationCommands();
    void testZoomCommands();
    void testSearchCommand();
    void testThumbnailCommand();
    void testBatchCommands();
    void testToggleFullscreen();
    void testDisconnectCommand();
    void testAuditLogging();
    void testRemoteCliHandlerExecution();
    void testSocketBufferThreadSafety();

private:
    uint16_t m_testPort{18444};
};

void TestRemoteServer::initTestCase() {
    app::AppContext::Instance().Initialize();
}

void TestRemoteServer::cleanupTestCase() {
    remote::RemoteControllerServer::Instance().Stop();
    app::AppContext::Instance().Shutdown();
}

void TestRemoteServer::init() {
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
}

void TestRemoteServer::cleanup() {
    remote::RemoteControllerServer::Instance().Stop();
    remote::RemoteSecurityPolicy::Instance().SetEnabled(false);
}

void TestRemoteServer::testServerLifecycle() {
    auto& server = remote::RemoteControllerServer::Instance();
    QVERIFY(server.IsListening());
    QCOMPARE(server.GetBoundPort(), m_testPort);

    server.Stop();
    QVERIFY(!server.IsListening());

    QVERIFY(server.Start("127.0.0.1", m_testPort));
    QVERIFY(server.IsListening());
}

void TestRemoteServer::testNavigationCommands() {
    int targetPage = 1;
    remote::RemoteCommandDispatcher::ExecutionHooks hooks;
    hooks.gotoPage = [&targetPage](int p) { targetPage = p; return true; };
    hooks.nextPage = [&targetPage]() { targetPage++; return true; };
    hooks.prevPage = [&targetPage]() { targetPage--; return true; };
    hooks.firstPage = [&targetPage]() { targetPage = 1; return true; };
    hooks.lastPage = [&targetPage]() { targetPage = 10; return true; };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(hooks);

    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_nav").success);

    // Goto page
    auto resGoto = client.GotoPage(5);
    QVERIFY(resGoto.success);
    QCOMPARE(targetPage, 5);

    // Next page
    auto resNext = client.NextPage();
    QVERIFY(resNext.success);
    QCOMPARE(targetPage, 6);

    // Prev page
    auto resPrev = client.PrevPage();
    QVERIFY(resPrev.success);
    QCOMPARE(targetPage, 5);

    // First page
    auto resFirst = client.FirstPage();
    QVERIFY(resFirst.success);
    QCOMPARE(targetPage, 1);

    // Last page
    auto resLast = client.LastPage();
    QVERIFY(resLast.success);
    QCOMPARE(targetPage, 10);
}

void TestRemoteServer::testZoomCommands() {
    double currentZoom = 1.0;
    std::string currentMode;
    remote::RemoteCommandDispatcher::ExecutionHooks hooks;
    hooks.setZoom = [&currentZoom, &currentMode](double z, const std::string& m) {
        currentZoom = z;
        currentMode = m;
        return true;
    };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(hooks);

    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_zoom").success);

    auto resZoom = client.SetZoom(1.5);
    QVERIFY(resZoom.success);
    QCOMPARE(currentZoom, 1.5);

    auto resFit = client.SetZoom(1.0, "fit_width");
    QVERIFY(resFit.success);
    QCOMPARE(currentMode, std::string("fit_width"));
}

void TestRemoteServer::testSearchCommand() {
    remote::RemoteCommandDispatcher::ExecutionHooks hooks;
    hooks.search = [](const std::string& q, bool) {
        QJsonObject res;
        res["query"] = QString::fromStdString(q);
        res["matches"] = 3;
        return res;
    };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(hooks);

    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_search").success);

    auto resSearch = client.Search("invoice", false);
    QVERIFY(resSearch.success);
    QCOMPARE(resSearch.result["matches"].toInt(), 3);
}

void TestRemoteServer::testThumbnailCommand() {
    remote::RemoteCommandDispatcher::ExecutionHooks hooks;
    hooks.getThumbnailBase64 = [](int, int) {
        return "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";
    };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(hooks);

    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_thumb").success);

    auto resThumb = client.GetThumbnail(1, 200);
    QVERIFY(resThumb.success);
    QVERIFY(!resThumb.result["base64"].toString().isEmpty());
}

void TestRemoteServer::testBatchCommands() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_batch").success);

    auto resJob = client.GetJobStatus();
    QVERIFY(resJob.success);
    QVERIFY(resJob.result.contains("hasActiveJob"));

    auto resPause = client.PauseJob("job-123");
    QVERIFY(resPause.success);

    auto resResume = client.ResumeJob("job-123");
    QVERIFY(resResume.success);
}

void TestRemoteServer::testToggleFullscreen() {
    bool fsToggled = false;
    remote::RemoteCommandDispatcher::ExecutionHooks hooks;
    hooks.toggleFullscreen = [&fsToggled]() { fsToggled = true; return true; };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(hooks);

    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_fs").success);

    auto resFs = client.ToggleFullscreen();
    QVERIFY(resFs.success);
    QVERIFY(fsToggled);
}

void TestRemoteServer::testDisconnectCommand() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "Client", "dev_disc").success);

    auto resDisc = client.SendCommand("disconnect");
    QVERIFY(resDisc.success);

    // Wait for socket closure
    QTest::qWait(200);
    QVERIFY(!client.IsConnected());
}

void TestRemoteServer::testAuditLogging() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    remote::RemoteControllerClient client;
    QVERIFY(client.ConnectToHost("127.0.0.1", m_testPort, 3000));
    QVERIFY(client.Pair(pin, "AuditClient", "dev_audit").success);

    auto logs = remote::RemoteSecurityPolicy::Instance().GetAuditLogs();
    QVERIFY(!logs.empty());

    bool hasPairLog = false;
    for (const auto& l : logs) {
        if (l.command == "pair" && l.status == "ALLOWED") {
            hasPairLog = true;
            break;
        }
    }
    QVERIFY(hasPairLog);
}

void TestRemoteServer::testRemoteCliHandlerExecution() {
    // 1. IsRemoteCommand detection
    char arg0[] = "PDFinity";
    char argBatch[] = "batch";
    char argRemote[] = "remote-client";
    char* argv1[] = { arg0, argBatch };
    char* argv2[] = { arg0, argRemote };
    QVERIFY(!app::RemoteCliHandler::IsRemoteCommand(2, argv1));
    QVERIFY(app::RemoteCliHandler::IsRemoteCommand(2, argv2));

    // 2. Help execution returns 0
    char argHelp[] = "--help";
    char* argvHelp[] = { arg0, argRemote, argHelp };
    QCOMPARE(app::RemoteCliHandler::Execute(3, argvHelp), 0);

    // 3. Missing credentials returns 1
    char* argvMissing[] = { arg0, argRemote };
    QCOMPARE(app::RemoteCliHandler::Execute(2, argvMissing), 1);

    // 4. Pair and status command execution against running test server
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    std::string portStr = std::to_string(m_testPort);
    char argHostFlag[] = "--host";
    char argHostVal[] = "127.0.0.1";
    char argPortFlag[] = "--port";
    char argPinFlag[] = "--pin";
    char argCmdFlag[] = "--command";
    char argCmdVal[] = "status";
    char argJson[] = "--json";

    char* argvCli[] = {
        arg0, argRemote,
        argHostFlag, argHostVal,
        argPortFlag, portStr.data(),
        argPinFlag, pin.data(),
        argCmdFlag, argCmdVal,
        argJson
    };
    int ret = app::RemoteCliHandler::Execute(11, argvCli);
    QCOMPARE(ret, 0);
}

void TestRemoteServer::testSocketBufferThreadSafety() {
    auto& server = remote::RemoteControllerServer::Instance();
    QVERIFY(server.Start("127.0.0.1", m_testPort));

    QTcpSocket client;
    client.connectToHost("127.0.0.1", m_testPort);
    QVERIFY(client.waitForConnected(3000));

    server.DisconnectAll();
    server.Stop();
    QVERIFY(!server.IsListening());
}

QTEST_MAIN(TestRemoteServer)
#include "test_remote_server.moc"
