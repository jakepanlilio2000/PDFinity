// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QApplication>
#include <QTemporaryDir>
#include "ui/RedactionToolbar.hpp"
#include "ui/RedactionPreviewDialog.hpp"
#include "ui/SanitizationDialog.hpp"
#include "ui/RedactionReportDialog.hpp"

using namespace pdfinity;

class TestRedactionUi : public QObject {
    Q_OBJECT

private slots:
    void testToolbarLifecycleAndMarkCount();
    void testPreviewDialogConfirmationGate();
    void testSanitizationDialogProfiles();
    void testReportDialogLifecycle();
};

void TestRedactionUi::testToolbarLifecycleAndMarkCount() {
    ui::RedactionToolbar toolbar;

    // Initially 0 marks -> apply/clear disabled
    toolbar.UpdateMarkCount(0);
    QVERIFY(!toolbar.IsMarkModeActive());

    // Toggle mark mode
    toolbar.SetMarkModeActive(true);
    QVERIFY(toolbar.IsMarkModeActive());

    // Update to 3 marks -> actions enabled
    toolbar.UpdateMarkCount(3);
}

void TestRedactionUi::testPreviewDialogConfirmationGate() {
    std::vector<pdf_api::RedactionMark> marks;
    pdf_api::RedactionMark m;
    m.id = "m1";
    m.pageIndex = 0;
    m.rect = {50.0, 50.0, 100.0, 20.0};
    m.overlayText = "[REDACTED]";
    marks.push_back(m);

    std::vector<pdf_api::RedactionTargetAnalysis> analyses;
    pdf_api::RedactionTargetAnalysis a;
    a.pageIndex = 0;
    a.redactionRect = m.rect;
    a.intersectingTextObjects = 1;
    a.detectedTextSnippets = {"Classified Name"};
    analyses.push_back(a);

    std::filesystem::path srcPath = "C:/tmp/test_doc.pdf";
    ui::RedactionPreviewDialog dlg(marks, analyses, srcPath);

    // Initial state: not confirmed, apply button disabled
    QVERIFY(!dlg.HasConfirmedIrreversible());

    // Destination path default has _redacted.pdf
    auto dst = dlg.GetDestinationPath();
    QVERIFY(dst.string().find("_redacted.pdf") != std::string::npos);
}

void TestRedactionUi::testSanitizationDialogProfiles() {
    std::filesystem::path srcPath = "C:/tmp/sample.pdf";
    ui::SanitizationDialog dlg(srcPath);

    // Default Standard profile options
    auto opt = dlg.GetOptions();
    QVERIFY(opt.scrubInfoMetadata);
    QVERIFY(opt.removeAttachments);
    QVERIFY(opt.removeJavaScript);
    QVERIFY(!opt.removeHiddenLayers);

    auto dst = dlg.GetDestinationPath();
    QVERIFY(dst.string().find("_sanitized.pdf") != std::string::npos);
}

void TestRedactionUi::testReportDialogLifecycle() {
    pdf_api::RedactionVerificationReport report;
    report.isSuccess = true;
    report.reportTimestamp = "2026-11-14 12:00:00Z";
    report.totalRedactionsApplied = 2;
    report.pagesModified = 1;
    report.textObjectsRemoved = 2;
    report.sourceFilePath = "C:/tmp/in.pdf";
    report.destinationFilePath = "C:/tmp/out.pdf";

    ui::RedactionReportDialog dlg(report);
    QVERIFY(dlg.windowTitle().contains("Report"));
}

QTEST_MAIN(TestRedactionUi)
#include "test_redaction_ui.moc"
