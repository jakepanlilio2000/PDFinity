// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <filesystem>

namespace pdfinity::ui {

class SignatureValidationDialog : public QDialog {
    Q_OBJECT

public:
    SignatureValidationDialog(
        const std::filesystem::path& pdfPath,
        const std::vector<pdf_api::SignatureValidationResult>& results,
        QWidget* parent = nullptr
    );
    ~SignatureValidationDialog() override = default;

private slots:
    void OnSignatureSelected(int index);
    void OnExportReport();

private:
    void SetupUi();
    void PopulateSignatures();

    std::filesystem::path m_pdfPath;
    std::vector<pdf_api::SignatureValidationResult> m_results;

    QListWidget* m_sigList{nullptr};
    QTextBrowser* m_detailsBrowser{nullptr};
    QPushButton* m_exportReportBtn{nullptr};
};

} // namespace pdfinity::ui
