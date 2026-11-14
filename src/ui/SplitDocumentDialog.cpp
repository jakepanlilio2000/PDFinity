// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SplitDocumentDialog.hpp"
#include "application/Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <sstream>
#include <algorithm>

namespace pdfinity::ui {

SplitDocumentDialog::SplitDocumentDialog(
    std::shared_ptr<pdf_api::IPdfEngine> engine,
    const std::filesystem::path& docPath,
    QWidget* parent
)
    : QDialog(parent),
      m_engine(std::move(engine)),
      m_sourcePath(docPath) {
    if (m_engine) {
        m_assemblyService = std::make_unique<services::DocumentAssemblyService>(m_engine);
    }
    setWindowTitle(tr("Split PDF Document"));
    resize(580, 440);
    setMinimumSize(500, 380);

    SetupUi();
    if (!m_sourcePath.empty()) {
        m_sourcePathEdit->setText(QString::fromStdString(m_sourcePath.string()));
        UpdateSourceDocInfo();
    }
}

void SplitDocumentDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Source Document Section
    auto* srcGroup = new QGroupBox(tr("Source Document"), this);
    auto* srcLayout = new QVBoxLayout(srcGroup);

    auto* srcRow = new QHBoxLayout();
    m_sourcePathEdit = new QLineEdit(this);
    m_sourcePathEdit->setReadOnly(true);
    auto* browseSrcBtn = new QPushButton(tr("Browse..."), this);
    srcRow->addWidget(m_sourcePathEdit, 1);
    srcRow->addWidget(browseSrcBtn);
    srcLayout->addLayout(srcRow);

    m_sourceInfoLabel = new QLabel(tr("No document selected"), this);
    m_sourceInfoLabel->setStyleSheet("color: #475569; font-size: 11px;");
    srcLayout->addWidget(m_sourceInfoLabel);

    mainLayout->addWidget(srcGroup);

    // Split Options Section
    auto* optionsGroup = new QGroupBox(tr("Split Method"), this);
    auto* optLayout = new QVBoxLayout(optionsGroup);

    m_burstRadio = new QRadioButton(tr("Burst into individual single-page files (1 page per file)"), this);
    m_burstRadio->setChecked(true);
    optLayout->addWidget(m_burstRadio);

    auto* chunkRow = new QHBoxLayout();
    m_everyNRadio = new QRadioButton(tr("Split into chunks of every:"), this);
    m_pagesPerChunkSpin = new QSpinBox(this);
    m_pagesPerChunkSpin->setRange(1, 1000);
    m_pagesPerChunkSpin->setValue(2);
    m_pagesPerChunkSpin->setEnabled(false);
    auto* pagesSuffix = new QLabel(tr("pages"), this);
    chunkRow->addWidget(m_everyNRadio);
    chunkRow->addWidget(m_pagesPerChunkSpin);
    chunkRow->addWidget(pagesSuffix);
    chunkRow->addStretch();
    optLayout->addLayout(chunkRow);

    auto* rangeRow = new QHBoxLayout();
    m_customRangesRadio = new QRadioButton(tr("By custom page ranges:"), this);
    m_customRangesEdit = new QLineEdit(this);
    m_customRangesEdit->setPlaceholderText(tr("e.g. 1-3, 4-7, 8-10"));
    m_customRangesEdit->setEnabled(false);
    rangeRow->addWidget(m_customRangesRadio);
    rangeRow->addWidget(m_customRangesEdit, 1);
    optLayout->addLayout(rangeRow);

    m_bookmarksRadio = new QRadioButton(tr("Split by top-level bookmarks / chapters"), this);
    m_bookmarksRadio->setEnabled(false);
    optLayout->addWidget(m_bookmarksRadio);

    mainLayout->addWidget(optionsGroup);

    // Destination Section
    auto* destGroup = new QGroupBox(tr("Output Settings"), this);
    auto* destLayout = new QGridLayout(destGroup);

    destLayout->addWidget(new QLabel(tr("Output Folder:"), this), 0, 0);
    m_outputDirEdit = new QLineEdit(this);
    auto* browseDirBtn = new QPushButton(tr("Browse..."), this);
    destLayout->addWidget(m_outputDirEdit, 0, 1);
    destLayout->addWidget(browseDirBtn, 0, 2);

    destLayout->addWidget(new QLabel(tr("Filename Prefix:"), this), 1, 0);
    m_prefixEdit = new QLineEdit("split_", this);
    destLayout->addWidget(m_prefixEdit, 1, 1);

    mainLayout->addWidget(destGroup);

    // Bottom Action Buttons
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    m_splitBtn = new QPushButton(tr("Split Document"), this);
    m_splitBtn->setStyleSheet("QPushButton { font-weight: bold; background-color: #DC2626; color: white; padding: 6px 14px; border-radius: 4px; } QPushButton:hover { background-color: #B91C1C; }");
    m_splitBtn->setEnabled(false);
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);

    bottomLayout->addWidget(m_splitBtn);
    bottomLayout->addWidget(cancelBtn);
    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(browseSrcBtn, &QPushButton::clicked, this, &SplitDocumentDialog::OnBrowseSource);
    connect(browseDirBtn, &QPushButton::clicked, this, &SplitDocumentDialog::OnBrowseOutputFolder);

    connect(m_burstRadio, &QRadioButton::toggled, this, &SplitDocumentDialog::OnSplitModeChanged);
    connect(m_everyNRadio, &QRadioButton::toggled, this, &SplitDocumentDialog::OnSplitModeChanged);
    connect(m_customRangesRadio, &QRadioButton::toggled, this, &SplitDocumentDialog::OnSplitModeChanged);
    connect(m_bookmarksRadio, &QRadioButton::toggled, this, &SplitDocumentDialog::OnSplitModeChanged);

    connect(m_splitBtn, &QPushButton::clicked, this, &SplitDocumentDialog::OnSplitClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SplitDocumentDialog::UpdateSourceDocInfo() {
    m_totalPages = 0;
    m_hasBookmarks = false;

    if (!m_engine || m_sourcePath.empty()) {
        m_sourceInfoLabel->setText(tr("No valid document selected"));
        m_splitBtn->setEnabled(false);
        return;
    }

    auto openRes = m_engine->OpenDocument(m_sourcePath);
    if (!openRes.has_value()) {
        m_sourceInfoLabel->setText(tr("Failed to inspect document"));
        m_splitBtn->setEnabled(false);
        return;
    }

    m_totalPages = m_engine->GetPageCount(openRes.value);
    auto bmRes = m_engine->GetBookmarks(openRes.value);
    m_hasBookmarks = (bmRes.has_value() && !bmRes.value.empty());
    m_engine->CloseDocument(openRes.value);

    m_sourceInfoLabel->setText(
        tr("Total pages: %1 | Outline bookmarks: %2")
            .arg(m_totalPages)
            .arg(m_hasBookmarks ? tr("Yes") : tr("None"))
    );

    m_bookmarksRadio->setEnabled(m_hasBookmarks);
    m_pagesPerChunkSpin->setMaximum(std::max(1, m_totalPages));

    if (m_outputDirEdit->text().isEmpty()) {
        m_outputDirEdit->setText(QString::fromStdString(m_sourcePath.parent_path().string()));
    }
    m_prefixEdit->setText(QString::fromStdString(m_sourcePath.stem().string()) + "_part_");

    m_splitBtn->setEnabled(m_totalPages > 0);
}

void SplitDocumentDialog::OnBrowseSource() {
    QString file = QFileDialog::getOpenFileName(
        this,
        tr("Select PDF Document to Split"),
        QString(),
        tr("PDF Documents (*.pdf)")
    );

    if (!file.isEmpty()) {
        m_sourcePath = file.toStdString();
        m_sourcePathEdit->setText(file);
        UpdateSourceDocInfo();
    }
}

void SplitDocumentDialog::OnBrowseOutputFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Output Folder"),
        m_outputDirEdit->text().isEmpty() ? QString() : m_outputDirEdit->text()
    );

    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
    }
}

void SplitDocumentDialog::OnSplitModeChanged() {
    m_pagesPerChunkSpin->setEnabled(m_everyNRadio->isChecked());
    m_customRangesEdit->setEnabled(m_customRangesRadio->isChecked());
}

void SplitDocumentDialog::OnSplitClicked() {
    if (!m_assemblyService) {
        QMessageBox::critical(this, tr("Error"), tr("Document assembly service is unavailable."));
        return;
    }

    std::filesystem::path outDir = m_outputDirEdit->text().trimmed().toStdString();
    if (outDir.empty() || !std::filesystem::exists(outDir)) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a valid output folder."));
        return;
    }

    services::SplitOptions options;
    options.filePrefix = m_prefixEdit->text().trimmed().toStdString();
    if (options.filePrefix.empty()) options.filePrefix = "split_";

    if (m_burstRadio->isChecked()) {
        options.mode = services::SplitMode::IndividualPages;
    } else if (m_everyNRadio->isChecked()) {
        options.mode = services::SplitMode::EveryNPages;
        options.pagesPerChunk = m_pagesPerChunkSpin->value();
    } else if (m_customRangesRadio->isChecked()) {
        options.mode = services::SplitMode::ByPageRanges;
        std::string rawRanges = m_customRangesEdit->text().toStdString();
        std::stringstream ss(rawRanges);
        std::string token;
        while (std::getline(ss, token, ',')) {
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                options.customRanges.push_back(token.substr(start, end - start + 1));
            }
        }
        if (options.customRanges.empty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please specify at least one page range (e.g. 1-3, 4-6)."));
            return;
        }
    } else if (m_bookmarksRadio->isChecked()) {
        options.mode = services::SplitMode::ByBookmarks;
    }

    auto splitRes = m_assemblyService->SplitDocument(m_sourcePath, outDir, options);
    if (!splitRes.has_value()) {
        QMessageBox::critical(this, tr("Split Failed"), QString::fromStdString(splitRes.errorMessage));
        return;
    }

    m_generatedFiles = splitRes.value.generatedFiles;
    QMessageBox::information(
        this,
        tr("Split Complete"),
        tr("Successfully split document into %1 files (%2 pages processed).")
            .arg(m_generatedFiles.size())
            .arg(splitRes.value.totalPagesProcessed)
    );

    accept();
}

} // namespace pdfinity::ui
