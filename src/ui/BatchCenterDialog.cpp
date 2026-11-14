// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchCenterDialog.hpp"
#include "BatchProfileEditorDialog.hpp"
#include "services/BatchProcessingService.hpp"
#include "services/BatchProfileManager.hpp"
#include "services/BatchJobHistory.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QApplication>
#include <QPointer>

namespace pdfinity::ui {

namespace {

QString FormatBytes(uint64_t bytes) {
    if (bytes == 0) return "-";
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024);
    return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

} // namespace

BatchCenterDialog::BatchCenterDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Batch Processing & Automation Center"));
    resize(980, 680);
    setModal(true);

    setupUi();
    refreshProfilesCombo();

    // Default output dir to user Documents/PDFinity_Batch
    QString defaultOut = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PDFinity_Batch";
    m_outputDirEdit->setText(defaultOut);
}

BatchCenterDialog::~BatchCenterDialog() {
    if (m_isRunning) {
        services::BatchProcessingService::Instance().CancelCurrentJob();
    }
}

void BatchCenterDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // 1. Top Configuration Group
    auto* cfgGroup = new QGroupBox(tr("Workflow Profile & Output Configuration"), this);
    auto* cfgLayout = new QGridLayout(cfgGroup);

    cfgLayout->addWidget(new QLabel(tr("Workflow Profile:"), this), 0, 0);
    m_profileCombo = new QComboBox(this);
    cfgLayout->addWidget(m_profileCombo, 0, 1);
    m_editProfilesBtn = new QPushButton(tr("Edit Profiles..."), this);
    connect(m_editProfilesBtn, &QPushButton::clicked, this, &BatchCenterDialog::onEditProfiles);
    cfgLayout->addWidget(m_editProfilesBtn, 0, 2);

    cfgLayout->addWidget(new QLabel(tr("Output Directory:"), this), 1, 0);
    m_outputDirEdit = new QLineEdit(this);
    cfgLayout->addWidget(m_outputDirEdit, 1, 1);
    m_browseOutputBtn = new QPushButton(tr("Browse..."), this);
    connect(m_browseOutputBtn, &QPushButton::clicked, this, &BatchCenterDialog::onBrowseOutput);
    cfgLayout->addWidget(m_browseOutputBtn, 1, 2);

    cfgLayout->addWidget(new QLabel(tr("Filename Template:"), this), 2, 0);
    m_templateCombo = new QComboBox(this);
    m_templateCombo->setEditable(true);
    m_templateCombo->addItems({
        "{basename}_processed.pdf",
        "{basename}.pdf",
        "{basename}_{date}.pdf",
        "processed_{basename}.pdf"
    });
    cfgLayout->addWidget(m_templateCombo, 2, 1);

    auto* rulesLayout = new QHBoxLayout();
    rulesLayout->addWidget(new QLabel(tr("Collision:"), this));
    m_collisionCombo = new QComboBox(this);
    m_collisionCombo->addItem(tr("Auto-Rename (e.g. file_1.pdf)"), static_cast<int>(domain::CollisionPolicy::AutoRename));
    m_collisionCombo->addItem(tr("Overwrite Existing"), static_cast<int>(domain::CollisionPolicy::Overwrite));
    m_collisionCombo->addItem(tr("Skip Existing"), static_cast<int>(domain::CollisionPolicy::Skip));
    m_collisionCombo->addItem(tr("Fail on Existing"), static_cast<int>(domain::CollisionPolicy::Fail));
    rulesLayout->addWidget(m_collisionCombo);

    rulesLayout->addWidget(new QLabel(tr("Concurrency:"), this));
    m_concurrencySpin = new QSpinBox(this);
    m_concurrencySpin->setRange(1, 8);
    m_concurrencySpin->setValue(2);
    rulesLayout->addWidget(m_concurrencySpin);

    m_resumeCheck = new QCheckBox(tr("Safe Resume (skip valid outputs)"), this);
    m_resumeCheck->setChecked(true);
    rulesLayout->addWidget(m_resumeCheck);

    rulesLayout->addStretch();
    cfgLayout->addLayout(rulesLayout, 2, 2);

    mainLayout->addWidget(cfgGroup);

    // 2. Middle Files Section
    auto* splitter = new QSplitter(Qt::Vertical, this);

    auto* tableWidget = new QWidget(this);
    auto* tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    auto* fileBtnLayout = new QHBoxLayout();
    m_addFilesBtn = new QPushButton(tr("Add Files..."), this);
    m_addFolderBtn = new QPushButton(tr("Add Folder..."), this);
    m_removeBtn = new QPushButton(tr("Remove Selected"), this);
    m_clearBtn = new QPushButton(tr("Clear All"), this);

    connect(m_addFilesBtn, &QPushButton::clicked, this, &BatchCenterDialog::onAddFiles);
    connect(m_addFolderBtn, &QPushButton::clicked, this, &BatchCenterDialog::onAddFolder);
    connect(m_removeBtn, &QPushButton::clicked, this, &BatchCenterDialog::onRemoveSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &BatchCenterDialog::onClearAll);

    fileBtnLayout->addWidget(m_addFilesBtn);
    fileBtnLayout->addWidget(m_addFolderBtn);
    fileBtnLayout->addWidget(m_removeBtn);
    fileBtnLayout->addWidget(m_clearBtn);
    fileBtnLayout->addStretch();
    tableLayout->addLayout(fileBtnLayout);

    m_filesTable = new QTableWidget(this);
    m_filesTable->setColumnCount(7);
    m_filesTable->setHorizontalHeaderLabels({
        tr("Status"), tr("Source File"), tr("Destination File"), tr("Input Size"), tr("Output Size"), tr("Duration"), tr("Details")
    });
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_filesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_filesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_filesTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_filesTable, &QTableWidget::itemSelectionChanged, this, &BatchCenterDialog::onFileSelectionChanged);
    tableLayout->addWidget(m_filesTable);

    splitter->addWidget(tableWidget);

    // Live Log Drawer
    m_logDrawer = new QTextEdit(this);
    m_logDrawer->setReadOnly(true);
    m_logDrawer->setPlaceholderText(tr("Select a file row above to view execution logs and diagnostic events..."));
    m_logDrawer->setStyleSheet("background-color: #0f172a; color: #f8fafc; font-family: monospace; font-size: 11px;");
    splitter->addWidget(m_logDrawer);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    // 3. Bottom Progress & Controls
    auto* bottomLayout = new QVBoxLayout();

    auto* progInfoLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Ready. Add PDF files to begin batch execution."), this);
    progInfoLayout->addWidget(m_statusLabel);
    progInfoLayout->addStretch();
    bottomLayout->addLayout(progInfoLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    bottomLayout->addWidget(m_progressBar);

    auto* actionBtnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton(tr("▶ Start Batch Execution"), this);
    m_startBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 18px; border-radius: 4px;");
    m_pauseResumeBtn = new QPushButton(tr("⏸ Pause"), this);
    m_pauseResumeBtn->setEnabled(false);
    m_cancelBtn = new QPushButton(tr("⏹ Cancel"), this);
    m_cancelBtn->setEnabled(false);

    connect(m_startBtn, &QPushButton::clicked, this, &BatchCenterDialog::onStartBatch);
    connect(m_pauseResumeBtn, &QPushButton::clicked, this, &BatchCenterDialog::onPauseResume);
    connect(m_cancelBtn, &QPushButton::clicked, this, &BatchCenterDialog::onCancel);

    actionBtnLayout->addWidget(m_startBtn);
    actionBtnLayout->addWidget(m_pauseResumeBtn);
    actionBtnLayout->addWidget(m_cancelBtn);

    actionBtnLayout->addStretch();

    m_exportReportBtn = new QPushButton(tr("Export Summary Report..."), this);
    m_exportReportBtn->setEnabled(false);
    connect(m_exportReportBtn, &QPushButton::clicked, this, &BatchCenterDialog::onExportReport);
    actionBtnLayout->addWidget(m_exportReportBtn);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    actionBtnLayout->addWidget(closeBtn);

    bottomLayout->addLayout(actionBtnLayout);
    mainLayout->addLayout(bottomLayout);
}

void BatchCenterDialog::refreshProfilesCombo() {
    m_profileCombo->clear();
    auto profiles = services::BatchProfileManager::Instance().GetAllProfiles();
    for (const auto& p : profiles) {
        QString title = QString::fromStdString(p.name);
        if (p.isBuiltIn) title += " [Built-in]";
        m_profileCombo->addItem(title, QString::fromStdString(p.id));
    }
}

void BatchCenterDialog::onEditProfiles() {
    BatchProfileEditorDialog dlg(this);
    dlg.exec();
    refreshProfilesCombo();
}

void BatchCenterDialog::onBrowseOutput() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Output Destination Directory"), m_outputDirEdit->text()
    );
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
        updateTableRows();
    }
}

void BatchCenterDialog::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select PDF Files for Batch Processing"), "", tr("PDF Documents (*.pdf)")
    );
    for (const auto& f : files) {
        std::filesystem::path p = f.toStdString();
        if (std::find(m_inputFiles.begin(), m_inputFiles.end(), p) == m_inputFiles.end()) {
            m_inputFiles.push_back(p);
        }
    }
    updateTableRows();
}

void BatchCenterDialog::onAddFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Folder to Scan for PDFs")
    );
    if (dir.isEmpty()) return;

    auto collected = services::BatchProcessingService::CollectInputFiles({dir.toStdString()}, true);
    for (const auto& p : collected) {
        if (std::find(m_inputFiles.begin(), m_inputFiles.end(), p) == m_inputFiles.end()) {
            m_inputFiles.push_back(p);
        }
    }
    updateTableRows();
}

void BatchCenterDialog::onRemoveSelected() {
    auto selected = m_filesTable->selectionModel()->selectedRows();
    std::vector<int> rows;
    for (const auto& idx : selected) {
        rows.push_back(idx.row());
    }
    std::sort(rows.rbegin(), rows.rend());
    for (int r : rows) {
        if (r < static_cast<int>(m_inputFiles.size())) {
            m_inputFiles.erase(m_inputFiles.begin() + r);
        }
    }
    updateTableRows();
}

void BatchCenterDialog::onClearAll() {
    m_inputFiles.clear();
    m_lastSummary = {};
    updateTableRows();
    m_logDrawer->clear();
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Queue cleared. Add files to begin."));
    m_exportReportBtn->setEnabled(false);
}

void BatchCenterDialog::updateTableRows() {
    m_filesTable->setRowCount(static_cast<int>(m_inputFiles.size()));
    auto outDir = std::filesystem::path(m_outputDirEdit->text().toStdString());
    auto tpl = m_templateCombo->currentText().toStdString();
    auto col = static_cast<domain::CollisionPolicy>(m_collisionCombo->currentData().toInt());

    for (size_t i = 0; i < m_inputFiles.size(); ++i) {
        const auto& in = m_inputFiles[i];
        auto out = services::BatchProcessingService::ResolveOutputPath(in, outDir, tpl, col);

        auto* itemStatus = new QTableWidgetItem("Pending");
        itemStatus->setBackground(QColor(241, 245, 249));

        uint64_t inSize = std::filesystem::exists(in) ? std::filesystem::file_size(in) : 0;

        m_filesTable->setItem(static_cast<int>(i), 0, itemStatus);
        m_filesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(in.filename().string())));
        m_filesTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(out.filename().string())));
        m_filesTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(FormatBytes(inSize)));
        m_filesTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem("-"));
        m_filesTable->setItem(static_cast<int>(i), 5, new QTableWidgetItem("-"));
        m_filesTable->setItem(static_cast<int>(i), 6, new QTableWidgetItem(tr("Queued")));
    }

    m_statusLabel->setText(tr("%1 file(s) in queue.").arg(m_inputFiles.size()));
}

void BatchCenterDialog::onFileSelectionChanged() {
    int row = m_filesTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_lastSummary.fileItems.size())) {
        m_logDrawer->clear();
        return;
    }

    const auto& item = m_lastSummary.fileItems[row];
    QString logText;
    logText += QString("=== File Execution Log: %1 ===\n").arg(QString::fromStdString(item.inputPath.filename().string()));
    logText += QString("Source: %1\nDestination: %2\n").arg(QString::fromStdString(item.inputPath.string())).arg(QString::fromStdString(item.outputPath.string()));
    logText += QString("Status: %1 | Duration: %2 ms\n\n").arg(
        item.status == domain::BatchFileStatus::Success ? "SUCCESS" :
        item.status == domain::BatchFileStatus::Skipped ? "SKIPPED" :
        item.status == domain::BatchFileStatus::Error ? "ERROR" : "PROCESSING"
    ).arg(item.durationMs);

    for (const auto& l : item.logs) {
        logText += QString("[%1] [%2] %3\n").arg(QString::fromStdString(l.timestamp)).arg(QString::fromStdString(l.level)).arg(QString::fromStdString(l.message));
    }
    m_logDrawer->setPlainText(logText);
}

void BatchCenterDialog::updateControlsState(bool isRunning) {
    m_isRunning = isRunning;
    m_startBtn->setEnabled(!isRunning);
    m_addFilesBtn->setEnabled(!isRunning);
    m_addFolderBtn->setEnabled(!isRunning);
    m_removeBtn->setEnabled(!isRunning);
    m_clearBtn->setEnabled(!isRunning);
    m_profileCombo->setEnabled(!isRunning);
    m_editProfilesBtn->setEnabled(!isRunning);
    m_outputDirEdit->setEnabled(!isRunning);
    m_browseOutputBtn->setEnabled(!isRunning);
    m_templateCombo->setEnabled(!isRunning);
    m_collisionCombo->setEnabled(!isRunning);
    m_concurrencySpin->setEnabled(!isRunning);
    m_resumeCheck->setEnabled(!isRunning);

    m_pauseResumeBtn->setEnabled(isRunning);
    m_cancelBtn->setEnabled(isRunning);
}

void BatchCenterDialog::onStartBatch() {
    if (m_inputFiles.empty()) {
        QMessageBox::warning(this, tr("No Files"), tr("Please add at least one PDF file to process."));
        return;
    }

    auto profId = m_profileCombo->currentData().toString().toStdString();
    auto prof = services::BatchProfileManager::Instance().FindProfile(profId);
    if (!prof.has_value()) {
        QMessageBox::critical(this, tr("Profile Missing"), tr("Selected profile could not be found."));
        return;
    }

    QString outDirStr = m_outputDirEdit->text().trimmed();
    if (outDirStr.isEmpty()) {
        QMessageBox::warning(this, tr("Output Missing"), tr("Please specify an output directory."));
        return;
    }

    domain::BatchJobConfig config;
    config.jobName = "Batch: " + prof.value().name;
    config.profile = prof.value();
    config.inputFiles = m_inputFiles;
    config.outputDirectory = outDirStr.toStdString();
    config.filenameTemplate = m_templateCombo->currentText().toStdString();
    config.collisionPolicy = static_cast<domain::CollisionPolicy>(m_collisionCombo->currentData().toInt());
    config.maxConcurrency = m_concurrencySpin->value();
    config.resumeExistingOutputs = m_resumeCheck->isChecked();

    updateControlsState(true);
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Starting batch processing with %1 worker(s)...").arg(config.maxConcurrency));

    // Progress callback (invoked across worker threads)
    QPointer<BatchCenterDialog> self(this);
    auto progressCb = [self](int completed, int total, const domain::BatchFileItem& item) {
        if (!self) return;
        QMetaObject::invokeMethod(self.data(), [self, completed, total, item]() {
            if (!self) return;
            int pct = (total > 0) ? (completed * 100 / total) : 0;
            self->m_progressBar->setValue(pct);
            self->m_statusLabel->setText(tr("Processing file %1 of %2 (%3%)...").arg(completed).arg(total).arg(pct));

            // Find matching row and update
            for (int r = 0; r < self->m_filesTable->rowCount(); ++r) {
                if (r < static_cast<int>(self->m_inputFiles.size()) && self->m_inputFiles[r] == item.inputPath) {
                    auto* itemStatus = self->m_filesTable->item(r, 0);
                    auto* itemOutSize = self->m_filesTable->item(r, 4);
                    auto* itemDuration = self->m_filesTable->item(r, 5);
                    auto* itemDetails = self->m_filesTable->item(r, 6);

                    if (item.status == domain::BatchFileStatus::Success) {
                        itemStatus->setText(tr("Success"));
                        itemStatus->setBackground(QColor(220, 252, 231));
                        itemDetails->setText(tr("Completed"));
                    } else if (item.status == domain::BatchFileStatus::Error) {
                        itemStatus->setText(tr("Error"));
                        itemStatus->setBackground(QColor(254, 226, 226));
                        itemDetails->setText(QString::fromStdString(item.errorMessage));
                    } else if (item.status == domain::BatchFileStatus::Skipped) {
                        itemStatus->setText(tr("Skipped"));
                        itemStatus->setBackground(QColor(254, 243, 199));
                        itemDetails->setText(tr("Output already valid"));
                    } else {
                        itemStatus->setText(tr("Processing..."));
                        itemStatus->setBackground(QColor(219, 234, 254));
                    }

                    if (item.outputSizeBytes > 0) {
                        itemOutSize->setText(FormatBytes(item.outputSizeBytes));
                    }
                    if (item.durationMs > 0) {
                        itemDuration->setText(QString("%1 ms").arg(item.durationMs));
                    }
                    break;
                }
            }
        }, Qt::QueuedConnection);
    };

    // Finish callback
    auto finishCb = [self](const domain::BatchJobSummary& summary) {
        if (!self) return;
        QMetaObject::invokeMethod(self.data(), [self, summary]() {
            if (!self) return;
            self->m_lastSummary = summary;
            self->m_progressBar->setValue(100);
            self->updateControlsState(false);
            self->m_exportReportBtn->setEnabled(true);

            QString resultBanner = tr("Batch Finished in %1s: %2 Succeeded, %3 Failed, %4 Skipped.")
                .arg(summary.totalDurationMs / 1000.0, 0, 'f', 1)
                .arg(summary.succeededFiles)
                .arg(summary.failedFiles)
                .arg(summary.skippedFiles);
            self->m_statusLabel->setText(resultBanner);

            if (summary.failedFiles > 0) {
                QMessageBox::warning(self.data(), tr("Batch Completed with Errors"), resultBanner);
            } else {
                QMessageBox::information(self.data(), tr("Batch Completed"), resultBanner);
            }
        }, Qt::QueuedConnection);
    };

    services::BatchProcessingService::Instance().StartJobAsync(config, progressCb, finishCb);
}

void BatchCenterDialog::onPauseResume() {
    if (!m_isRunning) return;

    if (!m_isPaused) {
        services::BatchProcessingService::Instance().PauseCurrentJob();
        m_pauseResumeBtn->setText(tr("▶ Resume"));
        m_statusLabel->setText(tr("Batch paused. Click Resume to continue."));
        m_isPaused = true;
    } else {
        services::BatchProcessingService::Instance().ResumeCurrentJob();
        m_pauseResumeBtn->setText(tr("⏸ Pause"));
        m_statusLabel->setText(tr("Resumed batch processing..."));
        m_isPaused = false;
    }
}

void BatchCenterDialog::onCancel() {
    if (!m_isRunning) return;
    services::BatchProcessingService::Instance().CancelCurrentJob();
    m_statusLabel->setText(tr("Cancelling batch job..."));
}

void BatchCenterDialog::onExportReport() {
    if (m_lastSummary.fileItems.empty()) return;

    QString path = QFileDialog::getSaveFileName(
        this, tr("Export Batch Summary Report"),
        QString::fromStdString(m_lastSummary.jobName + "_report.html"),
        tr("HTML Report (*.html);;JSON Report (*.json)")
    );
    if (path.isEmpty()) return;

    bool ok = false;
    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        ok = services::BatchJobHistory::ExportSummaryToJson(m_lastSummary, path.toStdString());
    } else {
        ok = services::BatchJobHistory::ExportSummaryToHtml(m_lastSummary, path.toStdString());
    }

    if (ok) {
        QMessageBox::information(this, tr("Report Exported"), tr("Summary report saved to:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write summary report file."));
    }
}

} // namespace pdfinity::ui
