// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/RedactionService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestRedactionService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMarkManagement();
    void testTargetAnalysis();
    void testImmutableSourcePreservationEnforced();
    void testUnsupportedContentDetection();
    void testApplyRedactionsAndSave();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestRedactionService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestRedactionService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestRedactionService::testMarkManagement() {
    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();
    QCOMPARE(service.GetRedactionMarks().size(), 0);

    pdf_api::RedactionMark m1;
    m1.id = "mark-1";
    m1.pageIndex = 0;
    m1.rect = {50.0, 700.0, 300.0, 30.0};
    m1.overlayText = "[CONFIDENTIAL]";

    pdf_api::RedactionMark m2;
    m2.id = "mark-2";
    m2.pageIndex = 1;
    m2.rect = {40.0, 600.0, 200.0, 50.0};

    service.AddRedactionMark(m1);
    service.AddRedactionMark(m2);

    QCOMPARE(service.GetRedactionMarks().size(), 2);
    QCOMPARE(service.GetRedactionMarksForPage(0).size(), 1);
    QCOMPARE(service.GetRedactionMarksForPage(1).size(), 1);
    QCOMPARE(service.GetRedactionMarksForPage(2).size(), 0);

    service.RemoveRedactionMark("mark-1");
    QCOMPARE(service.GetRedactionMarks().size(), 1);
    QCOMPARE(service.GetRedactionMarks()[0].id, std::string("mark-2"));

    service.ClearRedactionMarks();
    QCOMPARE(service.GetRedactionMarks().size(), 0);
}

void TestRedactionService::testTargetAnalysis() {
    auto pdfPath = m_fixturesDir / "redaction_adversarial.pdf";
    QVERIFY(std::filesystem::exists(pdfPath));

    auto openRes = m_engine->OpenDocument(pdfPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, pdfPath);

    // Target secret text on page 1 (TOP_SECRET_CODENAME_ALPHA99 at y=720)
    pdf_api::PdfRect targetRect{45.0, 710.0, 260.0, 25.0};
    auto analysisRes = doc.AnalyzeRedactionTarget(0, targetRect);
    QVERIFY(analysisRes.has_value());

    const auto& analysis = analysisRes.value;
    QCOMPARE(analysis.pageIndex, 0);
    QVERIFY(analysis.intersectingTextObjects >= 1);
    QVERIFY(!analysis.hasUnsupportedContent);

    // Verify text snippet was detected
    bool foundSnippet = false;
    for (const auto& s : analysis.detectedTextSnippets) {
        if (s.find("TOP_SECRET") != std::string::npos || s.find("ALPHA99") != std::string::npos) {
            foundSnippet = true;
            break;
        }
    }
    QVERIFY(foundSnippet);
}

void TestRedactionService::testImmutableSourcePreservationEnforced() {
    auto pdfPath = m_fixturesDir / "redaction_adversarial.pdf";
    auto openRes = m_engine->OpenDocument(pdfPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, pdfPath);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    pdf_api::RedactionMark m;
    m.id = "m-test";
    m.pageIndex = 0;
    m.rect = {50.0, 700.0, 200.0, 30.0};
    service.AddRedactionMark(m);

    // Attempting to overwrite source path in-place MUST be rejected!
    auto res = service.ApplyRedactionsAndSave(doc, pdfPath, pdfPath);
    QVERIFY(!res.has_value());
    QCOMPARE(res.error, pdf_api::EngineError::InvalidFormat);
}

void TestRedactionService::testUnsupportedContentDetection() {
    auto pdfPath = m_fixturesDir / "redaction_adversarial.pdf";
    auto openRes = m_engine->OpenDocument(pdfPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, pdfPath);

    // Page 2 has a nested Form XObject at y=600
    pdf_api::PdfRect formRect{45.0, 590.0, 150.0, 40.0};
    auto analysisRes = doc.AnalyzeRedactionTarget(1, formRect);
    QVERIFY(analysisRes.has_value());

    // VERIFICATION TEST: Unsupported content must be detected and disclosed!
    QVERIFY(analysisRes.value.hasUnsupportedContent);
    QVERIFY(!analysisRes.value.unsupportedWarnings.empty());
}

void TestRedactionService::testApplyRedactionsAndSave() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "redacted_out.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    pdf_api::RedactionMark mark;
    mark.id = "mark-secret";
    mark.pageIndex = 0;
    mark.rect = {40.0, 705.0, 280.0, 30.0}; // covers TOP_SECRET_CODENAME_ALPHA99
    mark.overlayText = "[REDACTED]";
    service.AddRedactionMark(mark);

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf);
    QVERIFY(repRes.has_value());
    QVERIFY(std::filesystem::exists(dstPdf));

    auto reopen = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopen.has_value());

    const auto& report = repRes.value;
    QCOMPARE(report.totalRedactionsApplied, 1);
    QCOMPARE(report.pagesModified, 1);
    QVERIFY(report.textObjectsRemoved >= 1);
    QVERIFY(!report.reportTimestamp.empty());
    QVERIFY(report.isSuccess);

    // Verify marks were cleared from registry after application
    QCOMPARE(service.GetRedactionMarks().size(), 0);
}

QTEST_MAIN(TestRedactionService)
#include "test_redaction_service.moc"
