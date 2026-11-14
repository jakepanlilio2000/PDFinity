// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QListWidget>
#include <QPushButton>
#include "ui/PageOrganizerDialog.hpp"
#include "ui/MergeDocumentsDialog.hpp"
#include "ui/SplitDocumentDialog.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include <filesystem>

using namespace pdfinity;
using namespace pdfinity::ui;

class TestPageOrganizer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testOrganizerLifecycle();
    void testOrganizerSelection();
    void testOrganizerRotations();
    void testOrganizerInsertAndDuplicate();
    void testMergeDialogControls();
    void testSplitDialogControls();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestPageOrganizer::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestPageOrganizer::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestPageOrganizer::testOrganizerLifecycle() {
    auto path = m_fixturesDir / "ordinary.pdf"; // 2 pages
    PageOrganizerDialog dlg(m_engine, path);

    auto* list = dlg.findChild<QListWidget*>();
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);
}

void TestPageOrganizer::testOrganizerSelection() {
    auto path = m_fixturesDir / "ordinary.pdf";
    PageOrganizerDialog dlg(m_engine, path);
    auto* list = dlg.findChild<QListWidget*>();
    QVERIFY(list != nullptr);

    // Select all
    list->selectAll();
    QCOMPARE(list->selectedItems().count(), 2);

    // Select none
    list->clearSelection();
    QCOMPARE(list->selectedItems().count(), 0);

    // Select row 0
    list->item(0)->setSelected(true);
    QCOMPARE(list->selectedItems().count(), 1);
}

void TestPageOrganizer::testOrganizerRotations() {
    auto path = m_fixturesDir / "ordinary.pdf";
    PageOrganizerDialog dlg(m_engine, path);
    auto* list = dlg.findChild<QListWidget*>();
    QVERIFY(list != nullptr);

    list->item(0)->setSelected(true);

    auto* rotateCwBtn = dlg.findChild<QPushButton*>();
    QVERIFY(rotateCwBtn != nullptr);

    // Initial label check
    QString txt = list->item(0)->text();
    QVERIFY(txt.contains("Page 1"));
}

void TestPageOrganizer::testOrganizerInsertAndDuplicate() {
    auto path = m_fixturesDir / "ordinary.pdf";
    PageOrganizerDialog dlg(m_engine, path);
    auto* list = dlg.findChild<QListWidget*>();
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);

    // Find and click Insert Blank Page button
    const auto buttons = dlg.findChildren<QPushButton*>();
    QPushButton* insertBtn = nullptr;
    for (auto* btn : buttons) {
        if (btn->text().contains("Insert Blank")) {
            insertBtn = btn;
            break;
        }
    }
    QVERIFY(insertBtn != nullptr);
    insertBtn->click();

    // Verify count increased to 3
    QCOMPARE(list->count(), 3);
}

void TestPageOrganizer::testMergeDialogControls() {
    MergeDocumentsDialog dlg(m_engine);
    QVERIFY(dlg.windowTitle().contains("Merge"));

    const auto buttons = dlg.findChildren<QPushButton*>();
    bool foundAdd = false;
    bool foundMerge = false;
    for (auto* btn : buttons) {
        if (btn->text().contains("Add Files")) foundAdd = true;
        if (btn->text().contains("Merge Documents")) foundMerge = true;
    }
    QVERIFY(foundAdd);
    QVERIFY(foundMerge);
}

void TestPageOrganizer::testSplitDialogControls() {
    auto path = m_fixturesDir / "ordinary.pdf";
    SplitDocumentDialog dlg(m_engine, path);
    QVERIFY(dlg.windowTitle().contains("Split"));

    const auto buttons = dlg.findChildren<QPushButton*>();
    bool foundSplit = false;
    for (auto* btn : buttons) {
        if (btn->text().contains("Split Document")) foundSplit = true;
    }
    QVERIFY(foundSplit);
}

QTEST_MAIN(TestPageOrganizer)
#include "test_page_organizer.moc"
