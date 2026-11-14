// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QSignalSpy>
#include <QTableWidget>
#include <QLabel>
#include "ui/ObjectInspectorDialog.hpp"
#include "ui/ContentEditingToolbar.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;
using namespace pdfinity::ui;

class TestContentEditingUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInspectorDialogLifecycle();
    void testToolbarActionsAndLayerBadge();
    void testSubsetFontWarningInInspector();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestContentEditingUi::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestContentEditingUi::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestContentEditingUi::testInspectorDialogLifecycle() {
    auto path = m_fixturesDir / "vector_art.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, path);

    auto editService = std::make_shared<services::ContentEditingService>();
    ObjectInspectorDialog dlg(doc, 0, editService);

    QVERIFY(dlg.windowTitle().contains("Object Inspector"));

    auto* table = dlg.findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QVERIFY(table->rowCount() > 0);

    // Verify row selection signal
    QSignalSpy spy(&dlg, &ObjectInspectorDialog::ObjectSelected);
    table->cellClicked(0, 0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
}

void TestContentEditingUi::testToolbarActionsAndLayerBadge() {
    auto editService = std::make_shared<services::ContentEditingService>();
    ContentEditingToolbar toolbar(editService);

    auto actions = toolbar.actions();
    QVERIFY(!actions.isEmpty());

    bool foundBadge = false;
    bool foundTextAct = false;
    bool foundShapeAct = false;
    bool foundImageAct = false;

    for (auto* act : actions) {
        if (act->text().contains("LAYER: Native Stream")) {
            foundBadge = true;
            QVERIFY(!act->isEnabled()); // Indicator only
        } else if (act->text().contains("Add Text")) {
            foundTextAct = true;
        } else if (act->text().contains("Add Rect")) {
            foundShapeAct = true;
        } else if (act->text().contains("Add Image")) {
            foundImageAct = true;
        }
    }

    QVERIFY(foundBadge);
    QVERIFY(foundTextAct);
    QVERIFY(foundShapeAct);
    QVERIFY(foundImageAct);
}

void TestContentEditingUi::testSubsetFontWarningInInspector() {
    auto path = m_fixturesDir / "subset_font.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    auto doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, path);

    auto editService = std::make_shared<services::ContentEditingService>();
    ObjectInspectorDialog dlg(doc, 0, editService);

    auto* table = dlg.findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QVERIFY(table->rowCount() > 0);

    // Check that warning banner is visible for subset font
    auto* banner = dlg.findChild<QLabel*>();
    QVERIFY(banner != nullptr);
    // Select row 0 (which has the subset font)
    table->cellClicked(0, 0);
    // Warning banner should contain "Subset Font Detected"
    auto labels = dlg.findChildren<QLabel*>();
    bool foundWarning = false;
    for (auto* l : labels) {
        if (l->text().contains("Subset Font Detected") && !l->isHidden()) {
            foundWarning = true;
            break;
        }
    }
    QVERIFY(foundWarning);
}

QTEST_MAIN(TestContentEditingUi)
#include "test_content_editing_ui.moc"
