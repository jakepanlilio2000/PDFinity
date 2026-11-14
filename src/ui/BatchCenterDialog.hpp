// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/BatchTypes.hpp"
#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTimer>

namespace pdfinity::ui {

class BatchCenterDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchCenterDialog(QWidget* parent = nullptr);
    ~BatchCenterDialog() override;

private slots:
    void onAddFiles();
    void onAddFolder();
    void onRemoveSelected();
    void onClearAll();
    void onBrowseOutput();
    void onEditProfiles();

    void onStartBatch();
    void onPauseResume();
    void onCancel();
    void onExportReport();
    void onFileSelectionChanged();

private:
    void setupUi();
    void refreshProfilesCombo();
    void updateTableRows();
    void updateControlsState(bool isRunning);

    // Configuration
    QComboBox* m_profileCombo{nullptr};
    QPushButton* m_editProfilesBtn{nullptr};
    QLineEdit* m_outputDirEdit{nullptr};
    QPushButton* m_browseOutputBtn{nullptr};
    QComboBox* m_templateCombo{nullptr};
    QComboBox* m_collisionCombo{nullptr};
    QSpinBox* m_concurrencySpin{nullptr};
    QCheckBox* m_resumeCheck{nullptr};

    // File Management
    QPushButton* m_addFilesBtn{nullptr};
    QPushButton* m_addFolderBtn{nullptr};
    QPushButton* m_removeBtn{nullptr};
    QPushButton* m_clearBtn{nullptr};
    QTableWidget* m_filesTable{nullptr};

    // Log drawer
    QTextEdit* m_logDrawer{nullptr};

    // Progress & Control
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_statusLabel{nullptr};
    QPushButton* m_startBtn{nullptr};
    QPushButton* m_pauseResumeBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QPushButton* m_exportReportBtn{nullptr};

    std::vector<std::filesystem::path> m_inputFiles;
    domain::BatchJobSummary m_lastSummary;
    bool m_isRunning{false};
    bool m_isPaused{false};
};

} // namespace pdfinity::ui
