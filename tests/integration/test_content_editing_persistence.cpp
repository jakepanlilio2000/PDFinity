// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"

using namespace pdfinity;

class TestContentEditingPersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInspectVectorArtAndPaths();
    void testInspectScannedRasterImage();
    void testInspectSubsetFontWarning();
    void testInspectRotatedText();
    void testInspectUnicodeText();
    void testInsertTextAndPersistence();
    void testInsertShapeAndPersistence();
    void testInsertImageAndPersistence();
    void testTransformObjectAndPersistence();
    void testDeleteObjectAndPersistence();
    void testReorderZOrderAndPersistence();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestContentEditingPersistence::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestContentEditingPersistence::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestContentEditingPersistence::testInspectVectorArtAndPaths() {
    auto path = m_fixturesDir / "vector_art.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto objsRes = doc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    QVERIFY(objsRes.value.size() >= 3);

    bool foundText = false;
    bool foundPath = false;

    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text) {
            foundText = true;
            QVERIFY(!obj.text.empty());
        } else if (obj.type == pdf_api::PageObjectType::Path) {
            foundPath = true;
            QVERIFY(obj.segmentCount > 0);
            QVERIFY(obj.bounds.width > 0.0);
            QVERIFY(obj.bounds.height > 0.0);
        }
    }

    QVERIFY(foundText);
    QVERIFY(foundPath);
}

void TestContentEditingPersistence::testInspectScannedRasterImage() {
    auto path = m_fixturesDir / "scanned_page.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto objsRes = doc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    QVERIFY(!objsRes.value.empty());

    bool foundImage = false;
    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Image) {
            foundImage = true;
            QCOMPARE(obj.pixelWidth, 16);
            QCOMPARE(obj.pixelHeight, 16);
            QVERIFY(obj.bounds.width > 0.0);
            QVERIFY(obj.bounds.height > 0.0);
        }
    }
    QVERIFY(foundImage);
}

void TestContentEditingPersistence::testInspectSubsetFontWarning() {
    auto path = m_fixturesDir / "subset_font.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto objsRes = doc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    QVERIFY(!objsRes.value.empty());

    bool foundSubset = false;
    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text && obj.isSubsetFont) {
            foundSubset = true;
            QVERIFY(obj.fontName.find('+') != std::string::npos);
            QVERIFY(!obj.fontWarning.empty());
        }
    }
    QVERIFY(foundSubset);
}

void TestContentEditingPersistence::testInspectRotatedText() {
    auto path = m_fixturesDir / "rotated_text.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto objsRes = doc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    QVERIFY(objsRes.value.size() >= 2);

    bool foundRotated = false;
    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text) {
            // Check non-identity matrix (b != 0 or c != 0)
            if (std::abs(obj.matrix[1]) > 0.01 || std::abs(obj.matrix[2]) > 0.01) {
                foundRotated = true;
            }
        }
    }
    QVERIFY(foundRotated);
}

void TestContentEditingPersistence::testInspectUnicodeText() {
    auto path = m_fixturesDir / "unicode_text.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto objsRes = doc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    QVERIFY(!objsRes.value.empty());

    bool foundUnicode = false;
    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text && obj.text.find("PDFinity") != std::string::npos) {
            foundUnicode = true;
        }
    }
    QVERIFY(foundUnicode);
}

void TestContentEditingPersistence::testInsertTextAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "text_inserted.pdf";

    auto origPath = m_fixturesDir / "ordinary.pdf";
    uint64_t origSize = std::filesystem::file_size(origPath);

    // Open original
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    int origObjCount = static_cast<int>(doc.GetPageObjects(0).value.size());

    // Insert native text directly into content stream
    std::string testString = "PDFinity Native Content Injection Verification";
    auto insertRes = doc.InsertPageText(
        0, testString, 72.0, 500.0, "Helvetica", 14.0, {255, 0, 0, 255}
    );
    QVERIFY(insertRes.has_value());

    // Save copy
    QVERIFY(doc.SaveCopy(outPath).has_value());
    QVERIFY(std::filesystem::exists(outPath));

    // Immutability: original file must not be modified
    uint64_t origSizeAfter = std::filesystem::file_size(origPath);
    QCOMPARE(origSize, origSizeAfter);

    // Reopen saved file in clean engine
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto savedObjsRes = savedDoc.GetPageObjects(0);
    QVERIFY(savedObjsRes.has_value());
    QCOMPARE(static_cast<int>(savedObjsRes.value.size()), origObjCount + 1);

    bool foundInserted = false;
    for (const auto& obj : savedObjsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text && obj.text.find("Native Content Injection") != std::string::npos) {
            foundInserted = true;
            QCOMPARE(QString::fromStdString(obj.fontName), QString("Helvetica"));
            QCOMPARE(obj.fontSize, 14.0);
            QCOMPARE(obj.fillColor.r, static_cast<uint8_t>(255));
        }
    }
    QVERIFY(foundInserted);
}

void TestContentEditingPersistence::testInsertShapeAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "shape_inserted.pdf";

    auto origPath = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    int origObjCount = static_cast<int>(doc.GetPageObjects(0).value.size());

    pdf_api::PdfRect rect{100.0, 300.0, 200.0, 80.0};
    auto insertRes = doc.InsertPageShape(0, rect, {0, 0, 255, 255}, {200, 220, 255, 255}, 2.5);
    QVERIFY(insertRes.has_value());

    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen and verify
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto savedObjsRes = savedDoc.GetPageObjects(0);
    QVERIFY(savedObjsRes.has_value());
    QCOMPARE(static_cast<int>(savedObjsRes.value.size()), origObjCount + 1);

    bool foundShape = false;
    for (const auto& obj : savedObjsRes.value) {
        // PDF stroke of 2.5 with joins adds up to 2.5 on each side (total width ~205.0)
        if (obj.type == pdf_api::PageObjectType::Path && std::abs(obj.bounds.width - 200.0) <= 8.0) {
            foundShape = true;
            QVERIFY(obj.strokeWidth >= 2.0);
        }
    }
    QVERIFY(foundShape);
}

void TestContentEditingPersistence::testInsertImageAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "image_inserted.pdf";

    auto origPath = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    int origObjCount = static_cast<int>(doc.GetPageObjects(0).value.size());

    // Generate 10x10 BGRA buffer
    std::vector<uint8_t> bgraPixels(10 * 10 * 4, 128);
    auto insertRes = doc.InsertPageImage(0, bgraPixels, 10, 10, 50.0, 200.0, 100.0, 100.0);
    QVERIFY(insertRes.has_value());

    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen and verify
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto savedObjs = savedDoc.GetPageObjects(0);
    QVERIFY(savedObjs.has_value());
    QCOMPARE(static_cast<int>(savedObjs.value.size()), origObjCount + 1);

    bool foundImg = false;
    for (const auto& obj : savedObjs.value) {
        if (obj.type == pdf_api::PageObjectType::Image) {
            foundImg = true;
            QCOMPARE(obj.pixelWidth, 10);
            QCOMPARE(obj.pixelHeight, 10);
        }
    }
    QVERIFY(foundImg);
}

void TestContentEditingPersistence::testTransformObjectAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "transformed.pdf";

    auto origPath = m_fixturesDir / "vector_art.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    auto objsBefore = doc.GetPageObjects(0);
    QVERIFY(objsBefore.has_value() && !objsBefore.value.empty());
    double origX = objsBefore.value[0].bounds.x;
    double origY = objsBefore.value[0].bounds.y;

    // Translate by dx=40, dy=30
    QVERIFY(doc.TransformPageObject(0, 0, 40.0, 30.0, 1.0, 1.0).has_value());
    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen and verify position shifted
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto objsAfter = savedDoc.GetPageObjects(0);
    QVERIFY(objsAfter.has_value());
    QVERIFY(std::abs(objsAfter.value[0].bounds.x - (origX + 40.0)) < 1.0);
    QVERIFY(std::abs(objsAfter.value[0].bounds.y - (origY + 30.0)) < 1.0);
}

void TestContentEditingPersistence::testDeleteObjectAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "deleted.pdf";

    auto origPath = m_fixturesDir / "vector_art.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    auto objsBefore = doc.GetPageObjects(0);
    QVERIFY(objsBefore.has_value());
    int countBefore = static_cast<int>(objsBefore.value.size());

    // Delete object 0
    QVERIFY(doc.DeletePageObject(0, 0).has_value());
    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen and verify count is reduced by 1
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto objsAfter = savedDoc.GetPageObjects(0);
    QVERIFY(objsAfter.has_value());
    QCOMPARE(static_cast<int>(objsAfter.value.size()), countBefore - 1);
}

void TestContentEditingPersistence::testReorderZOrderAndPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "reordered.pdf";

    auto origPath = m_fixturesDir / "vector_art.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    auto objsBefore = doc.GetPageObjects(0);
    QVERIFY(objsBefore.has_value() && objsBefore.value.size() >= 2);
    auto firstType = objsBefore.value[0].type;

    // Move object 0 to index 1
    QVERIFY(doc.ReorderPageObject(0, 0, 1).has_value());
    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen and verify
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto objsAfter = savedDoc.GetPageObjects(0);
    QVERIFY(objsAfter.has_value());
    QCOMPARE(objsAfter.value[1].type, firstType);
}

QTEST_MAIN(TestContentEditingPersistence)
#include "test_content_editing_persistence.moc"
