// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <QImage>
#include <QPainter>
#include <filesystem>
#include "services/OcrService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestOcrService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEngineDetectionActionableGuidance();
    void testRecognizePageImageWithSyntheticFallback();
    void testApplySearchableOcrLayer();
    void testFullOcrDocumentWorkflow();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestOcrService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestOcrService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestOcrService::testEngineDetectionActionableGuidance() {
    auto status = services::OcrService::Instance().DetectOcrEngine();
    QVERIFY(!status.engineName.empty());

    // If Tesseract is not installed on the system, actionable install guidance must be provided!
    if (!status.isAvailable) {
        QVERIFY(!status.installGuidance.empty());
        QVERIFY(status.installGuidance.find("https://github.com/UB-Mannheim/tesseract/wiki") != std::string::npos ||
                status.installGuidance.find("apt install tesseract") != std::string::npos);
    } else {
        QVERIFY(!status.executablePath.empty());
    }
}

void TestOcrService::testRecognizePageImageWithSyntheticFallback() {
    // Create synthetic test image
    QImage img(400, 300, QImage::Format_ARGB32);
    img.fill(Qt::white);
    {
        QPainter p(&img);
        p.setPen(Qt::black);
        p.setFont(QFont("Arial", 16));
        p.drawText(50, 100, "Scanned Document OCR Processed");
    }

    pdf_api::OcrOptions opt;
    opt.language = "eng";
    opt.targetDpi = 150;

    auto res = services::OcrService::Instance().RecognizePageImage(img, opt, true);
    QVERIFY(res.has_value());

    const auto& pageResult = res.value;
    QVERIFY(!pageResult.words.empty());
    QVERIFY(pageResult.meanConfidence > 0.5f);
    QVERIFY(!pageResult.fullText.empty());
}

void TestOcrService::testApplySearchableOcrLayer() {
    auto srcPdf = m_fixturesDir / "scanned_page.pdf";
    QVERIFY(std::filesystem::exists(srcPdf));

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    pdf_api::OcrPageResult ocrRes;
    ocrRes.pageIndex = 0;

    pdf_api::OcrWordBox w1;
    w1.text = "SEARCHABLE_TOKEN_ALPHA";
    w1.rect = {50.0, 600.0, 200.0, 20.0};
    w1.confidence = 0.99f;
    ocrRes.words.push_back(w1);

    auto applyRes = services::OcrService::Instance().ApplySearchableOcrLayer(doc, 0, ocrRes);
    QVERIFY(applyRes.has_value());

    // Verify text can now be extracted!
    auto textRes = doc.ExtractPageText(0);
    QVERIFY(textRes.has_value());
    QVERIFY(textRes.value.find("SEARCHABLE_TOKEN_ALPHA") != std::string::npos);

    // Verify text can be searched!
    auto searchRes = doc.SearchPageText(0, "SEARCHABLE_TOKEN_ALPHA");
    QVERIFY(searchRes.has_value());
    QVERIFY(!searchRes.value.empty());
}

void TestOcrService::testFullOcrDocumentWorkflow() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "scanned_page.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "searchable_out.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    pdf_api::OcrOptions opt;
    opt.language = "eng";
    opt.targetDpi = 150;
    opt.preserveOriginalImage = true;

    auto procRes = services::OcrService::Instance().ProcessDocumentOcr(doc, dstPdf, opt);
    QVERIFY(procRes.has_value());
    QVERIFY(std::filesystem::exists(dstPdf));

    // Reopen generated searchable PDF and verify text extraction
    auto reopenRes = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel searchableDoc(reopenRes.value, m_engine, dstPdf);

    auto textRes = searchableDoc.ExtractPageText(0);
    QVERIFY(textRes.has_value());
    QVERIFY(!textRes.value.empty());
}

QTEST_MAIN(TestOcrService)
#include "test_ocr_service.moc"
