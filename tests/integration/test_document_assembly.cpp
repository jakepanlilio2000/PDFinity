// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "services/DocumentAssemblyService.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>

using namespace pdfinity;

class TestDocumentAssembly : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMergeDocuments();
    void testMergeWithRanges();
    void testOriginalsUnchangedAfterMerge();
    void testSplitIndividualPages();
    void testSplitEveryNPages();
    void testSplitByCustomRanges();
    void testExtractPages();
    void testReorderDocument();
    void testInDocumentPageOperations();
    void testInvalidOperationsFailSafely();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::unique_ptr<services::DocumentAssemblyService> m_service;
    std::filesystem::path m_fixturesDir;
};

void TestDocumentAssembly::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_service = std::make_unique<services::DocumentAssemblyService>(m_engine);

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestDocumentAssembly::cleanupTestCase() {
    m_service.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestDocumentAssembly::testMergeDocuments() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "merged.pdf";

    auto doc1 = m_fixturesDir / "ordinary.pdf";         // 2 pages
    auto doc2 = m_fixturesDir / "unicode_metadata.pdf"; // 1 page

    std::vector<services::MergeSource> sources = {
        {doc1, "all", {}},
        {doc2, "all", {}}
    };

    auto res = m_service->MergeDocuments(sources, outPath);
    QVERIFY(res.has_value());
    QCOMPARE(res.value.totalPagesMerged, 3);
    QVERIFY(std::filesystem::exists(outPath));
    QVERIFY(std::filesystem::file_size(outPath) > 0);

    // Verify output document opens and renders
    auto openRes = m_engine->OpenDocument(outPath);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 3);

    pdf_api::RenderOptions opts;
    opts.scale = 0.5;
    auto renderRes = m_engine->RenderPage(openRes.value, 0, opts);
    QVERIFY(renderRes.has_value());
    QVERIFY(!renderRes.value.pixels.empty());

    m_engine->CloseDocument(openRes.value);
}

void TestDocumentAssembly::testMergeWithRanges() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "merged_range.pdf";

    auto doc1 = m_fixturesDir / "ordinary.pdf";         // 2 pages
    auto doc2 = m_fixturesDir / "unicode_metadata.pdf"; // 1 page

    // Only take page 2 of doc1 (1-based "2") and page 1 of doc2 (1-based "1")
    std::vector<services::MergeSource> sources = {
        {doc1, "2", {}},
        {doc2, "1", {}}
    };

    auto res = m_service->MergeDocuments(sources, outPath);
    QVERIFY(res.has_value());
    QCOMPARE(res.value.totalPagesMerged, 2);

    auto openRes = m_engine->OpenDocument(outPath);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 2);
    m_engine->CloseDocument(openRes.value);
}

void TestDocumentAssembly::testOriginalsUnchangedAfterMerge() {
    auto doc1 = m_fixturesDir / "ordinary.pdf";
    uint64_t initialSize = std::filesystem::file_size(doc1);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "test_immutability.pdf";

    std::vector<services::MergeSource> sources = {
        {doc1, "1", {}},
        {doc1, "2", {}}
    };

    auto res = m_service->MergeDocuments(sources, outPath);
    QVERIFY(res.has_value());

    // Verify original file size has not changed at all
    uint64_t afterSize = std::filesystem::file_size(doc1);
    QCOMPARE(initialSize, afterSize);
}

void TestDocumentAssembly::testSplitIndividualPages() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages

    services::SplitOptions options;
    options.mode = services::SplitMode::IndividualPages;
    options.filePrefix = "page_";

    auto res = m_service->SplitDocument(doc, outDir, options);
    QVERIFY(res.has_value());
    QCOMPARE(static_cast<int>(res.value.generatedFiles.size()), 2);
    QCOMPARE(res.value.totalPagesProcessed, 2);

    for (const auto& f : res.value.generatedFiles) {
        QVERIFY(std::filesystem::exists(f));
        auto openRes = m_engine->OpenDocument(f);
        QVERIFY(openRes.has_value());
        QCOMPARE(m_engine->GetPageCount(openRes.value), 1);
        m_engine->CloseDocument(openRes.value);
    }
}

void TestDocumentAssembly::testSplitEveryNPages() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages

    services::SplitOptions options;
    options.mode = services::SplitMode::EveryNPages;
    options.pagesPerChunk = 2;
    options.filePrefix = "chunk_";

    auto res = m_service->SplitDocument(doc, outDir, options);
    QVERIFY(res.has_value());
    QCOMPARE(static_cast<int>(res.value.generatedFiles.size()), 1);

    auto openRes = m_engine->OpenDocument(res.value.generatedFiles[0]);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 2);
    m_engine->CloseDocument(openRes.value);
}

void TestDocumentAssembly::testSplitByCustomRanges() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages

    services::SplitOptions options;
    options.mode = services::SplitMode::ByPageRanges;
    options.customRanges = {"1", "2"};
    options.filePrefix = "custom_";

    auto res = m_service->SplitDocument(doc, outDir, options);
    QVERIFY(res.has_value());
    QCOMPARE(static_cast<int>(res.value.generatedFiles.size()), 2);
}

void TestDocumentAssembly::testExtractPages() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "extracted.pdf";

    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages

    auto res = m_service->ExtractPages(doc, "2", outPath);
    QVERIFY(res.has_value());

    auto openRes = m_engine->OpenDocument(outPath);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 1);
    m_engine->CloseDocument(openRes.value);
}

void TestDocumentAssembly::testReorderDocument() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "reordered.pdf";

    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages: page 0, page 1

    // Reverse order: {1, 0}
    auto res = m_service->ReorderDocument(doc, {1, 0}, outPath);
    QVERIFY(res.has_value());

    auto openRes = m_engine->OpenDocument(outPath);
    QVERIFY(openRes.has_value());
    QCOMPARE(m_engine->GetPageCount(openRes.value), 2);

    // Verify first page of reordered document matches page 1 dimensions of ordinary.pdf
    auto p1Size = m_engine->GetPageSize(openRes.value, 0);
    QVERIFY(p1Size.has_value());
    QCOMPARE(static_cast<int>(p1Size.value.width), 595); // Page 1 of ordinary was A4 595x842
    m_engine->CloseDocument(openRes.value);
}

void TestDocumentAssembly::testInDocumentPageOperations() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto copyPath = std::filesystem::path(tempDir.path().toStdString()) / "mod_test.pdf";

    // Create a new blank document
    auto newDocRes = m_engine->CreateNewDocument();
    QVERIFY(newDocRes.has_value());
    auto docId = newDocRes.value;

    // Insert 2 blank pages
    auto b1 = m_engine->InsertBlankPage(docId, 0, 612.0, 792.0);
    QVERIFY(b1.has_value());
    auto b2 = m_engine->InsertBlankPage(docId, 1, 595.0, 842.0);
    QVERIFY(b2.has_value());
    QCOMPARE(m_engine->GetPageCount(docId), 2);

    // Rotate page 0 by 90 degrees
    auto rotRes = m_engine->RotatePages(docId, {0}, 90);
    QVERIFY(rotRes.has_value());

    // Resize page 1 to 800x600
    auto resRes = m_engine->ResizePage(docId, 1, 800.0, 600.0);
    QVERIFY(resRes.has_value());

    // Crop page 0
    pdf_api::PdfRect cropBox{10.0, 10.0, 500.0, 700.0};
    auto cropRes = m_engine->CropPage(docId, 0, cropBox);
    QVERIFY(cropRes.has_value());

    // Save and reload
    auto saveRes = m_engine->SaveCopy(docId, copyPath);
    QVERIFY(saveRes.has_value());
    m_engine->CloseDocument(docId);

    // Verify reloaded document
    auto reloadRes = m_engine->OpenDocument(copyPath);
    QVERIFY(reloadRes.has_value());
    QCOMPARE(m_engine->GetPageCount(reloadRes.value), 2);

    // Delete 1 page
    auto delRes = m_engine->DeletePages(reloadRes.value, {0});
    QVERIFY(delRes.has_value());
    QCOMPARE(m_engine->GetPageCount(reloadRes.value), 1);

    m_engine->CloseDocument(reloadRes.value);
}

void TestDocumentAssembly::testInvalidOperationsFailSafely() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "should_not_exist.pdf";

    // Non-existent source file
    std::vector<services::MergeSource> badSources = {
        {"non_existent_file.pdf", "all", {}}
    };
    auto res = m_service->MergeDocuments(badSources, outPath);
    QVERIFY(!res.has_value());
    QVERIFY(!std::filesystem::exists(outPath));

    // Invalid page range for existing document
    auto doc = m_fixturesDir / "ordinary.pdf"; // 2 pages
    auto badRangeRes = m_service->ExtractPages(doc, "99-100", outPath);
    QVERIFY(!badRangeRes.has_value());
    QVERIFY(!std::filesystem::exists(outPath));
}

QTEST_MAIN(TestDocumentAssembly)
#include "test_document_assembly.moc"
