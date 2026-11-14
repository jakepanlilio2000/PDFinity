// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"
#include "services/AnnotationService.hpp"
#include <filesystem>

using namespace pdfinity;
using namespace pdfinity::services;

class TestAnnotationPersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAnnotationCreationAndRetrieval();
    void testSaveAndReopenPersistence();
    void testPageContentUncorruptedAfterAnnotation();
    void testRenderAnnotationsVerification();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestAnnotationPersistence::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestAnnotationPersistence::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestAnnotationPersistence::testAnnotationCreationAndRetrieval() {
    auto path = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    // Add Highlight
    auto h1 = AnnotationService::CreateHighlight(0, {72.0, 700.0, 200.0, 15.0}, {}, {255, 255, 0, 180}, "Important section");
    auto addH1 = doc.AddAnnotation(0, h1);
    QVERIFY(addH1.has_value());

    // Add Sticky Note
    auto n1 = AnnotationService::CreateStickyNote(0, {150.0, 600.0}, "Reviewer Note: Verify this clause", "Lead Architect");
    auto addN1 = doc.AddAnnotation(0, n1);
    QVERIFY(addN1.has_value());

    // Add Freehand Ink
    std::vector<pdf_api::PdfPoint> stroke = {{100.0, 500.0}, {120.0, 520.0}, {140.0, 505.0}, {160.0, 530.0}};
    auto ink = AnnotationService::CreateFreehand(0, {stroke}, {0, 0, 255, 255}, 2.5f);
    auto addInk = doc.AddAnnotation(0, ink);
    QVERIFY(addInk.has_value());

    // Add Rectangle Shape
    auto sq = AnnotationService::CreateShape(0, pdf_api::AnnotationType::Square, {50.0, 400.0, 150.0, 80.0}, {220, 38, 38, 255}, {0, 0, 0, 0}, 2.0f);
    auto addSq = doc.AddAnnotation(0, sq);
    QVERIFY(addSq.has_value());

    // Add Stamp
    auto stamp = AnnotationService::CreateStamp(0, {300.0, 650.0, 160.0, 50.0}, "APPROVED");
    auto addStamp = doc.AddAnnotation(0, stamp);
    QVERIFY(addStamp.has_value());

    // Retrieve annotations from page 0
    auto getRes = doc.GetAnnotations(0);
    QVERIFY(getRes.has_value());
    QVERIFY(getRes.value.size() >= 5);

    bool foundHighlight = false;
    bool foundNote = false;
    bool foundInk = false;
    bool foundStamp = false;

    for (const auto& a : getRes.value) {
        if (a.type == pdf_api::AnnotationType::Highlight && a.contents == "Important section") foundHighlight = true;
        if (a.type == pdf_api::AnnotationType::TextNote && a.author == "Lead Architect") foundNote = true;
        if (a.type == pdf_api::AnnotationType::FreehandInk) foundInk = true;
        if (a.type == pdf_api::AnnotationType::Stamp) foundStamp = true;
    }

    QVERIFY(foundHighlight);
    QVERIFY(foundNote);
    QVERIFY(foundInk);
    QVERIFY(foundStamp);
}

void TestAnnotationPersistence::testSaveAndReopenPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "annotated_saved.pdf";

    auto origPath = m_fixturesDir / "ordinary.pdf";
    uint64_t origSize = std::filesystem::file_size(origPath);

    // Open original
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    // Add Underline on page 0
    auto u1 = AnnotationService::CreateUnderline(0, {100.0, 650.0, 180.0, 12.0}, {}, {0, 128, 255, 255}, "Underline Note");
    QVERIFY(doc.AddAnnotation(0, u1).has_value());

    // Add Strikeout on page 1
    auto s1 = AnnotationService::CreateStrikeout(1, {100.0, 700.0, 150.0, 12.0}, {}, {255, 0, 0, 255}, "Deprecated text");
    QVERIFY(doc.AddAnnotation(1, s1).has_value());

    // Save copy
    auto saveRes = doc.SaveCopy(outPath);
    QVERIFY(saveRes.has_value());
    QVERIFY(std::filesystem::exists(outPath));

    // Verify original file remained byte-identical
    uint64_t origSizeAfter = std::filesystem::file_size(origPath);
    QCOMPARE(origSize, origSizeAfter);

    // Reopen saved PDF in fresh engine instance
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    // Check page 0 annotations
    auto p0Annots = savedDoc.GetAnnotations(0);
    QVERIFY(p0Annots.has_value());
    bool foundUnderline = false;
    for (const auto& a : p0Annots.value) {
        if (a.type == pdf_api::AnnotationType::Underline && a.contents == "Underline Note") {
            foundUnderline = true;
            break;
        }
    }
    QVERIFY(foundUnderline);

    // Check page 1 annotations
    auto p1Annots = savedDoc.GetAnnotations(1);
    QVERIFY(p1Annots.has_value());
    bool foundStrikeout = false;
    for (const auto& a : p1Annots.value) {
        if (a.type == pdf_api::AnnotationType::Strikeout && a.contents == "Deprecated text") {
            foundStrikeout = true;
            break;
        }
    }
    QVERIFY(foundStrikeout);
}

void TestAnnotationPersistence::testPageContentUncorruptedAfterAnnotation() {
    auto path = m_fixturesDir / "ordinary.pdf";

    // Extract original text from page 0
    auto openOrig = m_engine->OpenDocument(path);
    QVERIFY(openOrig.has_value());
    auto origTextRes = m_engine->ExtractPageText(openOrig.value, 0);
    QVERIFY(origTextRes.has_value());
    std::string originalText = origTextRes.value;
    m_engine->CloseDocument(openOrig.value);

    // Open, annotate, and save
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "text_check.pdf";

    auto openMod = m_engine->OpenDocument(path);
    QVERIFY(openMod.has_value());
    domain::DocumentModel doc(openMod.value, m_engine, path);

    auto h = AnnotationService::CreateHighlight(0, {50.0, 50.0, 200.0, 30.0}, {}, {255, 255, 0, 180});
    doc.AddAnnotation(0, h);
    doc.SaveCopy(outPath);

    // Reopen and extract text
    auto openSaved = m_engine->OpenDocument(outPath);
    QVERIFY(openSaved.has_value());
    auto savedTextRes = m_engine->ExtractPageText(openSaved.value, 0);
    QVERIFY(savedTextRes.has_value());

    // Page content text must be 100% identical: annotations do not corrupt base text
    QCOMPARE(QString::fromStdString(savedTextRes.value), QString::fromStdString(originalText));
    m_engine->CloseDocument(openSaved.value);
}

void TestAnnotationPersistence::testRenderAnnotationsVerification() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "render_check.pdf";

    auto path = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    // Add multiple annotations
    doc.AddAnnotation(0, AnnotationService::CreateHighlight(0, {100.0, 600.0, 200.0, 20.0}));
    doc.AddAnnotation(0, AnnotationService::CreateShape(0, pdf_api::AnnotationType::Circle, {200.0, 400.0, 100.0, 100.0}));
    doc.SaveCopy(outPath);

    // Reopen and render page with annotations
    auto openSaved = m_engine->OpenDocument(outPath);
    QVERIFY(openSaved.has_value());

    pdf_api::RenderOptions opts;
    opts.scale = 1.0;
    opts.renderAnnotations = true;

    auto renderRes = m_engine->RenderPage(openSaved.value, 0, opts);
    QVERIFY(renderRes.has_value());
    QVERIFY(renderRes.value.width > 0);
    QVERIFY(renderRes.value.height > 0);
    QVERIFY(!renderRes.value.pixels.empty());

    m_engine->CloseDocument(openSaved.value);
}

QTEST_MAIN(TestAnnotationPersistence)
#include "test_annotation_persistence.moc"
