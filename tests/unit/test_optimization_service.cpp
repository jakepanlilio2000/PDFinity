// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include "services/OptimizationService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestOptimizationService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPresetValues();
    void testOptimizationDownsamplingAndCompression();
    void testOutputValidationIntegrity();
    void testImmutableSourcePreservationEnforced();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestOptimizationService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestOptimizationService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestOptimizationService::testPresetValues() {
    auto p1 = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::PrepressLossless);
    QCOMPARE(p1.targetImageDpi, 300);
    QCOMPARE(p1.jpegQuality, 95);
    QVERIFY(!p1.stripMetadata);

    auto p2 = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::RecommendedStandard);
    QCOMPARE(p2.targetImageDpi, 150);
    QCOMPARE(p2.jpegQuality, 80);
    QVERIFY(p2.stripMetadata);

    auto p3 = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::MaximumScreenWeb);
    QCOMPARE(p3.targetImageDpi, 96);
    QCOMPARE(p3.jpegQuality, 65);
    QVERIFY(p3.stripMetadata);
    QVERIFY(p3.removeAnnotations);
}

void TestOptimizationService::testOptimizationDownsamplingAndCompression() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "image_heavy.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "optimized_out.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto opt = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::RecommendedStandard);
    auto res = services::OptimizationService::Instance().OptimizePdf(doc, srcPdf, dstPdf, opt);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(report.isSuccess);
    QVERIFY(report.originalSizeBytes > 0);
    QVERIFY(report.optimizedSizeBytes > 0);
    QVERIFY(std::filesystem::exists(dstPdf));

    // Reopen and verify document remains 100% readable!
    auto reopenRes = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopenRes.has_value());
    QCOMPARE(m_engine->GetPageCount(reopenRes.value), doc.GetPageCount());
    m_engine->CloseDocument(reopenRes.value);
}

void TestOptimizationService::testOutputValidationIntegrity() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto nonExistent = std::filesystem::path(tempDir.path().toStdString()) / "does_not_exist.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "opt.pdf";

    auto openRes = m_engine->OpenDocument(m_fixturesDir / "ordinary.pdf");
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, m_fixturesDir / "ordinary.pdf");

    auto opt = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::RecommendedStandard);
    auto res = services::OptimizationService::Instance().OptimizePdf(doc, nonExistent, dstPdf, opt);
    QVERIFY(!res.has_value());
    QCOMPARE(res.error, pdf_api::EngineError::FileNotFound);
}

void TestOptimizationService::testImmutableSourcePreservationEnforced() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto opt = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::RecommendedStandard);
    // Overwriting source in-place MUST be rejected!
    auto res = services::OptimizationService::Instance().OptimizePdf(doc, srcPdf, srcPdf, opt);
    QVERIFY(!res.has_value());
    QCOMPARE(res.error, pdf_api::EngineError::InvalidFormat);
}

QTEST_MAIN(TestOptimizationService)
#include "test_optimization_service.moc"
