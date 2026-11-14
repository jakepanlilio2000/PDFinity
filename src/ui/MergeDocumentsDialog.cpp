// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "MergeDocumentsDialog.hpp"
#include "application/Logger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

namespace pdfinity::ui {

MergeDocumentsDialog::MergeDocumentsDialog(
    std::shared_ptr<pdf_api::IPdfEngine> engine,
    QWidget* parent
)
    : QDialog(parent),
      m_engine(std::move(engine)) {
    if (m_engine) {
        m_assemblyService = std::make_unique<services::DocumentAssemblyService>(m_engine);
    }
    setWindowTitle(tr("Merge PDF Documents"));
    resize(720, 480);
    setMinimumSize(600, 380);

    SetupUi();
}

void MergeDocumentsDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    auto* descLabel = new QLabel(tr("Add PDF documents and specify the order and page ranges to merge into a single file."), this);
    descLabel->setStyleSheet("color: #475569;");
    mainLayout->addWidget(descLabel);

    // Table of source documents
    m_fileTable = new QTableWidget(0, 4, this);
    m_fileTable->setHorizontalHeaderLabels({tr("File Name"), tr("Pages"), tr("Page Range (e.g. all, 1-3)"), tr("Full Path")});
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileTable->setAlternatingRowColors(true);

    mainLayout->addWidget(m_fileTable, 1);

    // Side/Bottom Buttons for table control
    auto* tableBtnsLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add Files..."), this);
    m_removeBtn = new QPushButton(tr("Remove"), this);
    m_moveUpBtn = new QPushButton(tr("Move Up"), this);
    m_moveDownBtn = new QPushButton(tr("Move Down"), this);
    auto* clearBtn = new QPushButton(tr("Clear All"), this);

    tableBtnsLayout->addWidget(addBtn);
    tableBtnsLayout->addWidget(m_removeBtn);
    tableBtnsLayout->addWidget(m_moveUpBtn);
    tableBtnsLayout->addWidget(m_moveDownBtn);
    tableBtnsLayout->addWidget(clearBtn);
    tableBtnsLayout->addStretch();

    mainLayout->addLayout(tableBtnsLayout);

    // Output Path Section
    auto* outLayout = new QHBoxLayout();
    outLayout->addWidget(new QLabel(tr("Output File:"), this));
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText(tr("Choose output PDF destination..."));
    auto* browseBtn = new QPushButton(tr("Browse..."), this);

    outLayout->addWidget(m_outputPathEdit, 1);
    outLayout->addWidget(browseBtn);
    mainLayout->addLayout(outLayout);

    // Bottom Action Buttons
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    m_mergeBtn = new QPushButton(tr("Merge Documents"), this);
    m_mergeBtn->setStyleSheet("QPushButton { font-weight: bold; background-color: #DC2626; color: white; padding: 6px 14px; border-radius: 4px; } QPushButton:hover { background-color: #B91C1C; }");
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);

    bottomLayout->addWidget(m_mergeBtn);
    bottomLayout->addWidget(cancelBtn);
    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(addBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnAddFiles);
    connect(m_removeBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnRemoveSelected);
    connect(m_moveUpBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnMoveUp);
    connect(m_moveDownBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnMoveDown);
    connect(clearBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnClearAll);
    connect(browseBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnBrowseOutput);
    connect(m_mergeBtn, &QPushButton::clicked, this, &MergeDocumentsDialog::OnMergeClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_fileTable, &QTableWidget::itemSelectionChanged, this, &MergeDocumentsDialog::OnSelectionChanged);

    UpdateControlsState();
}

int MergeDocumentsDialog::GetPdfPageCount(const std::filesystem::path& path) {
    if (!m_engine) return 0;
    auto res = m_engine->OpenDocument(path);
    if (!res.has_value()) return 0;
    int count = m_engine->GetPageCount(res.value);
    m_engine->CloseDocument(res.value);
    return count;
}

void MergeDocumentsDialog::OnAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Select PDF Documents to Merge"),
        QString(),
        tr("PDF Documents (*.pdf)")
    );

    if (files.isEmpty()) return;

    for (const auto& f : files) {
        std::filesystem::path path = f.toStdString();
        int pages = GetPdfPageCount(path);
        if (pages <= 0) continue;

        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);

        auto* nameItem = new QTableWidgetItem(QString::fromStdString(path.filename().string()));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

        auto* pagesItem = new QTableWidgetItem(QString::number(pages));
        pagesItem->setFlags(pagesItem->flags() & ~Qt::ItemIsEditable);
        pagesItem->setTextAlignment(Qt::AlignCenter);

        auto* rangeItem = new QTableWidgetItem("all");

        auto* pathItem = new QTableWidgetItem(QString::fromStdString(path.string()));
        pathItem->setFlags(pathItem->flags() & ~Qt::ItemIsEditable);

        m_fileTable->setItem(row, 0, nameItem);
        m_fileTable->setItem(row, 1, pagesItem);
        m_fileTable->setItem(row, 2, rangeItem);
        m_fileTable->setItem(row, 3, pathItem);
    }

    if (m_outputPathEdit->text().isEmpty() && !files.isEmpty()) {
        std::filesystem::path first = files.first().toStdString();
        m_outputPathEdit->setText(QString::fromStdString((first.parent_path() / "merged_document.pdf").string()));
    }

    UpdateControlsState();
}

void MergeDocumentsDialog::OnRemoveSelected() {
    int curRow = m_fileTable->currentRow();
    if (curRow >= 0) {
        m_fileTable->removeRow(curRow);
    }
    UpdateControlsState();
}

void MergeDocumentsDialog::OnMoveUp() {
    int curRow = m_fileTable->currentRow();
    if (curRow > 0) {
        auto* nameItem = m_fileTable->takeItem(curRow, 0);
        auto* pagesItem = m_fileTable->takeItem(curRow, 1);
        auto* rangeItem = m_fileTable->takeItem(curRow, 2);
        auto* pathItem = m_fileTable->takeItem(curRow, 3);

        m_fileTable->removeRow(curRow);
        m_fileTable->insertRow(curRow - 1);

        m_fileTable->setItem(curRow - 1, 0, nameItem);
        m_fileTable->setItem(curRow - 1, 1, pagesItem);
        m_fileTable->setItem(curRow - 1, 2, rangeItem);
        m_fileTable->setItem(curRow - 1, 3, pathItem);

        m_fileTable->selectRow(curRow - 1);
    }
    UpdateControlsState();
}

void MergeDocumentsDialog::OnMoveDown() {
    int curRow = m_fileTable->currentRow();
    if (curRow >= 0 && curRow + 1 < m_fileTable->rowCount()) {
        auto* nameItem = m_fileTable->takeItem(curRow, 0);
        auto* pagesItem = m_fileTable->takeItem(curRow, 1);
        auto* rangeItem = m_fileTable->takeItem(curRow, 2);
        auto* pathItem = m_fileTable->takeItem(curRow, 3);

        m_fileTable->removeRow(curRow);
        m_fileTable->insertRow(curRow + 1);

        m_fileTable->setItem(curRow + 1, 0, nameItem);
        m_fileTable->setItem(curRow + 1, 1, pagesItem);
        m_fileTable->setItem(curRow + 1, 2, rangeItem);
        m_fileTable->setItem(curRow + 1, 3, pathItem);

        m_fileTable->selectRow(curRow + 1);
    }
    UpdateControlsState();
}

void MergeDocumentsDialog::OnClearAll() {
    m_fileTable->setRowCount(0);
    UpdateControlsState();
}

void MergeDocumentsDialog::OnBrowseOutput() {
    QString out = QFileDialog::getSaveFileName(
        this,
        tr("Select Merged PDF Destination"),
        m_outputPathEdit->text().isEmpty() ? QString() : m_outputPathEdit->text(),
        tr("PDF Documents (*.pdf)")
    );

    if (!out.isEmpty()) {
        m_outputPathEdit->setText(out);
        UpdateControlsState();
    }
}

void MergeDocumentsDialog::OnSelectionChanged() {
    UpdateControlsState();
}

void MergeDocumentsDialog::UpdateControlsState() {
    int count = m_fileTable->rowCount();
    int cur = m_fileTable->currentRow();

    m_removeBtn->setEnabled(cur >= 0);
    m_moveUpBtn->setEnabled(cur > 0);
    m_moveDownBtn->setEnabled(cur >= 0 && cur + 1 < count);
    m_mergeBtn->setEnabled(count >= 1 && !m_outputPathEdit->text().trimmed().isEmpty());
}

void MergeDocumentsDialog::OnMergeClicked() {
    if (!m_assemblyService) {
        QMessageBox::critical(this, tr("Error"), tr("Document assembly service is unavailable."));
        return;
    }

    int count = m_fileTable->rowCount();
    if (count == 0) {
        QMessageBox::warning(this, tr("Warning"), tr("Please add at least one document to merge."));
        return;
    }

    std::filesystem::path outputPath = m_outputPathEdit->text().trimmed().toStdString();
    if (outputPath.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please specify an output file path."));
        return;
    }

    std::vector<services::MergeSource> sources;
    for (int r = 0; r < count; ++r) {
        services::MergeSource src;
        src.path = m_fileTable->item(r, 3)->text().toStdString();
        src.pageRange = m_fileTable->item(r, 2)->text().trimmed().toStdString();
        if (src.pageRange.empty()) src.pageRange = "all";
        sources.push_back(std::move(src));
    }

    auto mergeRes = m_assemblyService->MergeDocuments(sources, outputPath);
    if (!mergeRes.has_value()) {
        QMessageBox::critical(this, tr("Merge Failed"), QString::fromStdString(mergeRes.errorMessage));
        return;
    }

    m_savedOutputPath = outputPath;
    QMessageBox::information(
        this,
        tr("Merge Successful"),
        tr("Successfully merged %1 documents into %2 (%3 pages total).")
            .arg(sources.size())
            .arg(QString::fromStdString(outputPath.filename().string()))
            .arg(mergeRes.value.totalPagesMerged)
    );

    accept();
}

} // namespace pdfinity::ui
