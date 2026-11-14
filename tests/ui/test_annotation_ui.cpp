// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QSignalSpy>
#include <QTreeWidget>
#include <QAction>
#include "ui/AnnotationToolbar.hpp"
#include "ui/AnnotationPropertiesDialog.hpp"
#include "ui/NavigationSidebar.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"
#include "services/AnnotationService.hpp"

using namespace pdfinity;
using namespace pdfinity::ui;

class TestAnnotationUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testToolbarToolSelection();
    void testToolbarUndoRedoState();
    void testPropertiesDialog();
    void testSidebarAnnotationsTree();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_fixturesDir;
};

void TestAnnotationUi::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    auto path = m_fixturesDir / "ordinary.pdf";

    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    m_doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, path);
}

void TestAnnotationUi::cleanupTestCase() {
    m_doc.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestAnnotationUi::testToolbarToolSelection() {
    AnnotationToolbar tb;
    QSignalSpy toolSpy(&tb, &AnnotationToolbar::ToolChanged);
    QSignalSpy colorSpy(&tb, &AnnotationToolbar::ColorChanged);

    QCOMPARE(tb.GetActiveTool(), AnnotationToolMode::Select);

    // Find and trigger Highlight action
    const auto actions = tb.actions();
    QAction* highlightAct = nullptr;
    QAction* noteAct = nullptr;

    for (auto* act : actions) {
        if (act->text().contains("Highlight")) highlightAct = act;
        if (act->text().contains("Sticky Note")) noteAct = act;
    }

    QVERIFY(highlightAct != nullptr);
    QVERIFY(noteAct != nullptr);

    highlightAct->trigger();
    QCOMPARE(tb.GetActiveTool(), AnnotationToolMode::Highlight);
    QCOMPARE(toolSpy.count(), 1);

    noteAct->trigger();
    QCOMPARE(tb.GetActiveTool(), AnnotationToolMode::StickyNote);
    QCOMPARE(toolSpy.count(), 2);
}

void TestAnnotationUi::testToolbarUndoRedoState() {
    AnnotationToolbar tb;
    QSignalSpy undoSpy(&tb, &AnnotationToolbar::UndoRequested);
    QSignalSpy redoSpy(&tb, &AnnotationToolbar::RedoRequested);

    tb.SetUndoEnabled(true);
    tb.SetRedoEnabled(true);

    const auto actions = tb.actions();
    for (auto* act : actions) {
        if (act->text().contains("Undo")) act->trigger();
        if (act->text().contains("Redo")) act->trigger();
    }

    QCOMPARE(undoSpy.count(), 1);
    QCOMPARE(redoSpy.count(), 1);
}

void TestAnnotationUi::testPropertiesDialog() {
    auto data = services::AnnotationService::CreateStickyNote(0, {100.0, 100.0}, "Sample Note Text", "Test Author");
    AnnotationPropertiesDialog dlg(data);

    auto updated = dlg.GetUpdatedData();
    QCOMPARE(QString::fromStdString(updated.contents), QString("Sample Note Text"));
    QCOMPARE(QString::fromStdString(updated.author), QString("Test Author"));
    QVERIFY(!dlg.DeleteRequested());
}

void TestAnnotationUi::testSidebarAnnotationsTree() {
    // Add an annotation to the document first
    auto h = services::AnnotationService::CreateHighlight(0, {50.0, 50.0, 150.0, 20.0}, {}, {255, 255, 0, 180}, "Sidebar Test Item");
    m_doc->AddAnnotation(0, h);

    NavigationSidebar sidebar(m_doc, nullptr);
    QSignalSpy annotSelectedSpy(&sidebar, &NavigationSidebar::AnnotationSelected);

    auto* tree = sidebar.findChild<QTreeWidget*>("", Qt::FindChildrenRecursively);
    QVERIFY(tree != nullptr);

    // Call populate annotations
    sidebar.PopulateAnnotations();

    // Verify top-level items or children exist
    QVERIFY(sidebar.findChildren<QTreeWidget*>().count() >= 2);
}

QTEST_MAIN(TestAnnotationUi)
#include "test_annotation_ui.moc"
