// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <QImage>
#include <QPainter>
#include <filesystem>
#include "services/ConversionService.hpp"
#include "services/OcrService.hpp"
#include "services/OptimizationService.hpp"
#include "services/TransformationJobQueue.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestTransformationPipeline : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEndToEndPipeline_Image_Ocr_Optimize_Export();
    void testBatchJobQueueTransformation();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestTransformationPipeline::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestTransformationPipeline::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestTransformationPipeline::testEndToEndPipeline_Image_Ocr_Optimize_Export() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // 1. Create a high-res synthetic test image with text
    QImage inputImg(600, 800, QImage::Format_ARGB32);
    inputImg.fill(Qt::white);
    {
        QPainter p(&inputImg);
        p.setPen(Qt::darkBlue);
        p.setFont(QFont("Arial", 24));
        p.drawText(80, 150, "Contract Agreement 2026");
        p.setFont(QFont("Arial", 16));
        p.drawText(80, 220, "Confidential Pipeline Test");
    }

    auto imgPath = std::filesystem::path(tempDir.path().toStdString()) / "contract.png";
    QVERIFY(inputImg.save(QString::fromStdString(imgPath.string()), "PNG"));

    // 2. Convert Image to PDF
    auto step1Pdf = std::filesystem::path(tempDir.path().toStdString()) / "step1_image.pdf";
    pdf_api::ImageToPdfOptions imgOpt;
    imgOpt.pageFit = pdf_api::ImagePageFit::FitA4;
    auto cRes = services::ConversionService::Instance().ConvertImagesToPdf({imgPath}, step1Pdf, imgOpt, m_engine);
    QVERIFY(cRes.has_value());
    QVERIFY(std::filesystem::exists(step1Pdf));

    // 3. Run OCR to inject searchable text layer
    auto step2Pdf = std::filesystem::path(tempDir.path().toStdString()) / "step2_searchable.pdf";
    auto openRes = m_engine->OpenDocument(step1Pdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc1(openRes.value, m_engine, step1Pdf);

    pdf_api::OcrOptions ocrOpt;
    ocrOpt.targetDpi = 150;
    ocrOpt.preserveOriginalImage = true;

    auto ocrRes = services::OcrService::Instance().ProcessDocumentOcr(doc1, step2Pdf, ocrOpt);
    QVERIFY(ocrRes.has_value());
    QVERIFY(std::filesystem::exists(step2Pdf));

    // 4. Verify text is now searchable
    auto openStep2 = m_engine->OpenDocument(step2Pdf);
    QVERIFY(openStep2.has_value());
    domain::DocumentModel doc2(openStep2.value, m_engine, step2Pdf);

    auto textRes = doc2.ExtractPageText(0);
    QVERIFY(textRes.has_value());
    QVERIFY(!textRes.value.empty());

    // 5. Optimize & Compress PDF
    auto step3Pdf = std::filesystem::path(tempDir.path().toStdString()) / "step3_optimized.pdf";
    auto optOptions = services::OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset::RecommendedStandard);
    auto optRes = services::OptimizationService::Instance().OptimizePdf(doc2, step2Pdf, step3Pdf, optOptions);
    QVERIFY(optRes.has_value());
    QVERIFY(std::filesystem::exists(step3Pdf));

    // 6. Verify optimized PDF remains 100% readable and retains searchable text!
    auto openStep3 = m_engine->OpenDocument(step3Pdf);
    QVERIFY(openStep3.has_value());
    domain::DocumentModel doc3(openStep3.value, m_engine, step3Pdf);

    auto textAfterOpt = doc3.ExtractPageText(0);
    QVERIFY(textAfterOpt.has_value());
    QVERIFY(!textAfterOpt.value.empty());

    // 7. Export back to PNG images and HTML
    auto outImgDir = std::filesystem::path(tempDir.path().toStdString()) / "final_images";
    pdf_api::PdfToImageOptions expOpt;
    expOpt.format = pdf_api::ExportImageFormat::PNG;
    auto expRes = services::ConversionService::Instance().ExportPdfToImages(doc3, outImgDir, "final", expOpt);
    QVERIFY(expRes.has_value());
    QCOMPARE(expRes.value.size(), 1);
    QVERIFY(std::filesystem::exists(expRes.value[0]));

    auto outHtml = std::filesystem::path(tempDir.path().toStdString()) / "final.html";
    auto htmlRes = services::ConversionService::Instance().ExportPdfToHtml(doc3, outHtml);
    QVERIFY(htmlRes.has_value());
    QVERIFY(std::filesystem::exists(outHtml));
}

void TestTransformationPipeline::testBatchJobQueueTransformation() {
    auto& queue = services::TransformationJobQueue::Instance();
    queue.ClearCompletedJobs();

    int executedCounter = 0;
    auto jobId = queue.EnqueueJob("Batch OCR and Optimization", [&](std::atomic<bool>& cancel, auto progressCb) {
        for (int i = 1; i <= 3; ++i) {
            if (cancel.load()) {
                return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Cancelled");
            }
            progressCb(i, 3, "Batch step " + std::to_string(i));
            executedCounter++;
        }
        return pdf_api::EngineResult<void>::Ok();
    });

    QVERIFY(!jobId.empty());
    QCOMPARE(queue.GetJobStatus(jobId), pdf_api::JobStatus::Queued);

    auto runRes = queue.RunJobSynchronously(jobId);
    QVERIFY(runRes.has_value());
    QCOMPARE(executedCounter, 3);
    QCOMPARE(queue.GetJobStatus(jobId), pdf_api::JobStatus::Completed);
}

QTEST_MAIN(TestTransformationPipeline)
#include "test_transformation_pipeline.moc"
