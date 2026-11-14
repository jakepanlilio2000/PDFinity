// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <filesystem>
#include <memory>

namespace pdfinity::ui {

class OcrStudioDialog : public QDialog {
    Q_OBJECT

public:
    OcrStudioDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );

    std::filesystem::path GetDestinationPath() const;
    pdf_api::OcrOptions GetOptions() const;

private slots:
    void onBrowseDestination();
    void onStartOcr();

private:
    void setupUi();
    void updateEngineStatus();

    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_destinationPath;

    QLabel* m_statusBanner{nullptr};
    QComboBox* m_langCombo{nullptr};
    QComboBox* m_pageRangeCombo{nullptr};
    QLineEdit* m_customRangeEdit{nullptr};
    QComboBox* m_dpiCombo{nullptr};
    QCheckBox* m_preserveImageCheck{nullptr};
    QLineEdit* m_destPathEdit{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_progressLabel{nullptr};
    QPushButton* m_startBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
};

} // namespace pdfinity::ui
