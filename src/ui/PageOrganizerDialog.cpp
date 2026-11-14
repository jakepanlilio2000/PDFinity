// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PageOrganizerDialog.hpp"
#include "domain/PageRangeParser.hpp"
#include "application/Logger.hpp"
#include "security/PathSanitizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QTransform>
#include <algorithm>

namespace pdfinity::ui {

PageOrganizerDialog::PageOrganizerDialog(
    std::shared_ptr<pdf_api::IPdfEngine> engine,
    const std::filesystem::path& docPath,
    QWidget* parent
)
    : QDialog(parent),
      m_engine(std::move(engine)),
      m_docPath(docPath) {
    setWindowTitle(tr("Page Organizer - %1").arg(QString::fromStdString(docPath.filename().string())));
    resize(960, 640);
    setMinimumSize(800, 500);

    SetupUi();
    LoadDocumentPages();
}

void PageOrganizerDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // Top Toolbar: Page Operations
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(6);

    m_rotateCwBtn = new QPushButton(tr("Rotate 90° CW"), this);
    m_rotateCcwBtn = new QPushButton(tr("Rotate 90° CCW"), this);
    m_deleteBtn = new QPushButton(tr("Delete Page(s)"), this);
    m_duplicateBtn = new QPushButton(tr("Duplicate"), this);
    m_insertBlankBtn = new QPushButton(tr("Insert Blank Page"), this);
    m_moveLeftBtn = new QPushButton(tr("Move Left"), this);
    m_moveRightBtn = new QPushButton(tr("Move Right"), this);
    m_reverseBtn = new QPushButton(tr("Reverse Order"), this);
    m_extractBtn = new QPushButton(tr("Extract..."), this);

    toolbarLayout->addWidget(m_rotateCwBtn);
    toolbarLayout->addWidget(m_rotateCcwBtn);
    toolbarLayout->addWidget(m_deleteBtn);
    toolbarLayout->addWidget(m_duplicateBtn);
    toolbarLayout->addWidget(m_insertBlankBtn);
    toolbarLayout->addWidget(m_moveLeftBtn);
    toolbarLayout->addWidget(m_moveRightBtn);
    toolbarLayout->addWidget(m_reverseBtn);
    toolbarLayout->addWidget(m_extractBtn);
    toolbarLayout->addStretch();

    mainLayout->addLayout(toolbarLayout);

    // Selection Bar
    auto* selLayout = new QHBoxLayout();
    selLayout->setSpacing(6);

    auto* selLabel = new QLabel(tr("Select:"), this);
    auto* selAllBtn = new QPushButton(tr("All"), this);
    auto* selNoneBtn = new QPushButton(tr("None"), this);
    auto* selOddBtn = new QPushButton(tr("Odd Pages"), this);
    auto* selEvenBtn = new QPushButton(tr("Even Pages"), this);
    auto* selRangeBtn = new QPushButton(tr("By Range..."), this);

    selLayout->addWidget(selLabel);
    selLayout->addWidget(selAllBtn);
    selLayout->addWidget(selNoneBtn);
    selLayout->addWidget(selOddBtn);
    selLayout->addWidget(selEvenBtn);
    selLayout->addWidget(selRangeBtn);
    selLayout->addStretch();

    mainLayout->addLayout(selLayout);

    // Central Thumbnail Grid
    m_pageListWidget = new QListWidget(this);
    m_pageListWidget->setViewMode(QListView::IconMode);
    m_pageListWidget->setIconSize(QSize(130, 170));
    m_pageListWidget->setGridSize(QSize(160, 210));
    m_pageListWidget->setResizeMode(QListView::Adjust);
    m_pageListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_pageListWidget->setSpacing(10);
    m_pageListWidget->setStyleSheet(
        "QListWidget { background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px; }"
        "QListWidget::item { border: 2px solid transparent; border-radius: 6px; padding: 4px; }"
        "QListWidget::item:selected { border: 2px solid #DC2626; background-color: rgba(220, 38, 38, 0.12); }"
    );

    mainLayout->addWidget(m_pageListWidget, 1);

    // Bottom Bar: Status and Actions
    auto* bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Total: 0 pages | Selected: 0 pages"), this);
    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();

    m_saveAsBtn = new QPushButton(tr("Save As New Document..."), this);
    m_saveAsBtn->setStyleSheet("QPushButton { font-weight: bold; background-color: #DC2626; color: white; padding: 6px 14px; border-radius: 4px; } QPushButton:hover { background-color: #B91C1C; }");
    auto* closeBtn = new QPushButton(tr("Close"), this);

    bottomLayout->addWidget(m_saveAsBtn);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);

    // Connections
    connect(m_rotateCwBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnRotateCw);
    connect(m_rotateCcwBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnRotateCcw);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnDeletePages);
    connect(m_duplicateBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnDuplicatePages);
    connect(m_insertBlankBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnInsertBlankPage);
    connect(m_moveLeftBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnMoveLeft);
    connect(m_moveRightBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnMoveRight);
    connect(m_reverseBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnReverseOrder);
    connect(m_extractBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnExtractSelected);

    connect(selAllBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnSelectAll);
    connect(selNoneBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnSelectNone);
    connect(selOddBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnSelectOdd);
    connect(selEvenBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnSelectEven);
    connect(selRangeBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnSelectByRange);

    connect(m_saveAsBtn, &QPushButton::clicked, this, &PageOrganizerDialog::OnApplySaveAs);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_pageListWidget, &QListWidget::itemSelectionChanged, this, &PageOrganizerDialog::OnItemSelectionChanged);
}

void PageOrganizerDialog::LoadDocumentPages() {
    m_pages.clear();
    m_pageListWidget->clear();

    if (!m_engine) {
        QMessageBox::critical(this, tr("Error"), tr("PDF engine is not initialized"));
        return;
    }

    auto openRes = m_engine->OpenDocument(m_docPath);
    if (!openRes.has_value()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open document: %1").arg(QString::fromStdString(openRes.errorMessage)));
        return;
    }

    m_docModel = std::make_unique<domain::DocumentModel>(openRes.value, m_engine, m_docPath);
    int totalPages = m_docModel->GetPageCount();

    for (int i = 0; i < totalPages; ++i) {
        OrganizerPageItem item;
        item.originalPageIndex = i;
        item.rotationDegrees = 0;
        item.isBlank = false;

        auto sizeRes = m_docModel->GetPageSize(i);
        if (sizeRes.has_value()) {
            item.width = sizeRes.value.width;
            item.height = sizeRes.value.height;
        }

        item.baseThumbnail = RenderThumbnail(i, 0);
        m_pages.push_back(std::move(item));
    }

    RefreshListItems();
}

QPixmap PageOrganizerDialog::RenderThumbnail(int pageIndex, int rotation) {
    if (!m_docModel) return QPixmap();

    pdf_api::RenderOptions opts;
    opts.scale = 0.25;
    opts.rotationDegrees = rotation;
    opts.renderAnnotations = true;

    auto renderRes = m_docModel->RenderPage(pageIndex, opts);
    if (renderRes.has_value() && !renderRes.value.pixels.empty()) {
        const auto& rendImg = renderRes.value;
        QImage img(
            reinterpret_cast<const uchar*>(rendImg.pixels.data()),
            rendImg.width,
            rendImg.height,
            rendImg.stride,
            rendImg.isBgra ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGBA8888
        );
        return QPixmap::fromImage(img.copy());
    }

    // Fallback placeholder thumbnail
    QPixmap pix(130, 170);
    pix.fill(Qt::white);
    QPainter p(&pix);
    p.setPen(Qt::lightGray);
    p.drawRect(0, 0, 129, 169);
    p.drawText(pix.rect(), Qt::AlignCenter, QString("Page %1").arg(pageIndex + 1));
    p.end();
    return pix;
}

QPixmap PageOrganizerDialog::CreateBlankThumbnail(int rotation) {
    Q_UNUSED(rotation);
    QPixmap pix(130, 170);
    pix.fill(Qt::white);
    QPainter p(&pix);
    p.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    p.drawRect(4, 4, 121, 161);
    p.setPen(Qt::darkGray);
    p.drawText(pix.rect(), Qt::AlignCenter, tr("[Blank Page]"));
    p.end();
    return pix;
}

void PageOrganizerDialog::RefreshListItems() {
    m_pageListWidget->clear();

    for (size_t i = 0; i < m_pages.size(); ++i) {
        const auto& page = m_pages[i];

        QString labelText;
        if (page.isBlank) {
            labelText = tr("Page %1\n(Blank)").arg(i + 1);
        } else {
            labelText = tr("Page %1\n(Src %2)").arg(i + 1).arg(page.originalPageIndex + 1);
            if (page.rotationDegrees != 0) {
                labelText += QString(" [%1°]").arg(page.rotationDegrees);
            }
        }

        QPixmap currentThumbnail = page.baseThumbnail;
        if (page.rotationDegrees != 0 && !page.isBlank) {
            QTransform trans;
            trans.rotate(page.rotationDegrees);
            currentThumbnail = currentThumbnail.transformed(trans, Qt::SmoothTransformation);
        }

        auto* item = new QListWidgetItem(QIcon(currentThumbnail), labelText, m_pageListWidget);
        item->setData(Qt::UserRole, static_cast<int>(i));
        item->setTextAlignment(Qt::AlignCenter);
    }

    UpdateSelectionStatus();
}

void PageOrganizerDialog::UpdateSelectionStatus() {
    int total = static_cast<int>(m_pages.size());
    int selected = m_pageListWidget->selectedItems().count();

    m_statusLabel->setText(tr("Total: %1 pages | Selected: %2 pages").arg(total).arg(selected));

    bool hasSelection = (selected > 0);
    m_rotateCwBtn->setEnabled(hasSelection);
    m_rotateCcwBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_duplicateBtn->setEnabled(hasSelection);
    m_moveLeftBtn->setEnabled(hasSelection && selected == 1);
    m_moveRightBtn->setEnabled(hasSelection && selected == 1);
    m_extractBtn->setEnabled(hasSelection);
    m_saveAsBtn->setEnabled(total > 0);
}

void PageOrganizerDialog::OnItemSelectionChanged() {
    UpdateSelectionStatus();
}

void PageOrganizerDialog::OnRotateCw() {
    auto selectedItems = m_pageListWidget->selectedItems();
    for (auto* item : selectedItems) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
            m_pages[static_cast<size_t>(idx)].rotationDegrees = (m_pages[static_cast<size_t>(idx)].rotationDegrees + 90) % 360;
        }
    }
    RefreshListItems();
}

void PageOrganizerDialog::OnRotateCcw() {
    auto selectedItems = m_pageListWidget->selectedItems();
    for (auto* item : selectedItems) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
            int rot = m_pages[static_cast<size_t>(idx)].rotationDegrees - 90;
            if (rot < 0) rot += 360;
            m_pages[static_cast<size_t>(idx)].rotationDegrees = rot;
        }
    }
    RefreshListItems();
}

void PageOrganizerDialog::OnDeletePages() {
    auto selectedItems = m_pageListWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    if (selectedItems.size() == static_cast<int>(m_pages.size())) {
        QMessageBox::warning(this, tr("Warning"), tr("Cannot delete all pages. A PDF must have at least one page."));
        return;
    }

    std::vector<int> indicesToDelete;
    for (auto* item : selectedItems) {
        indicesToDelete.push_back(item->data(Qt::UserRole).toInt());
    }

    std::sort(indicesToDelete.rbegin(), indicesToDelete.rend());
    for (int idx : indicesToDelete) {
        if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
            m_pages.erase(m_pages.begin() + idx);
        }
    }

    RefreshListItems();
}

void PageOrganizerDialog::OnDuplicatePages() {
    auto selectedItems = m_pageListWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    std::vector<int> indices;
    for (auto* item : selectedItems) {
        indices.push_back(item->data(Qt::UserRole).toInt());
    }
    std::sort(indices.rbegin(), indices.rend());

    for (int idx : indices) {
        if (idx >= 0 && idx < static_cast<int>(m_pages.size())) {
            OrganizerPageItem copy = m_pages[static_cast<size_t>(idx)];
            m_pages.insert(m_pages.begin() + idx + 1, copy);
        }
    }

    RefreshListItems();
}

void PageOrganizerDialog::OnInsertBlankPage() {
    int insertIdx = static_cast<int>(m_pages.size());
    auto selectedItems = m_pageListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        insertIdx = selectedItems.last()->data(Qt::UserRole).toInt() + 1;
    }

    OrganizerPageItem blankItem;
    blankItem.originalPageIndex = -1;
    blankItem.rotationDegrees = 0;
    blankItem.width = 595.0; // Standard A4 points
    blankItem.height = 842.0;
    blankItem.isBlank = true;
    blankItem.baseThumbnail = CreateBlankThumbnail(0);

    if (insertIdx >= 0 && insertIdx <= static_cast<int>(m_pages.size())) {
        m_pages.insert(m_pages.begin() + insertIdx, std::move(blankItem));
    } else {
        m_pages.push_back(std::move(blankItem));
    }

    RefreshListItems();
}

void PageOrganizerDialog::OnMoveLeft() {
    auto selectedItems = m_pageListWidget->selectedItems();
    if (selectedItems.size() != 1) return;

    int idx = selectedItems.first()->data(Qt::UserRole).toInt();
    if (idx > 0 && idx < static_cast<int>(m_pages.size())) {
        std::swap(m_pages[static_cast<size_t>(idx)], m_pages[static_cast<size_t>(idx - 1)]);
        RefreshListItems();
        m_pageListWidget->setCurrentRow(idx - 1);
    }
}

void PageOrganizerDialog::OnMoveRight() {
    auto selectedItems = m_pageListWidget->selectedItems();
    if (selectedItems.size() != 1) return;

    int idx = selectedItems.first()->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx + 1 < static_cast<int>(m_pages.size())) {
        std::swap(m_pages[static_cast<size_t>(idx)], m_pages[static_cast<size_t>(idx + 1)]);
        RefreshListItems();
        m_pageListWidget->setCurrentRow(idx + 1);
    }
}

void PageOrganizerDialog::OnReverseOrder() {
    std::reverse(m_pages.begin(), m_pages.end());
    RefreshListItems();
}

void PageOrganizerDialog::OnSelectAll() {
    m_pageListWidget->selectAll();
}

void PageOrganizerDialog::OnSelectNone() {
    m_pageListWidget->clearSelection();
}

void PageOrganizerDialog::OnSelectOdd() {
    m_pageListWidget->clearSelection();
    for (int i = 0; i < m_pageListWidget->count(); i += 2) {
        m_pageListWidget->item(i)->setSelected(true);
    }
}

void PageOrganizerDialog::OnSelectEven() {
    m_pageListWidget->clearSelection();
    for (int i = 1; i < m_pageListWidget->count(); i += 2) {
        m_pageListWidget->item(i)->setSelected(true);
    }
}

void PageOrganizerDialog::OnSelectByRange() {
    bool ok = false;
    QString rangeText = QInputDialog::getText(
        this,
        tr("Select Pages by Range"),
        tr("Enter page numbers or ranges (e.g. 1-3, 5, 8-10):"),
        QLineEdit::Normal,
        "",
        &ok
    );

    if (!ok || rangeText.trimmed().isEmpty()) return;

    auto parseRes = domain::PageRangeParser::Parse(rangeText.toStdString(), static_cast<int>(m_pages.size()));
    if (!parseRes.has_value()) {
        QMessageBox::warning(this, tr("Invalid Range"), QString::fromStdString(parseRes.errorMessage));
        return;
    }

    m_pageListWidget->clearSelection();
    for (int p : parseRes.value) {
        if (p >= 0 && p < m_pageListWidget->count()) {
            m_pageListWidget->item(p)->setSelected(true);
        }
    }
}

void PageOrganizerDialog::OnExtractSelected() {
    auto selectedItems = m_pageListWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    QString defaultOut = QString::fromStdString(m_docPath.parent_path().string()) + "/extracted_pages.pdf";
    QString outPathStr = QFileDialog::getSaveFileName(
        this,
        tr("Extract Selected Pages To"),
        defaultOut,
        tr("PDF Documents (*.pdf)")
    );

    if (outPathStr.isEmpty()) return;
    std::filesystem::path outPath = outPathStr.toStdString();

    auto newDocRes = m_engine->CreateNewDocument();
    if (!newDocRes.has_value()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create new document"));
        return;
    }
    pdf_api::DocumentId newDocId = newDocRes.value;

    std::vector<int> selectedIndices;
    for (auto* itm : selectedItems) {
        selectedIndices.push_back(itm->data(Qt::UserRole).toInt());
    }
    std::sort(selectedIndices.begin(), selectedIndices.end());

    int destIdx = 0;
    for (int idx : selectedIndices) {
        const auto& page = m_pages[static_cast<size_t>(idx)];
        if (page.isBlank) {
            m_engine->InsertBlankPage(newDocId, destIdx, page.width, page.height);
        } else {
            m_engine->ImportPages(newDocId, m_docModel->GetId(), {page.originalPageIndex}, destIdx);
            if (page.rotationDegrees != 0) {
                m_engine->RotatePages(newDocId, {destIdx}, page.rotationDegrees);
            }
        }
        destIdx++;
    }

    auto saveRes = m_engine->SaveCopy(newDocId, outPath);
    m_engine->CloseDocument(newDocId);

    if (!saveRes.has_value()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save extracted pages: %1").arg(QString::fromStdString(saveRes.errorMessage)));
        return;
    }

    QMessageBox::information(this, tr("Success"), tr("Successfully extracted %1 pages to %2").arg(selectedIndices.size()).arg(QString::fromStdString(outPath.filename().string())));
}

void PageOrganizerDialog::OnApplySaveAs() {
    if (m_pages.empty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Cannot save an empty document."));
        return;
    }

    QString defaultOut = QString::fromStdString(m_docPath.parent_path().string()) + "/"
        + QString::fromStdString(m_docPath.stem().string()) + "_organized.pdf";

    QString outPathStr = QFileDialog::getSaveFileName(
        this,
        tr("Save Reorganized Document As"),
        defaultOut,
        tr("PDF Documents (*.pdf)")
    );

    if (outPathStr.isEmpty()) return;
    std::filesystem::path outPath = outPathStr.toStdString();

    auto newDocRes = m_engine->CreateNewDocument();
    if (!newDocRes.has_value()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create output document: %1").arg(QString::fromStdString(newDocRes.errorMessage)));
        return;
    }
    pdf_api::DocumentId newDocId = newDocRes.value;

    int destIdx = 0;
    for (const auto& page : m_pages) {
        if (page.isBlank) {
            m_engine->InsertBlankPage(newDocId, destIdx, page.width, page.height);
        } else {
            m_engine->ImportPages(newDocId, m_docModel->GetId(), {page.originalPageIndex}, destIdx);
            if (page.rotationDegrees != 0) {
                m_engine->RotatePages(newDocId, {destIdx}, page.rotationDegrees);
            }
        }
        destIdx++;
    }

    auto saveRes = m_engine->SaveCopy(newDocId, outPath);
    m_engine->CloseDocument(newDocId);

    if (!saveRes.has_value()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save output document: %1").arg(QString::fromStdString(saveRes.errorMessage)));
        return;
    }

    // Validate saved file integrity and page count
    auto verifyRes = m_engine->OpenDocument(outPath);
    if (!verifyRes.has_value()) {
        QMessageBox::critical(this, tr("Integrity Check Failed"), tr("Saved document failed verification check."));
        return;
    }
    int verifyPages = m_engine->GetPageCount(verifyRes.value);
    m_engine->CloseDocument(verifyRes.value);

    if (verifyPages != static_cast<int>(m_pages.size())) {
        QMessageBox::critical(this, tr("Integrity Check Failed"),
            tr("Page count mismatch: expected %1 but got %2").arg(m_pages.size()).arg(verifyPages));
        return;
    }

    m_savedOutputPath = outPath;
    QMessageBox::information(
        this,
        tr("Success"),
        tr("Successfully saved %1 pages to %2").arg(verifyPages).arg(QString::fromStdString(outPath.filename().string()))
    );

    accept();
}

} // namespace pdfinity::ui
