// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/DocumentIntelligenceService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestDocumentIntelligence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAnalyzeDocumentMetadataAndBoxes();
    void testAnalyzeFontsAndColorSpaces();
    void testUpdateMetadataAndXmp();
    void testEmbedAndExtractAttachment();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestDocumentIntelligence::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestDocumentIntelligence::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestDocumentIntelligence::testAnalyzeDocumentMetadataAndBoxes() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::DocumentIntelligenceService::Instance().AnalyzeDocument(doc);
    QVERIFY(res.has_value());

    const auto& intel = res.value;
    QCOMPARE(intel.pageBoxes.size(), 2);

    for (const auto& b : intel.pageBoxes) {
        QVERIFY(b.mediaBox.width > 0);
        QVERIFY(b.mediaBox.height > 0);
        QVERIFY(b.cropBox.width > 0);
    }
}

void TestDocumentIntelligence::testAnalyzeFontsAndColorSpaces() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::DocumentIntelligenceService::Instance().AnalyzeDocument(doc);
    QVERIFY(res.has_value());

    const auto& intel = res.value;
    // Check fonts
    for (const auto& f : intel.fonts) {
        QVERIFY(!f.baseFontName.empty());
    }

    // Check images
    auto imgPdf = m_fixturesDir / "image_heavy.pdf";
    auto imgOpen = m_engine->OpenDocument(imgPdf);
    QVERIFY(imgOpen.has_value());
    domain::DocumentModel imgDoc(imgOpen.value, m_engine, imgPdf);

    auto imgIntel = services::DocumentIntelligenceService::Instance().AnalyzeDocument(imgDoc);
    QVERIFY(imgIntel.has_value());
    QVERIFY(!imgIntel.value.images.empty());

    for (const auto& img : imgIntel.value.images) {
        QVERIFY(img.pixelWidth > 0);
        QVERIFY(img.pixelHeight > 0);
        QVERIFY(!img.colorSpace.empty());
    }
}

void TestDocumentIntelligence::testUpdateMetadataAndXmp() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "updated_meta.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    pdf_api::DocumentMetadata newMeta;
    newMeta.title = "Updated Audit Document Title";
    newMeta.author = "Senior Reviewer";
    newMeta.subject = "Compliance Test Subject";

    auto updateRes = services::DocumentIntelligenceService::Instance().UpdateMetadata(doc, dstPdf, newMeta);
    QVERIFY(updateRes.has_value());
    QVERIFY(std::filesystem::exists(dstPdf));

    // Verify written file contains updated fields
    std::ifstream in(dstPdf, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(content.find("Updated Audit Document Title") != std::string::npos);
    QVERIFY(content.find("Senior Reviewer") != std::string::npos);
    QVERIFY(content.find("<x:xmpmeta") != std::string::npos);
}

void TestDocumentIntelligence::testEmbedAndExtractAttachment() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create a dummy attachment file
    auto attachSrc = std::filesystem::path(tempDir.path().toStdString()) / "data.csv";
    {
        std::ofstream out(attachSrc);
        out << "id,name,value\n1,alpha,100\n2,beta,200\n";
    }

    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    int initialCount = 0;
    auto initialAtts = doc.GetDocumentAttachments();
    if (initialAtts.has_value()) initialCount = static_cast<int>(initialAtts.value.size());

    // Embed attachment
    auto embedRes = services::DocumentIntelligenceService::Instance().EmbedAttachment(
        doc, attachSrc, "Test CSV Data"
    );
    QVERIFY(embedRes.has_value());

    auto attsAfter = doc.GetDocumentAttachments();
    QVERIFY(attsAfter.has_value());
    QCOMPARE(attsAfter.value.size(), initialCount + 1);

    // Extract attachment back to disk
    auto extractedDst = std::filesystem::path(tempDir.path().toStdString()) / "extracted.csv";
    auto extractRes = services::DocumentIntelligenceService::Instance().ExtractAttachment(
        doc, initialCount, extractedDst
    );
    QVERIFY(extractRes.has_value());
    QVERIFY(std::filesystem::exists(extractedDst));
    QVERIFY(std::filesystem::file_size(extractedDst) > 0);
}

QTEST_MAIN(TestDocumentIntelligence)
#include "test_document_intelligence.moc"
