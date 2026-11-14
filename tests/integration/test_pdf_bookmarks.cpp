// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <filesystem>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "services/DocumentLifecycleService.hpp"

using namespace pdfinity;

class TestPdfBookmarks : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testExtractHierarchicalBookmarks();
    void testDocumentWithoutBookmarks();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::unique_ptr<services::DocumentLifecycleService> m_lifecycle;
    std::filesystem::path m_fixturesDir;
};

void TestPdfBookmarks::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_lifecycle = std::make_unique<services::DocumentLifecycleService>(m_engine);
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
}

void TestPdfBookmarks::cleanupTestCase() {
    m_lifecycle.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestPdfBookmarks::testExtractHierarchicalBookmarks() {
    auto path = m_fixturesDir / "bookmarked_searchable.pdf";
    auto res = m_lifecycle->OpenDocument(path);
    QVERIFY(res.has_value());
    auto doc = std::move(res.value);

    auto bmRes = doc->GetBookmarks();
    QVERIFY(bmRes.has_value());
    const auto& bookmarks = bmRes.value;

    // Verify 2 top-level bookmarks
    QCOMPARE(bookmarks.size(), 2ULL);

    // Bookmark 1: "Architecture Overview" -> Page 0
    QCOMPARE(QString::fromStdString(bookmarks[0].title), QString("Architecture Overview"));
    QCOMPARE(bookmarks[0].targetPageIndex, 0);
    QVERIFY(bookmarks[0].children.empty());

    // Bookmark 2: "Systems Engineering" -> Page 1
    QCOMPARE(QString::fromStdString(bookmarks[1].title), QString("Systems Engineering"));
    QCOMPARE(bookmarks[1].targetPageIndex, 1);
    QCOMPARE(bookmarks[1].children.size(), 1ULL);

    // Child Bookmark 2.1: "Performance & Bounded Caches" -> Page 1
    const auto& child = bookmarks[1].children[0];
    QCOMPARE(QString::fromStdString(child.title), QString("Performance & Bounded Caches"));
    QCOMPARE(child.targetPageIndex, 1);
}

void TestPdfBookmarks::testDocumentWithoutBookmarks() {
    auto path = m_fixturesDir / "ordinary.pdf";
    auto res = m_lifecycle->OpenDocument(path);
    QVERIFY(res.has_value());
    auto doc = std::move(res.value);

    auto bmRes = doc->GetBookmarks();
    QVERIFY(bmRes.has_value());
    QVERIFY(bmRes.value.empty()); // No bookmarks present
}

QTEST_MAIN(TestPdfBookmarks)
#include "test_pdf_bookmarks.moc"
