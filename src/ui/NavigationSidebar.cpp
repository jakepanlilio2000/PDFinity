// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "NavigationSidebar.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QPainter>

namespace pdfinity::ui {

NavigationSidebar::NavigationSidebar(
    std::shared_ptr<domain::DocumentModel> doc,
    services::AsyncRenderService* renderService,
    QWidget* parent
) : QWidget(parent),
    m_doc(std::move(doc)),
    m_renderService(renderService) {

    setMinimumWidth(220);
    setMaximumWidth(380);
    SetupUi();
}

void NavigationSidebar::SetupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_tabWidget = new QTabWidget(this);

    // Tab 1: Thumbnails
    m_thumbnailList = new QListWidget(this);
    m_thumbnailList->setViewMode(QListView::IconMode);
    m_thumbnailList->setIconSize(QSize(120, 160));
    m_thumbnailList->setGridSize(QSize(140, 190));
    m_thumbnailList->setMovement(QListView::Static);
    m_thumbnailList->setResizeMode(QListView::Adjust);
    m_thumbnailList->setWordWrap(true);
    connect(m_thumbnailList, &QListWidget::itemClicked, this, &NavigationSidebar::OnThumbnailClicked);
    m_tabWidget->addTab(m_thumbnailList, tr("Thumbnails"));

    // Tab 2: Bookmarks
    m_bookmarkTree = new QTreeWidget(this);
    m_bookmarkTree->header()->setVisible(false);
    connect(m_bookmarkTree, &QTreeWidget::itemClicked, this, &NavigationSidebar::OnBookmarkClicked);
    m_tabWidget->addTab(m_bookmarkTree, tr("Outlines"));

    // Tab 3: Annotations
    m_annotationTree = new QTreeWidget(this);
    m_annotationTree->setHeaderLabels({tr("Annotation"), tr("Page")});
    m_annotationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_annotationTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(m_annotationTree, &QTreeWidget::itemClicked, this, &NavigationSidebar::OnAnnotationItemClicked);
    m_tabWidget->addTab(m_annotationTree, tr("Annotations"));

    layout->addWidget(m_tabWidget);

    if (m_renderService) {
        connect(m_renderService, &services::AsyncRenderService::PageRendered,
                this, &NavigationSidebar::OnThumbnailRendered);
    }

    PopulateThumbnails();
    PopulateBookmarks();
    PopulateAnnotations();
}

void NavigationSidebar::PopulateThumbnails() {
    m_thumbnailList->clear();
    if (!m_doc || !m_doc->IsValid()) return;

    int pageCount = m_doc->GetPageCount();
    for (int i = 0; i < pageCount; ++i) {
        auto* item = new QListWidgetItem(tr("Page %1").arg(i + 1), m_thumbnailList);
        item->setData(Qt::UserRole, i);
        item->setTextAlignment(Qt::AlignCenter);

        // Blank initial thumbnail icon
        QPixmap pix(120, 160);
        pix.fill(Qt::white);
        QPainter p(&pix);
        p.setPen(Qt::lightGray);
        p.drawRect(0, 0, 119, 159);
        p.drawText(pix.rect(), Qt::AlignCenter, QString::number(i + 1));
        p.end();

        item->setIcon(QIcon(pix));

        // Request low-res thumbnail render
        if (m_renderService) {
            m_renderService->RequestPageRender(m_doc, i, 0.20, 1.0, -10);
        }
    }
}

void NavigationSidebar::PopulateBookmarks() {
    m_bookmarkTree->clear();
    if (!m_doc || !m_doc->IsValid()) return;

    auto res = m_doc->GetBookmarks();
    if (!res.has_value() || res.value.empty()) {
        auto* emptyItem = new QTreeWidgetItem(m_bookmarkTree);
        emptyItem->setText(0, tr("(No document outline)"));
        emptyItem->setFlags(Qt::NoItemFlags);
        return;
    }

    AddBookmarkNodes(nullptr, res.value);
    m_bookmarkTree->expandAll();
}

void NavigationSidebar::AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks) {
    for (const auto& bm : bookmarks) {
        QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_bookmarkTree);
        item->setText(0, QString::fromStdString(bm.title));
        item->setData(0, Qt::UserRole, bm.targetPageIndex);
        if (bm.targetPageIndex >= 0) {
            item->setToolTip(0, tr("Jump to Page %1").arg(bm.targetPageIndex + 1));
        }

        if (!bm.children.empty()) {
            AddBookmarkNodes(item, bm.children);
        }
    }
}

void NavigationSidebar::SetActivePage(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_thumbnailList->count()) return;
    m_thumbnailList->setCurrentRow(pageIndex);
}

void NavigationSidebar::OnThumbnailClicked(QListWidgetItem* item) {
    if (!item) return;
    int page = item->data(Qt::UserRole).toInt();
    emit PageSelected(page);
}

void NavigationSidebar::OnBookmarkClicked(QTreeWidgetItem* item, int /*column*/) {
    if (!item) return;
    int page = item->data(0, Qt::UserRole).toInt();
    if (page >= 0) {
        emit PageSelected(page);
    }
}

void NavigationSidebar::OnThumbnailRendered(int pageIndex, double zoomScale, const QImage& image) {
    // Only accept thumbnail-scale renders (around 0.20x)
    if (std::abs(zoomScale - 0.20) > 0.05) return;

    if (pageIndex >= 0 && pageIndex < m_thumbnailList->count()) {
        auto* item = m_thumbnailList->item(pageIndex);
        if (item) {
            QPixmap pix = QPixmap::fromImage(image).scaled(120, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setIcon(QIcon(pix));
        }
    }
}

void NavigationSidebar::PopulateAnnotations() {
    if (!m_annotationTree) return;
    m_annotationTree->clear();
    if (!m_doc || !m_doc->IsValid()) return;

    int totalAnnots = 0;
    int pageCount = m_doc->GetPageCount();

    for (int p = 0; p < pageCount; ++p) {
        auto res = m_doc->GetAnnotations(p);
        if (!res.has_value() || res.value.empty()) continue;

        auto* pageNode = new QTreeWidgetItem(m_annotationTree);
        pageNode->setText(0, tr("Page %1 (%2)").arg(p + 1).arg(res.value.size()));
        pageNode->setData(0, Qt::UserRole, p);
        pageNode->setData(0, Qt::UserRole + 1, -1);
        pageNode->setExpanded(true);

        for (const auto& a : res.value) {
            auto* item = new QTreeWidgetItem(pageNode);
            QString typeLabel;
            switch (a.type) {
                case pdf_api::AnnotationType::Highlight: typeLabel = "🖍️ Highlight"; break;
                case pdf_api::AnnotationType::Underline: typeLabel = "̲ Underline"; break;
                case pdf_api::AnnotationType::Strikeout: typeLabel = "̶ Strikeout"; break;
                case pdf_api::AnnotationType::TextNote: typeLabel = "💬 Note"; break;
                case pdf_api::AnnotationType::FreehandInk: typeLabel = "✏️ Drawing"; break;
                case pdf_api::AnnotationType::Square: typeLabel = "⬜ Rectangle"; break;
                case pdf_api::AnnotationType::Circle: typeLabel = "⚪ Circle"; break;
                case pdf_api::AnnotationType::Line:
                case pdf_api::AnnotationType::Arrow: typeLabel = "➡️ Line/Arrow"; break;
                case pdf_api::AnnotationType::Stamp: typeLabel = "🏷️ Stamp"; break;
                case pdf_api::AnnotationType::FreeText: typeLabel = "🔤 Text Box"; break;
                default: typeLabel = "Annotation"; break;
            }

            QString title = typeLabel;
            if (!a.contents.empty()) {
                title += QString(": %1").arg(QString::fromStdString(a.contents).left(30));
            } else if (!a.author.empty()) {
                title += QString(" by %1").arg(QString::fromStdString(a.author));
            }

            item->setText(0, title);
            item->setText(1, QString::number(p + 1));
            item->setData(0, Qt::UserRole, p);
            item->setData(0, Qt::UserRole + 1, a.index);
            totalAnnots++;
        }
    }

    if (totalAnnots == 0) {
        auto* emptyItem = new QTreeWidgetItem(m_annotationTree);
        emptyItem->setText(0, tr("(No annotations)"));
        emptyItem->setFlags(Qt::NoItemFlags);
    }
}

void NavigationSidebar::OnAnnotationItemClicked(QTreeWidgetItem* item, int /*column*/) {
    if (!item) return;
    int pageIndex = item->data(0, Qt::UserRole).toInt();
    int annotIndex = item->data(0, Qt::UserRole + 1).toInt();
    if (pageIndex >= 0) {
        emit PageSelected(pageIndex);
        if (annotIndex >= 0) {
            emit AnnotationSelected(pageIndex, annotIndex);
        }
    }
}

} // namespace pdfinity::ui
