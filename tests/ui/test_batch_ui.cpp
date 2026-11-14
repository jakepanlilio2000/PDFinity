// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QApplication>
#include <filesystem>
#include "ui/BatchCenterDialog.hpp"
#include "ui/BatchProfileEditorDialog.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestBatchUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testBatchCenterDialogLifecycle();
    void testBatchProfileEditorDialogLifecycle();

private:
    std::filesystem::path m_fixturesDir;
};

void TestBatchUi::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
    app::AppContext::Instance().Initialize();
}

void TestBatchUi::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestBatchUi::testBatchCenterDialogLifecycle() {
    ui::BatchCenterDialog dlg;
    QVERIFY(dlg.windowTitle().contains("Batch Processing"));
}

void TestBatchUi::testBatchProfileEditorDialogLifecycle() {
    ui::BatchProfileEditorDialog dlg;
    QVERIFY(dlg.windowTitle().contains("Batch Profile Editor"));
}

QTEST_MAIN(TestBatchUi)
#include "test_batch_ui.moc"
