// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include "ui/McpConfigDialog.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "mcp/McpServer.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestMcpUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMcpConfigDialogConstruction();
    void testToggleEnableUpdatesPolicy();
    void testConsentPolicyChange();
    void testApprovedRootsListOperations();

private:
    std::filesystem::path m_fixturesDir;
};

void TestMcpUi::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
    app::AppContext::Instance().Initialize();
}

void TestMcpUi::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestMcpUi::testMcpConfigDialogConstruction() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(false);
    sec.SetConsentPolicy(mcp::McpConsentPolicy::AskForDestructive);
    sec.ClearApprovedRoots();
    sec.AddApprovedRoot(m_fixturesDir);

    ui::McpConfigDialog dlg;
    dlg.show();
    QVERIFY(dlg.isVisible());

    auto* enableCheck = dlg.findChild<QCheckBox*>();
    QVERIFY(enableCheck != nullptr);
    QCOMPARE(enableCheck->isChecked(), false);

    auto* rootsList = dlg.findChild<QListWidget*>();
    QVERIFY(rootsList != nullptr);
    QCOMPARE(rootsList->count(), 1);

    auto* auditTable = dlg.findChild<QTableWidget*>();
    QVERIFY(auditTable != nullptr);
    QCOMPARE(auditTable->columnCount(), 5);
}

void TestMcpUi::testToggleEnableUpdatesPolicy() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(false);

    ui::McpConfigDialog dlg;
    auto* enableCheck = dlg.findChild<QCheckBox*>();
    QVERIFY(enableCheck != nullptr);

    // Toggle ON
    enableCheck->setChecked(true);
    QCOMPARE(sec.IsEnabled(), true);

    // Toggle OFF
    enableCheck->setChecked(false);
    QCOMPARE(sec.IsEnabled(), false);
}

void TestMcpUi::testConsentPolicyChange() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetConsentPolicy(mcp::McpConsentPolicy::AskForDestructive);

    ui::McpConfigDialog dlg;
    auto* combo = dlg.findChild<QComboBox*>();
    QVERIFY(combo != nullptr);

    // Switch to TrustApprovedRootsOnly (index 1)
    combo->setCurrentIndex(1);
    QCOMPARE(sec.GetConsentPolicy(), mcp::McpConsentPolicy::TrustApprovedRootsOnly);

    // Switch to DenyAll (index 3)
    combo->setCurrentIndex(3);
    QCOMPARE(sec.GetConsentPolicy(), mcp::McpConsentPolicy::DenyAll);
}

void TestMcpUi::testApprovedRootsListOperations() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.ClearApprovedRoots();
    sec.AddApprovedRoot(m_fixturesDir);

    ui::McpConfigDialog dlg;
    auto* rootsList = dlg.findChild<QListWidget*>();
    QVERIFY(rootsList != nullptr);
    QCOMPARE(rootsList->count(), 1);

    // Add another root
    sec.AddApprovedRoot(std::filesystem::current_path());
    QCOMPARE(sec.GetApprovedRoots().size(), 2);
}

QTEST_MAIN(TestMcpUi)
#include "test_mcp_ui.moc"
