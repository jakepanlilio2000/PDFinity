// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QTableWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <vector>
#include <filesystem>

namespace pdfinity::ui {

class RedactionPreviewDialog : public QDialog {
    Q_OBJECT

public:
    explicit RedactionPreviewDialog(
        const std::vector<pdf_api::RedactionMark>& marks,
        const std::vector<pdf_api::RedactionTargetAnalysis>& analyses,
        const std::filesystem::path& defaultSourcePath,
        QWidget* parent = nullptr
    );

    std::filesystem::path GetDestinationPath() const;
    bool HasConfirmedIrreversible() const;

private slots:
    void onBrowseDestination();
    void onConfirmToggled(bool checked);
    void onApplyClicked();

private:
    void setupUi(
        const std::vector<pdf_api::RedactionMark>& marks,
        const std::vector<pdf_api::RedactionTargetAnalysis>& analyses,
        const std::filesystem::path& defaultSourcePath
    );

    QTableWidget* m_table{nullptr};
    QLabel* m_warningBanner{nullptr};
    QLineEdit* m_destPathEdit{nullptr};
    QCheckBox* m_confirmCheckbox{nullptr};
    QPushButton* m_applyBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    std::filesystem::path m_sourcePath;
    std::filesystem::path m_destinationPath;
};

} // namespace pdfinity::ui
