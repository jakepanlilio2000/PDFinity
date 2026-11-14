// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <filesystem>
#include <memory>

namespace pdfinity::ui {

class OptimizerDialog : public QDialog {
    Q_OBJECT

public:
    OptimizerDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );

    std::filesystem::path GetDestinationPath() const;
    pdf_api::OptimizationOptions GetOptions() const;

private slots:
    void onPresetChanged(int index);
    void onAnalyzeClicked();
    void onBrowseDestination();
    void onOptimizeClicked();

private:
    void setupUi();
    void applyPresetToControls(pdf_api::CompressionPreset preset);

    std::shared_ptr<domain::DocumentModel> m_doc;
    std::filesystem::path m_destinationPath;

    QComboBox* m_presetCombo{nullptr};
    QSpinBox* m_dpiSpin{nullptr};
    QSpinBox* m_qualitySpin{nullptr};
    QCheckBox* m_stripMetaCheck{nullptr};
    QCheckBox* m_removeAnnotsCheck{nullptr};
    QCheckBox* m_cleanStreamsCheck{nullptr};

    QLabel* m_analysisLabel{nullptr};
    QPushButton* m_analyzeBtn{nullptr};

    QLineEdit* m_destPathEdit{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_statusLabel{nullptr};

    QPushButton* m_optimizeBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
};

} // namespace pdfinity::ui
