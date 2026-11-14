// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <filesystem>
#include "application/AppContext.hpp"
#include "ui/PdfViewerWidget.hpp"

using namespace pdfinity;
using namespace pdfinity::ui;

class TestViewerStress : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testLargeDocumentLoading();
    void testRapidPageNavigation();
    void testLayoutModeTransitions();
    void testZoomAndFitModes();
    void testSearchInViewer();
    void testMemoryCacheBounds();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<domain::DocumentModel> m_doc;
};

void TestViewerStress::initTestCase() {
    QVERIFY(app::AppContext::Instance().Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";

    auto path = m_fixturesDir / "large_document_20p.pdf";
    QVERIFY(std::filesystem::exists(path));

    auto res = app::AppContext::Instance().GetLifecycleService().OpenDocument(path);
    QVERIFY(res.has_value());
    m_doc = std::move(res.value);
    QVERIFY(m_doc->IsValid());
    QCOMPARE(m_doc->GetPageCount(), 20);
}

void TestViewerStress::cleanupTestCase() {
    m_doc.reset();
    app::AppContext::Instance().Shutdown();
}

void TestViewerStress::testLargeDocumentLoading() {
    PdfViewerWidget viewer(m_doc);
    QCOMPARE(viewer.GetCurrentPage(), 0);
    QCOMPARE(viewer.GetLayoutMode(), pdf_api::PageLayoutMode::Continuous);
    QVERIFY(viewer.GetZoomScale() > 0.0);
}

void TestViewerStress::testRapidPageNavigation() {
    PdfViewerWidget viewer(m_doc);

    // Rapidly navigate through pages
    for (int p : {5, 10, 15, 19, 0, 8, 12, 19}) {
        viewer.GoToPage(p);
        QCOMPARE(viewer.GetCurrentPage(), p);
    }
}

void TestViewerStress::testLayoutModeTransitions() {
    PdfViewerWidget viewer(m_doc);

    viewer.SetLayout(pdf_api::PageLayoutMode::SinglePage);
    QCOMPARE(viewer.GetLayoutMode(), pdf_api::PageLayoutMode::SinglePage);

    viewer.SetLayout(pdf_api::PageLayoutMode::FacingPages);
    QCOMPARE(viewer.GetLayoutMode(), pdf_api::PageLayoutMode::FacingPages);

    viewer.SetLayout(pdf_api::PageLayoutMode::Continuous);
    QCOMPARE(viewer.GetLayoutMode(), pdf_api::PageLayoutMode::Continuous);
}

void TestViewerStress::testZoomAndFitModes() {
    PdfViewerWidget viewer(m_doc);

    double initial = viewer.GetZoomScale();
    viewer.ZoomIn();
    QVERIFY(viewer.GetZoomScale() > initial);

    viewer.ZoomOut();
    viewer.ZoomOut();
    QVERIFY(viewer.GetZoomScale() < initial);

    viewer.SetZoom(1.0);
    QCOMPARE(viewer.GetZoomScale(), 1.0);

    viewer.ZoomFitWidth();
    QVERIFY(viewer.GetZoomScale() > 0.0);

    viewer.ZoomFitPage();
    QVERIFY(viewer.GetZoomScale() > 0.0);
}

void TestViewerStress::testSearchInViewer() {
    PdfViewerWidget viewer(m_doc);
    viewer.ToggleFind();

    // Verify search can be executed on the 20-page document
    auto res = m_doc->SearchPageText(0, "Stress", false, false);
    QVERIFY(res.has_value());
    QVERIFY(!res.value.empty());
}

void TestViewerStress::testMemoryCacheBounds() {
    services::PageCache cache(5 * 1024 * 1024); // 5 MB bounded cache

    // Simulate rendering all 20 pages
    for (int i = 0; i < 20; ++i) {
        pdf_api::RenderOptions opts;
        opts.scale = 0.5;
        auto res = m_doc->RenderPage(i, opts);
        if (res.has_value() && !res.value.pixels.empty()) {
            const auto& raw = res.value;
            QImage img(raw.pixels.data(), raw.width, raw.height, raw.stride, QImage::Format_ARGB32_Premultiplied);
            cache.Put(i, 0.5, 1.0, img.copy());
        }
    }

    // Verify cache bounded size is strictly respected
    QVERIFY(cache.CurrentSizeBytes() <= cache.MaxSizeBytes());
    QVERIFY(cache.ItemCount() > 0);
}

QTEST_MAIN(TestViewerStress)
#include "test_viewer_stress.moc"
