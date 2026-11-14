// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "services/AnnotationCommandManager.hpp"
#include "services/AnnotationService.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;
using namespace pdfinity::services;

class TestAnnotationCommand : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAddCommandExecuteAndUndo();
    void testModifyCommandExecuteAndUndo();
    void testDeleteCommandExecuteAndUndo();
    void testCommandStackLimits();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_fixturesDir;
};

void TestAnnotationCommand::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    auto docPath = m_fixturesDir / "ordinary.pdf";

    auto openRes = m_engine->OpenDocument(docPath);
    QVERIFY(openRes.has_value());
    m_doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, docPath);
}

void TestAnnotationCommand::cleanupTestCase() {
    m_doc.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestAnnotationCommand::testAddCommandExecuteAndUndo() {
    AnnotationCommandManager mgr;
    QSignalSpy undoSpy(&mgr, &AnnotationCommandManager::CanUndoChanged);
    QSignalSpy redoSpy(&mgr, &AnnotationCommandManager::CanRedoChanged);

    int initialCount = 0;
    auto initialAnnots = m_doc->GetAnnotations(0);
    if (initialAnnots.has_value()) {
        initialCount = static_cast<int>(initialAnnots.value.size());
    }

    auto data = AnnotationService::CreateHighlight(0, {50.0, 100.0, 200.0, 20.0}, {}, {255, 255, 0, 180}, "Test Highlight");
    auto cmd = std::make_unique<AddAnnotationCommand>(m_doc, 0, data);

    QVERIFY(mgr.ExecuteCommand(std::move(cmd)));
    QVERIFY(mgr.CanUndo());
    QVERIFY(!mgr.CanRedo());
    QCOMPARE(QString::fromStdString(mgr.GetUndoDescription()), QString("Add Highlight"));

    auto afterAdd = m_doc->GetAnnotations(0);
    QVERIFY(afterAdd.has_value());
    QCOMPARE(static_cast<int>(afterAdd.value.size()), initialCount + 1);

    // Test Undo
    QVERIFY(mgr.Undo());
    QVERIFY(!mgr.CanUndo());
    QVERIFY(mgr.CanRedo());
    QCOMPARE(QString::fromStdString(mgr.GetRedoDescription()), QString("Add Highlight"));

    auto afterUndo = m_doc->GetAnnotations(0);
    QVERIFY(afterUndo.has_value());
    QCOMPARE(static_cast<int>(afterUndo.value.size()), initialCount);

    // Test Redo
    QVERIFY(mgr.Redo());
    QVERIFY(mgr.CanUndo());
    QVERIFY(!mgr.CanRedo());

    auto afterRedo = m_doc->GetAnnotations(0);
    QVERIFY(afterRedo.has_value());
    QCOMPARE(static_cast<int>(afterRedo.value.size()), initialCount + 1);

    // Clean up
    QVERIFY(mgr.Undo());
}

void TestAnnotationCommand::testModifyCommandExecuteAndUndo() {
    AnnotationCommandManager mgr;

    auto data = AnnotationService::CreateStickyNote(0, {100.0, 200.0}, "Initial Text", "Author A");
    auto addRes = m_doc->AddAnnotation(0, data);
    QVERIFY(addRes.has_value());
    int annotIdx = addRes.value;

    auto modifiedData = data;
    modifiedData.contents = "Updated Text";
    modifiedData.author = "Author B";

    auto cmd = std::make_unique<ModifyAnnotationCommand>(m_doc, 0, annotIdx, data, modifiedData);
    QVERIFY(mgr.ExecuteCommand(std::move(cmd)));

    auto checkRes = m_doc->GetAnnotations(0);
    QVERIFY(checkRes.has_value());
    QCOMPARE(QString::fromStdString(checkRes.value.back().contents), QString("Updated Text"));

    // Undo modification
    QVERIFY(mgr.Undo());
    auto checkUndo = m_doc->GetAnnotations(0);
    QVERIFY(checkUndo.has_value());
    QCOMPARE(QString::fromStdString(checkUndo.value.back().contents), QString("Initial Text"));

    // Redo modification
    QVERIFY(mgr.Redo());
    auto checkRedo = m_doc->GetAnnotations(0);
    QVERIFY(checkRedo.has_value());
    QCOMPARE(QString::fromStdString(checkRedo.value.back().contents), QString("Updated Text"));

    // Clean up
    m_doc->DeleteAnnotation(0, annotIdx);
}

void TestAnnotationCommand::testDeleteCommandExecuteAndUndo() {
    AnnotationCommandManager mgr;

    auto data = AnnotationService::CreateShape(0, pdf_api::AnnotationType::Square, {50.0, 50.0, 100.0, 100.0});
    auto addRes = m_doc->AddAnnotation(0, data);
    QVERIFY(addRes.has_value());
    int annotIdx = addRes.value;

    int countBeforeDelete = static_cast<int>(m_doc->GetAnnotations(0).value.size());

    auto cmd = std::make_unique<DeleteAnnotationCommand>(m_doc, 0, annotIdx, data);
    QVERIFY(mgr.ExecuteCommand(std::move(cmd)));

    int countAfterDelete = static_cast<int>(m_doc->GetAnnotations(0).value.size());
    QCOMPARE(countAfterDelete, countBeforeDelete - 1);

    // Undo delete
    QVERIFY(mgr.Undo());
    int countAfterUndo = static_cast<int>(m_doc->GetAnnotations(0).value.size());
    QCOMPARE(countAfterUndo, countBeforeDelete);

    // Clean up
    m_doc->DeleteAnnotation(0, annotIdx);
}

void TestAnnotationCommand::testCommandStackLimits() {
    AnnotationCommandManager mgr;
    QVERIFY(!mgr.CanUndo());
    QVERIFY(!mgr.CanRedo());

    auto data = AnnotationService::CreateHighlight(0, {10.0, 10.0, 50.0, 20.0});
    auto cmd = std::make_unique<AddAnnotationCommand>(m_doc, 0, data);
    mgr.ExecuteCommand(std::move(cmd));

    QVERIFY(mgr.CanUndo());
    mgr.Clear();
    QVERIFY(!mgr.CanUndo());
    QVERIFY(!mgr.CanRedo());
}

QTEST_MAIN(TestAnnotationCommand)
#include "test_annotation_command.moc"
