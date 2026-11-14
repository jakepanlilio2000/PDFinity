// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RedactionReportDialog.hpp"
#include "services/RedactionService.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

namespace pdfinity::ui {

RedactionReportDialog::RedactionReportDialog(
    const pdf_api::RedactionVerificationReport& report,
    QWidget* parent
)
    : QDialog(parent),
      m_report(report)
{
    setWindowTitle("Redaction & Sanitization Verification Report");
    resize(680, 500);
    setModal(true);

    m_reportMarkdown = services::RedactionService::GenerateVerificationReport(m_report);
    setupUi();
}

void RedactionReportDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setMarkdown(QString::fromStdString(m_reportMarkdown));
    mainLayout->addWidget(m_textEdit);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_exportBtn = new QPushButton("Export Report...", this);
    connect(m_exportBtn, &QPushButton::clicked, this, &RedactionReportDialog::onExportReport);
    btnLayout->addWidget(m_exportBtn);

    m_closeBtn = new QPushButton("Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);
}

void RedactionReportDialog::onExportReport() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export Verification Report", "redaction_report.md",
        "Markdown Files (*.md);;Text Files (*.txt);;All Files (*.*)"
    );
    if (!path.isEmpty()) {
        std::ofstream out(path.toStdString(), std::ios::binary);
        if (out.is_open()) {
            out.write(m_reportMarkdown.data(), m_reportMarkdown.size());
            out.close();
            QMessageBox::information(this, "Report Exported", "Verification report saved successfully to:\n" + path);
        } else {
            QMessageBox::critical(this, "Export Error", "Failed to write report to specified file path.");
        }
    }
}

} // namespace pdfinity::ui
