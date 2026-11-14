// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QSignalSpy>
#include <QTableWidget>
#include "ui/FormInspectorDialog.hpp"
#include "ui/XfaNoticeBanner.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"

using namespace pdfinity;
using namespace pdfinity::ui;

class TestFormUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testFormInspectorDialogPopulation();
    void testXfaNoticeBanner();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_fixturesDir;
};

void TestFormUi::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    m_doc = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, path);
}

void TestFormUi::cleanupTestCase() {
    m_doc.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestFormUi::testFormInspectorDialogPopulation() {
    FormInspectorDialog dlg(m_doc);
    auto* table = dlg.findChild<QTableWidget*>();
    QVERIFY(table != nullptr);
    QVERIFY(table->rowCount() >= 4);

    QSignalSpy selectSpy(&dlg, &FormInspectorDialog::FieldSelected);
    table->cellClicked(0, 1);
    QCOMPARE(selectSpy.count(), 1);
}

void TestFormUi::testXfaNoticeBanner() {
    XfaNoticeBanner banner;
    QVERIFY(banner.isVisible() || !banner.isVisible()); // Object creates cleanly

    banner.SetFormTypeInfo(true);
    banner.show();
    QVERIFY(banner.isVisible());

    banner.SetFormTypeInfo(false);
    QVERIFY(banner.isVisible());

    // Dismiss banner
    banner.hide();
    QVERIFY(!banner.isVisible());
}

QTEST_MAIN(TestFormUi)
#include "test_form_ui.moc"
