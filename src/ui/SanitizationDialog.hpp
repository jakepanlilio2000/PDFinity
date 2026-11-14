// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <filesystem>

namespace pdfinity::ui {

class SanitizationDialog : public QDialog {
    Q_OBJECT

public:
    explicit SanitizationDialog(
        const std::filesystem::path& sourcePath,
        QWidget* parent = nullptr
    );

    pdf_api::SanitizationOptions GetOptions() const;
    std::filesystem::path GetDestinationPath() const;

private slots:
    void onProfileChanged(int index);
    void onBrowseDestination();
    void onSanitizeClicked();

private:
    void setupUi(const std::filesystem::path& sourcePath);

    QComboBox* m_profileCombo{nullptr};
    QCheckBox* m_infoMetaCheck{nullptr};
    QCheckBox* m_xmpMetaCheck{nullptr};
    QCheckBox* m_attachmentsCheck{nullptr};
    QCheckBox* m_jsCheck{nullptr};
    QCheckBox* m_hiddenLayersCheck{nullptr};
    QCheckBox* m_annotationsCheck{nullptr};

    QLineEdit* m_destPathEdit{nullptr};
    QPushButton* m_sanitizeBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};

    std::filesystem::path m_sourcePath;
    std::filesystem::path m_destinationPath;
};

} // namespace pdfinity::ui
