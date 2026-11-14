// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QSignalSpy>
#include <filesystem>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "services/DocumentLifecycleService.hpp"
#include "services/DocumentSearchService.hpp"

using namespace pdfinity;

class TestPdfSearch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testExtractPageText();
    void testSearchPageText();
    void testCaseSensitiveSearch();
    void testWholeWordSearch();
    void testAsyncSearchService();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::unique_ptr<services::DocumentLifecycleService> m_lifecycle;
    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_fixturesDir;
};

void TestPdfSearch::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_lifecycle = std::make_unique<services::DocumentLifecycleService>(m_engine);

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    auto path = m_fixturesDir / "bookmarked_searchable.pdf";
    QVERIFY(std::filesystem::exists(path));

    auto res = m_lifecycle->OpenDocument(path);
    QVERIFY(res.has_value());
    m_doc = std::move(res.value);
    QVERIFY(m_doc->IsValid());
}

void TestPdfSearch::cleanupTestCase() {
    m_doc.reset();
    m_lifecycle.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestPdfSearch::testExtractPageText() {
    auto res1 = m_doc->ExtractPageText(0);
    QVERIFY(res1.has_value());
    QVERIFY(res1.value.find("PDFinity Architecture Overview") != std::string::npos);
    QVERIFY(res1.value.find("offline-first") != std::string::npos);

    auto res2 = m_doc->ExtractPageText(1);
    QVERIFY(res2.has_value());
    QVERIFY(res2.value.find("Systems Engineering") != std::string::npos);
}

void TestPdfSearch::testSearchPageText() {
    auto res = m_doc->SearchPageText(0, "PDFinity", false, false);
    QVERIFY(res.has_value());
    QVERIFY(!res.value.empty());
    // "PDFinity" appears twice on page 1
    QVERIFY(res.value.size() >= 2);

    for (const auto& match : res.value) {
        QCOMPARE(match.pageIndex, 0);
        QVERIFY(match.charCount > 0);
        QVERIFY(!match.rects.empty());
        for (const auto& r : match.rects) {
            QVERIFY(r.width > 0);
            QVERIFY(r.height > 0);
        }
    }
}

void TestPdfSearch::testCaseSensitiveSearch() {
    // Exact case: "pdfinity" (lowercase) vs "PDFinity" (uppercase)
    auto matchExact = m_doc->SearchPageText(0, "PDFinity", true, false);
    QVERIFY(matchExact.has_value());
    QVERIFY(!matchExact.value.empty());

    auto matchLower = m_doc->SearchPageText(0, "pdfinity", true, false);
    QVERIFY(matchLower.has_value());
    QVERIFY(matchLower.value.empty()); // Lowercase must find 0 matches when matchCase=true

    auto matchInsensitive = m_doc->SearchPageText(0, "pdfinity", false, false);
    QVERIFY(matchInsensitive.has_value());
    QVERIFY(!matchInsensitive.value.empty()); // Case-insensitive must find matches
}

void TestPdfSearch::testWholeWordSearch() {
    // "Arch" should match "Architecture" if wholeWord=false, but 0 matches if wholeWord=true
    auto partialMatch = m_doc->SearchPageText(0, "Arch", false, false);
    QVERIFY(partialMatch.has_value());
    QVERIFY(!partialMatch.value.empty());

    auto wholeMatch = m_doc->SearchPageText(0, "Arch", false, true);
    QVERIFY(wholeMatch.has_value());
    QVERIFY(wholeMatch.value.empty()); // Whole word rejects partial word match

    auto fullWord = m_doc->SearchPageText(0, "Architecture", false, true);
    QVERIFY(fullWord.has_value());
    QCOMPARE(fullWord.value.size(), 1ULL);
}

void TestPdfSearch::testAsyncSearchService() {
    services::DocumentSearchService searchService;
    QSignalSpy startSpy(&searchService, &services::DocumentSearchService::SearchStarted);
    QSignalSpy matchSpy(&searchService, &services::DocumentSearchService::MatchFound);
    QSignalSpy progressSpy(&searchService, &services::DocumentSearchService::SearchProgress);
    QSignalSpy completedSpy(&searchService, &services::DocumentSearchService::SearchCompleted);

    searchService.StartSearch(m_doc, "PDFinity", false, false);

    QVERIFY(completedSpy.wait(3000));
    QCOMPARE(startSpy.count(), 1);
    QVERIFY(matchSpy.count() >= 2);
    QVERIFY(progressSpy.count() >= 2);
    QCOMPARE(completedSpy.count(), 1);

    auto matches = qvariant_cast<std::vector<pdf_api::TextSearchResult>>(completedSpy.first().at(0));
    QVERIFY(matches.size() >= 2);
}

QTEST_MAIN(TestPdfSearch)
#include "test_pdf_search.moc"
#include <algorithm>
