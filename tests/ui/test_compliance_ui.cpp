// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QApplication>
#include <filesystem>
#include "ui/DocumentIntelligenceDialog.hpp"
#include "ui/ComparisonWorkspaceDialog.hpp"
#include "ui/PreflightComplianceDialog.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"

using namespace pdfinity;

class TestComplianceUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDocumentIntelligenceDialogLifecycle();
    void testComparisonWorkspaceDialogLifecycle();
    void testPreflightComplianceDialogLifecycle();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestComplianceUi::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestComplianceUi::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestComplianceUi::testDocumentIntelligenceDialogLifecycle() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::DocumentIntelligenceDialog dlg(doc);
    QVERIFY(dlg.windowTitle().contains("Intelligence"));
}

void TestComplianceUi::testComparisonWorkspaceDialogLifecycle() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::ComparisonWorkspaceDialog dlg(doc, m_engine);
    QVERIFY(dlg.windowTitle().contains("Comparison Workspace"));
}

void TestComplianceUi::testPreflightComplianceDialogLifecycle() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::PreflightComplianceDialog dlg(doc);
    QVERIFY(dlg.windowTitle().contains("Compliance Studio"));
}

QTEST_MAIN(TestComplianceUi)
#include "test_compliance_ui.moc"
