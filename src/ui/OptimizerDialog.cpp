// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "OptimizerDialog.hpp"
#include "services/OptimizationService.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

namespace pdfinity::ui {

namespace {

QString FormatFileSize(size_t bytes) {
    if (bytes >= 1024 * 1024) {
        return QString("%1 MB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0), 0, 'f', 2);
    }
    if (bytes >= 1024) {
        return QString("%1 KB").arg(static_cast<double>(bytes) / 1024.0, 0, 'f', 1);
    }
    return QString("%1 B").arg(bytes);
}

} // namespace

OptimizerDialog::OptimizerDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
)
    : QDialog(parent),
      m_doc(std::move(doc))
{
    setWindowTitle("PDF Optimizer & Stream Compression");
    resize(640, 520);
    setModal(true);

    setupUi();
    onPresetChanged(0); // Default to Recommended
}

void OptimizerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    auto* banner = new QLabel(
        "<b>⚡ PDF Compression & Stream Optimization:</b> Reduce document size by downsampling high-resolution "
        "images, recompressing raster streams, and purging unused metadata. Source file is never overwritten.", this
    );
    banner->setWordWrap(true);
    banner->setStyleSheet("background-color: #EFF6FF; color: #1E40AF; border: 1px solid #93C5FD; "
                          "border-radius: 6px; padding: 10px; font-size: 13px;");
    mainLayout->addWidget(banner);

    // Preset Selector
    auto* presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel("Optimization Preset:", this));
    m_presetCombo = new QComboBox(this);
    m_presetCombo->addItem("Recommended (150 DPI Standard, Clean Metadata)", static_cast<int>(pdf_api::CompressionPreset::RecommendedStandard));
    m_presetCombo->addItem("Screen & Web (96 DPI Maximum, Strip Annotations)", static_cast<int>(pdf_api::CompressionPreset::MaximumScreenWeb));
    m_presetCombo->addItem("Prepress Archival (300 DPI High-Res, Lossless)", static_cast<int>(pdf_api::CompressionPreset::PrepressLossless));
    m_presetCombo->addItem("Custom Configuration...", static_cast<int>(pdf_api::CompressionPreset::Custom));
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OptimizerDialog::onPresetChanged);
    presetLayout->addWidget(m_presetCombo, 1);
    mainLayout->addLayout(presetLayout);

    // Settings Group
    auto* settingsGroup = new QGroupBox("Image Resampling & Stream Settings", this);
    auto* setGrid = new QGridLayout(settingsGroup);

    setGrid->addWidget(new QLabel("Target Image DPI:", settingsGroup), 0, 0);
    m_dpiSpin = new QSpinBox(settingsGroup);
    m_dpiSpin->setRange(50, 600);
    m_dpiSpin->setValue(150);
    setGrid->addWidget(m_dpiSpin, 0, 1);

    setGrid->addWidget(new QLabel("JPEG Compression Quality (%):", settingsGroup), 0, 2);
    m_qualitySpin = new QSpinBox(settingsGroup);
    m_qualitySpin->setRange(30, 100);
    m_qualitySpin->setValue(80);
    setGrid->addWidget(m_qualitySpin, 0, 3);

    m_stripMetaCheck = new QCheckBox("Strip Document Info and XMP Metadata", settingsGroup);
    m_stripMetaCheck->setChecked(true);
    setGrid->addWidget(m_stripMetaCheck, 1, 0, 1, 2);

    m_removeAnnotsCheck = new QCheckBox("Remove Annotations & Comments", settingsGroup);
    m_removeAnnotsCheck->setChecked(false);
    setGrid->addWidget(m_removeAnnotsCheck, 1, 2, 1, 2);

    m_cleanStreamsCheck = new QCheckBox("Rebuild Clean Object Streams (No Incremental)", settingsGroup);
    m_cleanStreamsCheck->setChecked(true);
    m_cleanStreamsCheck->setEnabled(false); // Mandatory
    setGrid->addWidget(m_cleanStreamsCheck, 2, 0, 1, 4);

    mainLayout->addWidget(settingsGroup);

    // Analysis Group
    auto* anaGroup = new QGroupBox("Document Analysis", this);
    auto* anaLayout = new QHBoxLayout(anaGroup);

    size_t origSize = 0;
    if (m_doc && !m_doc->GetPath().empty()) {
        std::error_code ec;
        origSize = std::filesystem::file_size(m_doc->GetPath(), ec);
    }
    m_analysisLabel = new QLabel(
        QString("Original File Size: <b>%1</b> | Pages: <b>%2</b>")
            .arg(FormatFileSize(origSize))
            .arg(m_doc ? m_doc->GetPageCount() : 0),
        anaGroup
    );
    anaLayout->addWidget(m_analysisLabel, 1);

    m_analyzeBtn = new QPushButton("Analyze Potential Savings", anaGroup);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &OptimizerDialog::onAnalyzeClicked);
    anaLayout->addWidget(m_analyzeBtn);
    mainLayout->addWidget(anaGroup);

    // Destination Path
    auto* destLayout = new QHBoxLayout();
    destLayout->addWidget(new QLabel("Save Optimized Copy To:", this));

    std::filesystem::path defDst;
    if (m_doc && !m_doc->GetPath().empty()) {
        auto src = m_doc->GetPath();
        defDst = src.parent_path() / (src.stem().string() + "_optimized.pdf");
    } else {
        defDst = std::filesystem::current_path() / "document_optimized.pdf";
    }
    m_destinationPath = defDst;

    m_destPathEdit = new QLineEdit(QString::fromStdString(defDst.string()), this);
    destLayout->addWidget(m_destPathEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &OptimizerDialog::onBrowseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Progress
    m_statusLabel = new QLabel("Ready to optimize.", this);
    m_statusLabel->setStyleSheet("color: #64748B; font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_optimizeBtn = new QPushButton("Optimize & Save Copy...", this);
    m_optimizeBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 8px 18px; border-radius: 4px;");
    connect(m_optimizeBtn, &QPushButton::clicked, this, &OptimizerDialog::onOptimizeClicked);
    btnLayout->addWidget(m_optimizeBtn);

    mainLayout->addLayout(btnLayout);
}

void OptimizerDialog::applyPresetToControls(pdf_api::CompressionPreset preset) {
    auto opt = services::OptimizationService::GetOptionsForPreset(preset);
    if (m_dpiSpin) m_dpiSpin->setValue(opt.targetImageDpi);
    if (m_qualitySpin) m_qualitySpin->setValue(opt.jpegQuality);
    if (m_stripMetaCheck) m_stripMetaCheck->setChecked(opt.stripMetadata);
    if (m_removeAnnotsCheck) m_removeAnnotsCheck->setChecked(opt.removeAnnotations);
}

void OptimizerDialog::onPresetChanged(int index) {
    auto preset = static_cast<pdf_api::CompressionPreset>(m_presetCombo->itemData(index).toInt());
    if (preset != pdf_api::CompressionPreset::Custom) {
        applyPresetToControls(preset);
        m_dpiSpin->setEnabled(false);
        m_qualitySpin->setEnabled(false);
        m_stripMetaCheck->setEnabled(false);
        m_removeAnnotsCheck->setEnabled(false);
    } else {
        m_dpiSpin->setEnabled(true);
        m_qualitySpin->setEnabled(true);
        m_stripMetaCheck->setEnabled(true);
        m_removeAnnotsCheck->setEnabled(true);
    }
}

void OptimizerDialog::onAnalyzeClicked() {
    if (!m_doc || !m_doc->IsValid()) return;
    auto opt = GetOptions();
    auto rep = services::OptimizationService::Instance().AnalyzeDocument(*m_doc, m_doc->GetPath(), opt);

    m_analysisLabel->setText(
        QString("Original: <b>%1</b> | Pages: <b>%2</b> | Images detected for downsampling: <b>%3</b>")
            .arg(FormatFileSize(rep.originalSizeBytes))
            .arg(m_doc->GetPageCount())
            .arg(rep.imagesResampled)
    );
}

void OptimizerDialog::onBrowseDestination() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Optimized PDF Copy",
        m_destPathEdit ? m_destPathEdit->text() : "",
        "PDF Documents (*.pdf)"
    );
    if (!path.isEmpty()) {
        m_destPathEdit->setText(path);
        m_destinationPath = std::filesystem::path(path.toStdString());
    }
}

void OptimizerDialog::onOptimizeClicked() {
    if (!m_doc || !m_doc->IsValid()) {
        QMessageBox::critical(this, "Error", "No valid document to optimize.");
        return;
    }

    if (m_destPathEdit) {
        m_destinationPath = std::filesystem::path(m_destPathEdit->text().toStdString());
    }

    std::error_code ec;
    if (std::filesystem::equivalent(m_doc->GetPath(), m_destinationPath, ec)) {
        QMessageBox::critical(
            this, "Error: In-Place Overwrite Prohibited",
            "Please choose a separate destination path to preserve the original source document."
        );
        return;
    }

    auto opt = GetOptions();
    m_optimizeBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);

    auto progressCb = [this](int cur, int tot, const std::string& msg) {
        int pct = (tot > 0) ? (cur * 100 / tot) : 0;
        m_progressBar->setValue(pct);
        m_statusLabel->setText(QString::fromStdString(msg));
        QApplication::processEvents();
    };

    auto res = services::OptimizationService::Instance().OptimizePdf(
        *m_doc, m_doc->GetPath(), m_destinationPath, opt, progressCb
    );

    m_optimizeBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);

    if (res.has_value()) {
        const auto& report = res.value;
        size_t savedBytes = (report.originalSizeBytes > report.optimizedSizeBytes)
            ? (report.originalSizeBytes - report.optimizedSizeBytes) : 0;

        QString msg = QString(
            "<b>Optimization Completed Successfully!</b><br><br>"
            "• Original Size: <b>%1</b><br>"
            "• Optimized Size: <b>%2</b><br>"
            "• Space Saved: <b>%3 (%4%)</b><br>"
            "• Images Resampled: <b>%5</b><br>"
            "• Output Integrity: <b>Verified (Reopened & Parsed)</b><br><br>"
            "Output saved to:<br><code>%6</code>"
        )
            .arg(FormatFileSize(report.originalSizeBytes))
            .arg(FormatFileSize(report.optimizedSizeBytes))
            .arg(FormatFileSize(savedBytes))
            .arg(report.compressionRatio, 0, 'f', 1)
            .arg(report.imagesResampled)
            .arg(QString::fromStdString(m_destinationPath.string()));

        QMessageBox::information(this, "Optimization Complete", msg);
        accept();
    } else {
        QMessageBox::critical(this, "Optimization Failed", QString::fromStdString(res.errorMessage));
    }
}

std::filesystem::path OptimizerDialog::GetDestinationPath() const {
    return m_destinationPath;
}

pdf_api::OptimizationOptions OptimizerDialog::GetOptions() const {
    pdf_api::OptimizationOptions opt;
    opt.preset = static_cast<pdf_api::CompressionPreset>(m_presetCombo->currentData().toInt());
    opt.targetImageDpi = m_dpiSpin ? m_dpiSpin->value() : 150;
    opt.jpegQuality = m_qualitySpin ? m_qualitySpin->value() : 80;
    opt.stripMetadata = m_stripMetaCheck ? m_stripMetaCheck->isChecked() : true;
    opt.removeAnnotations = m_removeAnnotsCheck ? m_removeAnnotsCheck->isChecked() : false;
    opt.cleanUnusedStreams = true;
    return opt;
}

} // namespace pdfinity::ui
