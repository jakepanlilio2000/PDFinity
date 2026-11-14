// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QTextEdit>
#include <QPushButton>

namespace pdfinity::ui {

class RedactionReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit RedactionReportDialog(
        const pdf_api::RedactionVerificationReport& report,
        QWidget* parent = nullptr
    );

private slots:
    void onExportReport();

private:
    void setupUi();

    pdf_api::RedactionVerificationReport m_report;
    std::string m_reportMarkdown;
    QTextEdit* m_textEdit{nullptr};
    QPushButton* m_exportBtn{nullptr};
    QPushButton* m_closeBtn{nullptr};
};

} // namespace pdfinity::ui
