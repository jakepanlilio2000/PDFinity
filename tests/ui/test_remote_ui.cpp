// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include "ui/RemoteConfigDialog.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemotePairingManager.hpp"
#include "remote/RemoteControllerServer.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestRemoteUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testDialogConstruction();
    void testToggleEnableStartsStopsServer();
    void testPinGenerationUI();
    void testCancelPairingUI();
    void testEmergencyDisableButton();

private:
    uint16_t m_testPort{18445};
};

void TestRemoteUi::initTestCase() {
    app::AppContext::Instance().Initialize();
}

void TestRemoteUi::cleanupTestCase() {
    remote::RemoteControllerServer::Instance().Stop();
    app::AppContext::Instance().Shutdown();
}

void TestRemoteUi::init() {
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    sec.SetEnabled(false);
    sec.SetBindAddress("127.0.0.1");
    sec.SetServerPort(m_testPort);

    auto& pairing = remote::RemotePairingManager::Instance();
    pairing.CancelPairing();
    pairing.ClearAllDevices();

    remote::RemoteControllerServer::Instance().Stop();
}

void TestRemoteUi::cleanup() {
    remote::RemoteControllerServer::Instance().Stop();
    remote::RemoteSecurityPolicy::Instance().SetEnabled(false);
}

void TestRemoteUi::testDialogConstruction() {
    ui::RemoteConfigDialog dlg;
    dlg.show();
    QVERIFY(dlg.isVisible());

    auto* enableCheck = dlg.findChild<QCheckBox*>();
    QVERIFY(enableCheck != nullptr);
    QCOMPARE(enableCheck->isChecked(), false);

    auto* startPairBtn = dlg.findChild<QPushButton*>();
    QVERIFY(startPairBtn != nullptr);

    auto tables = dlg.findChildren<QTableWidget*>();
    QVERIFY(tables.size() >= 2); // Paired devices & audit logs
}

void TestRemoteUi::testToggleEnableStartsStopsServer() {
    ui::RemoteConfigDialog dlg;
    auto* enableCheck = dlg.findChild<QCheckBox*>();
    QVERIFY(enableCheck != nullptr);

    // Toggle ON
    enableCheck->setChecked(true);
    QVERIFY(remote::RemoteSecurityPolicy::Instance().IsEnabled());
    QVERIFY(remote::RemoteControllerServer::Instance().IsListening());

    // Toggle OFF
    enableCheck->setChecked(false);
    QVERIFY(!remote::RemoteSecurityPolicy::Instance().IsEnabled());
    QVERIFY(!remote::RemoteControllerServer::Instance().IsListening());
}

void TestRemoteUi::testPinGenerationUI() {
    ui::RemoteConfigDialog dlg;
    auto* startPairBtn = dlg.findChild<QPushButton*>("startPairBtn");
    QVERIFY(startPairBtn != nullptr);

    // Simulate clicking start pairing
    startPairBtn->click();

    QVERIFY(remote::RemotePairingManager::Instance().IsPairingActive());
    std::string pin = remote::RemotePairingManager::Instance().GetActivePin();
    QCOMPARE(pin.length(), 6);
}

void TestRemoteUi::testCancelPairingUI() {
    ui::RemoteConfigDialog dlg;
    auto* startPairBtn = dlg.findChild<QPushButton*>("startPairBtn");
    QVERIFY(startPairBtn != nullptr);
    startPairBtn->click();
    QVERIFY(remote::RemotePairingManager::Instance().IsPairingActive());

    auto* cancelBtn = dlg.findChild<QPushButton*>("cancelPairBtn");
    QVERIFY(cancelBtn != nullptr);
    cancelBtn->click();

    QVERIFY(!remote::RemotePairingManager::Instance().IsPairingActive());
}

void TestRemoteUi::testEmergencyDisableButton() {
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    sec.SetEnabled(true);
    remote::RemoteControllerServer::Instance().Start("127.0.0.1", m_testPort);
    QVERIFY(remote::RemoteControllerServer::Instance().IsListening());

    // Call EmergencyDisable directly
    remote::RemoteControllerServer::Instance().EmergencyDisable();
    QVERIFY(!remote::RemoteControllerServer::Instance().IsListening());
    QVERIFY(!sec.IsEnabled());
}

QTEST_MAIN(TestRemoteUi)
#include "test_remote_ui.moc"
