// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QApplication>
#include <QTemporaryDir>
#include <filesystem>
#include "ui/OcrStudioDialog.hpp"
#include "ui/ConversionCenterDialog.hpp"
#include "ui/OptimizerDialog.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"

using namespace pdfinity;

class TestTransformationUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testOcrStudioDialogLifecycle();
    void testConversionCenterDialogTabsAndControls();
    void testOptimizerDialogPresetsAndAnalysis();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestTransformationUi::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestTransformationUi::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestTransformationUi::testOcrStudioDialogLifecycle() {
    auto srcPdf = m_fixturesDir / "scanned_page.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::OcrStudioDialog dlg(doc);
    QVERIFY(dlg.windowTitle().contains("OCR Studio"));

    auto opt = dlg.GetOptions();
    QCOMPARE(opt.language, std::string("eng"));
    QCOMPARE(opt.targetDpi, 300);
    QVERIFY(opt.preserveOriginalImage);

    auto dst = dlg.GetDestinationPath();
    QVERIFY(dst.string().find("_searchable.pdf") != std::string::npos);
}

void TestTransformationUi::testConversionCenterDialogTabsAndControls() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::ConversionCenterDialog dlg(doc, m_engine);
    QVERIFY(dlg.windowTitle().contains("Conversion Center"));
}

void TestTransformationUi::testOptimizerDialogPresetsAndAnalysis() {
    auto srcPdf = m_fixturesDir / "image_heavy.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, srcPdf);

    ui::OptimizerDialog dlg(doc);
    QVERIFY(dlg.windowTitle().contains("Optimizer"));

    auto opt = dlg.GetOptions();
    QCOMPARE(opt.targetImageDpi, 150);
    QCOMPARE(opt.jpegQuality, 80);
    QVERIFY(opt.stripMetadata);

    auto dst = dlg.GetDestinationPath();
    QVERIFY(dst.string().find("_optimized.pdf") != std::string::npos);
}

QTEST_MAIN(TestTransformationUi)
#include "test_transformation_ui.moc"
