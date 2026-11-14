// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "services/ContentEditingService.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;
using namespace pdfinity::services;

class TestContentEditingService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAlignmentCalculations();
    void testLayoutCollisionDetection();
    void testLayoutMarginOverflow();
    void testFontCapabilitiesAnalysis();
    void testUndoRedoHistory();
    void testClipboardCopyAndPaste();
    void testLayerDescriptionDistinction();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestContentEditingService::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestContentEditingService::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestContentEditingService::testAlignmentCalculations() {
    std::vector<pdf_api::PageObjectData> objs;

    pdf_api::PageObjectData o1;
    o1.bounds = {50.0, 100.0, 100.0, 50.0};
    objs.push_back(o1);

    pdf_api::PageObjectData o2;
    o2.bounds = {150.0, 200.0, 80.0, 40.0};
    objs.push_back(o2);

    // Align Left: minLeft is 50.0
    auto leftDeltas = ContentEditingService::CalculateAlignment(AlignmentType::Left, objs);
    QCOMPARE(leftDeltas.size(), 2ULL);
    QCOMPARE(leftDeltas[0].first, 0.0);
    QCOMPARE(leftDeltas[1].first, -100.0); // 150 -> 50

    // Align Right: maxRight is max(50+100, 150+80) = 230.0
    auto rightDeltas = ContentEditingService::CalculateAlignment(AlignmentType::Right, objs);
    QCOMPARE(rightDeltas.size(), 2ULL);
    QCOMPARE(rightDeltas[0].first, 80.0); // 150 -> 230
    QCOMPARE(rightDeltas[1].first, 0.0);  // 230 -> 230

    // Align Bottom: minBottom is 100.0
    auto botDeltas = ContentEditingService::CalculateAlignment(AlignmentType::Bottom, objs);
    QCOMPARE(botDeltas.size(), 2ULL);
    QCOMPARE(botDeltas[0].second, 0.0);
    QCOMPARE(botDeltas[1].second, -100.0); // 200 -> 100
}

void TestContentEditingService::testLayoutCollisionDetection() {
    pdf_api::PageObjectData target;
    target.objectIndex = 0;
    target.type = pdf_api::PageObjectType::Text;
    target.text = "Short text";
    target.fontSize = 12.0;
    target.bounds = {50.0, 500.0, 60.0, 15.0};

    pdf_api::PageObjectData neighbor;
    neighbor.objectIndex = 1;
    neighbor.type = pdf_api::PageObjectType::Text;
    neighbor.text = "Neighbor text block";
    neighbor.bounds = {130.0, 500.0, 100.0, 15.0};

    std::vector<pdf_api::PageObjectData> allObjects = {target, neighbor};

    // 1. Safe small edit: no collision
    auto warnSafe = ContentEditingService::ValidateLayoutSafety(
        target, "Short abc", 12.0, allObjects
    );
    QVERIFY(!warnSafe.hasCollision);
    QVERIFY(!warnSafe.hasOverflow);

    // 2. Large text expansion: expands width by 3x (from 60 to ~180), colliding with neighbor at x=130
    auto warnColl = ContentEditingService::ValidateLayoutSafety(
        target, "Extremely long modified text that significantly exceeds the original bounding width", 12.0, allObjects
    );
    QVERIFY(warnColl.hasCollision);
    QCOMPARE(warnColl.conflictingObjectIndex, 1);
    QVERIFY(!warnColl.warningMessage.empty());
}

void TestContentEditingService::testLayoutMarginOverflow() {
    pdf_api::PageObjectData target;
    target.objectIndex = 0;
    target.type = pdf_api::PageObjectType::Text;
    target.text = "Right Margin Text";
    target.fontSize = 12.0;
    target.bounds = {550.0, 500.0, 50.0, 15.0};

    std::vector<pdf_api::PageObjectData> allObjects = {target};

    // Expanding beyond page width 612
    auto warn = ContentEditingService::ValidateLayoutSafety(
        target, "Much longer text expanding far beyond standard page width margins", 12.0, allObjects, 612.0, 792.0
    );
    QVERIFY(warn.hasOverflow);
    QVERIFY(!warn.warningMessage.empty());
}

void TestContentEditingService::testFontCapabilitiesAnalysis() {
    // 1. Standard Helvetica
    pdf_api::PageObjectData stdObj;
    stdObj.type = pdf_api::PageObjectType::Text;
    stdObj.fontName = "Helvetica";
    std::string stdCap = ContentEditingService::AnalyzeFontCapabilities(stdObj);
    QVERIFY(stdCap.find("Standard Font") != std::string::npos);

    // 2. Embedded Subset Font
    pdf_api::PageObjectData subObj;
    subObj.type = pdf_api::PageObjectType::Text;
    subObj.fontName = "ABCDEF+Arial";
    subObj.isSubsetFont = true;
    std::string subCap = ContentEditingService::AnalyzeFontCapabilities(subObj);
    QVERIFY(subCap.find("Embedded Subset Font") != std::string::npos);
    QVERIFY(subCap.find("standard Helvetica substitution") != std::string::npos);
}

void TestContentEditingService::testUndoRedoHistory() {
    ContentEditingService service;
    QVERIFY(!service.CanUndo());
    QVERIFY(!service.CanRedo());

    // Mock command
    class MockCommand : public IContentEditCommand {
    public:
        explicit MockCommand(int& val) : m_val(val) {}
        bool Execute() override { m_val++; return true; }
        bool Undo() override { m_val--; return true; }
        std::string GetDescription() const override { return "Mock"; }
    private:
        int& m_val;
    };

    int counter = 0;
    QVERIFY(service.ExecuteCommand(std::make_unique<MockCommand>(counter)));
    QCOMPARE(counter, 1);
    QVERIFY(service.CanUndo());
    QVERIFY(!service.CanRedo());

    QVERIFY(service.Undo());
    QCOMPARE(counter, 0);
    QVERIFY(!service.CanUndo());
    QVERIFY(service.CanRedo());

    QVERIFY(service.Redo());
    QCOMPARE(counter, 1);
    QVERIFY(service.CanUndo());
    QVERIFY(!service.CanRedo());
}

void TestContentEditingService::testClipboardCopyAndPaste() {
    ContentEditingService service;
    QVERIFY(!service.CanPaste());

    pdf_api::PageObjectData o1;
    o1.type = pdf_api::PageObjectType::Text;
    o1.text = "Copied Text";
    o1.bounds = {100.0, 200.0, 50.0, 10.0};

    service.CopyObjects({o1});
    QVERIFY(service.CanPaste());
    QCOMPARE(service.GetClipboard().size(), 1ULL);
    QCOMPARE(QString::fromStdString(service.GetClipboard()[0].text), QString("Copied Text"));
}

void TestContentEditingService::testLayerDescriptionDistinction() {
    QString nativeDesc = ContentEditingService::GetLayerDescription(pdf_api::ContentEditMode::NativeContentEdit);
    QVERIFY(nativeDesc.contains("Native Page Content Stream"));
    QVERIFY(!nativeDesc.contains("Annotation"));

    QString reconDesc = ContentEditingService::GetLayerDescription(pdf_api::ContentEditMode::Reconstruction);
    QVERIFY(reconDesc.contains("Reconstructed Content Stream"));

    QString overlayDesc = ContentEditingService::GetLayerDescription(pdf_api::ContentEditMode::OverlayPlacement);
    QVERIFY(overlayDesc.contains("Interactive Overlay"));
    QVERIFY(overlayDesc.contains("Not Native Stream"));
}

QTEST_MAIN(TestContentEditingService)
#include "test_content_editing_service.moc"
