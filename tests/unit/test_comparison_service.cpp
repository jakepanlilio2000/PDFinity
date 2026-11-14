// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QImage>
#include <QPainter>
#include <filesystem>
#include "services/ComparisonService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestComparisonService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testVisualDiffIdenticalImages();
    void testVisualDiffModifiedImages();
    void testTextDiffLines();
    void testDocumentComparisonWorkflow();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestComparisonService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestComparisonService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestComparisonService::testVisualDiffIdenticalImages() {
    QImage img1(200, 200, QImage::Format_ARGB32);
    img1.fill(Qt::white);
    {
        QPainter p(&img1);
        p.setPen(Qt::blue);
        p.drawText(20, 50, "Sample Text");
    }

    QImage img2 = img1.copy();

    double diffPct = 0.0;
    int diffPix = 0;
    QImage diff = services::ComparisonService::Instance().GenerateVisualDiffImage(img1, img2, &diffPct, &diffPix);

    QVERIFY(!diff.isNull());
    QCOMPARE(diffPix, 0);
    QCOMPARE(diffPct, 0.0);
}

void TestComparisonService::testVisualDiffModifiedImages() {
    QImage img1(200, 200, QImage::Format_ARGB32);
    img1.fill(Qt::white);

    QImage img2 = img1.copy();
    {
        QPainter p(&img2);
        p.fillRect(50, 50, 50, 50, Qt::red);
    }

    double diffPct = 0.0;
    int diffPix = 0;
    QImage diff = services::ComparisonService::Instance().GenerateVisualDiffImage(img1, img2, &diffPct, &diffPix);

    QVERIFY(!diff.isNull());
    QVERIFY(diffPix >= 2500); // 50x50 block
    QVERIFY(diffPct > 0.0);
}

void TestComparisonService::testTextDiffLines() {
    std::string textA = "Header Line\nFirst paragraph text\nFooter 2025";
    std::string textB = "Header Line\nFirst paragraph modified text\nFooter 2026\nAdded Extra Line";

    auto diffs = services::ComparisonService::Instance().DiffPageTexts(0, textA, 0, textB);
    QVERIFY(!diffs.empty());

    bool hasUnchanged = false;
    bool hasModified = false;
    bool hasAdded = false;

    for (const auto& d : diffs) {
        if (d.type == pdf_api::DiffType::Unchanged) hasUnchanged = true;
        if (d.type == pdf_api::DiffType::Modified) hasModified = true;
        if (d.type == pdf_api::DiffType::Added) hasAdded = true;
    }

    QVERIFY(hasUnchanged);
    QVERIFY(hasModified);
    QVERIFY(hasAdded);
}

void TestComparisonService::testDocumentComparisonWorkflow() {
    auto srcA = m_fixturesDir / "ordinary.pdf";
    auto srcB = m_fixturesDir / "ordinary.pdf";

    auto openA = m_engine->OpenDocument(srcA);
    auto openB = m_engine->OpenDocument(srcB);
    QVERIFY(openA.has_value());
    QVERIFY(openB.has_value());

    domain::DocumentModel docA(openA.value, m_engine, srcA);
    domain::DocumentModel docB(openB.value, m_engine, srcB);

    auto res = services::ComparisonService::Instance().CompareDocuments(docA, docB, 72.0);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(report.isIdentical);
    QCOMPARE(report.overallVisualDifferencePercent, 0.0);
    QCOMPARE(report.pageResults.size(), 2);
}

QTEST_MAIN(TestComparisonService)
#include "test_comparison_service.moc"
