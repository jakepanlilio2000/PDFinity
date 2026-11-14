// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <QImage>
#include <QPainter>
#include <filesystem>
#include <fstream>
#include "services/ConversionService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestConversionService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testConvertImagesToPdf();
    void testExportPdfToImages();
    void testExportPdfToText();
    void testExportPdfToHtmlWithFidelityDisclaimer();
    void testOfficeEngineDetection();
    void testOfficeConversionMissingEngineRejection();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestConversionService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestConversionService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestConversionService::testConvertImagesToPdf() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create 2 test PNG images
    std::vector<std::filesystem::path> imgPaths;
    for (int i = 1; i <= 2; ++i) {
        QImage img(200, 200, QImage::Format_ARGB32);
        img.fill((i == 1) ? Qt::blue : Qt::green);
        auto p = std::filesystem::path(tempDir.path().toStdString()) / ("test_img_" + std::to_string(i) + ".png");
        QVERIFY(img.save(QString::fromStdString(p.string()), "PNG"));
        imgPaths.push_back(p);
    }

    auto outPdf = std::filesystem::path(tempDir.path().toStdString()) / "from_images.pdf";
    pdf_api::ImageToPdfOptions opt;
    opt.pageFit = pdf_api::ImagePageFit::MatchImageSize;

    auto res = services::ConversionService::Instance().ConvertImagesToPdf(imgPaths, outPdf, opt, m_engine);
    QVERIFY(res.has_value());
    QVERIFY(std::filesystem::exists(outPdf));

    // Reopen and verify page count
    auto openRes = m_engine->OpenDocument(outPdf);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 2);
    m_engine->CloseDocument(openRes.value);
}

void TestConversionService::testExportPdfToImages() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    pdf_api::PdfToImageOptions opt;
    opt.format = pdf_api::ExportImageFormat::PNG;
    opt.dpi = 150;

    auto outDir = std::filesystem::path(tempDir.path().toStdString()) / "img_exports";
    auto expRes = services::ConversionService::Instance().ExportPdfToImages(doc, outDir, "ordinary_out", opt);
    QVERIFY(expRes.has_value());
    QCOMPARE(expRes.value.size(), 2);

    for (const auto& p : expRes.value) {
        QVERIFY(std::filesystem::exists(p));
        QVERIFY(std::filesystem::file_size(p) > 0);
        QImage readImg(QString::fromStdString(p.string()));
        QVERIFY(!readImg.isNull());
    }
}

void TestConversionService::testExportPdfToText() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto outTxt = std::filesystem::path(tempDir.path().toStdString()) / "ordinary.txt";
    auto res = services::ConversionService::Instance().ExportPdfToText(doc, outTxt);
    QVERIFY(res.has_value());
    QVERIFY(std::filesystem::exists(outTxt));

    std::ifstream in(outTxt);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(text.find("--- Page 1 ---") != std::string::npos);
    QVERIFY(text.find("--- Page 2 ---") != std::string::npos);
}

void TestConversionService::testExportPdfToHtmlWithFidelityDisclaimer() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto outHtml = std::filesystem::path(tempDir.path().toStdString()) / "ordinary.html";
    auto res = services::ConversionService::Instance().ExportPdfToHtml(doc, outHtml);
    QVERIFY(res.has_value());
    QVERIFY(std::filesystem::exists(outHtml));

    std::ifstream in(outHtml);
    std::string htmlContent((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // WORK ITEM 7: Never promise exact layout preservation for inherently lossy conversions!
    QVERIFY(htmlContent.find("fidelity-disclaimer") != std::string::npos);
    QVERIFY(htmlContent.find("Notice on Layout Fidelity") != std::string::npos);
    QVERIFY(htmlContent.find("pdf-page") != std::string::npos);
}

void TestConversionService::testOfficeEngineDetection() {
    auto status = services::ConversionService::Instance().DetectOfficeEngine();
    QVERIFY(!status.engineName.empty());
    if (!status.isAvailable) {
        QVERIFY(!status.installGuidance.empty());
        QVERIFY(status.installGuidance.find("libreoffice.org") != std::string::npos);
    }
}

void TestConversionService::testOfficeConversionMissingEngineRejection() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create a dummy .docx file
    auto docxPath = std::filesystem::path(tempDir.path().toStdString()) / "sample.docx";
    std::ofstream out(docxPath, std::ios::binary);
    out << "PK\x03\x04DummyDocxFileContent";
    out.close();

    auto outPdf = std::filesystem::path(tempDir.path().toStdString()) / "sample.pdf";

    auto status = services::ConversionService::Instance().DetectOfficeEngine();
    auto res = services::ConversionService::Instance().ConvertOfficeToPdf(docxPath, outPdf);

    if (!status.isAvailable) {
        // Verification criterion: Missing optional engines produce actionable messages, NOT fake success!
        QVERIFY(!res.has_value());
        QCOMPARE(res.error, pdf_api::EngineError::DependencyMissing);
        QVERIFY(!res.errorMessage.empty());
        QVERIFY(res.errorMessage.find("libreoffice.org") != std::string::npos);
    }
}

QTEST_MAIN(TestConversionService)
#include "test_conversion_service.moc"
