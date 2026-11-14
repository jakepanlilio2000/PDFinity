// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "LeftNavigationRail.hpp"
#include "VectorIcons.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>

namespace pdfinity::ui {

LeftNavigationRail::LeftNavigationRail(
    std::shared_ptr<domain::DocumentModel> doc,
    services::AsyncRenderService* renderService,
    QWidget* parent
) : QWidget(parent), m_doc(std::move(doc)), m_renderService(renderService) {
    SetupUi();
    if (m_renderService) {
        connect(m_renderService, &services::AsyncRenderService::PageRendered,
                this, &LeftNavigationRail::OnThumbnailRendered);
    }
    PopulateThumbnails();
    PopulateBookmarks();
    RefreshAnnotations();
}

void LeftNavigationRail::SetupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Slim Navigation Rail (Fixed 48px width)
    m_railWidget = CreateRailWidget();
    mainLayout->addWidget(m_railWidget);

    // 2. Slide-out Drawer Panel (Fixed 240px width when open, 0px when closed)
    m_drawerWidget = CreateDrawerWidget();
    m_drawerWidget->setVisible(false);
    mainLayout->addWidget(m_drawerWidget);
}

QWidget* LeftNavigationRail::CreateRailWidget() {
    auto* rail = new QWidget(this);
    rail->setFixedWidth(48);
    rail->setStyleSheet(
        "QWidget {"
        "  background-color: #f8fafc;"
        "  border-right: 1px solid #e2e8f0;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  margin: 2px 4px;"
        "  min-width: 36px;"
        "  min-height: 36px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e2e8f0;"
        "}"
        "QPushButton:checked {"
        "  background-color: #dbeafe;"
        "}"
    );

    auto* layout = new QVBoxLayout(rail);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(4);

    m_thumbBtn = new QPushButton(rail);
    m_thumbBtn->setCheckable(true);
    m_thumbBtn->setIcon(VectorIcons::GetIcon(IconType::Thumbnails));
    m_thumbBtn->setToolTip(tr("Thumbnails"));
    connect(m_thumbBtn, &QPushButton::clicked, this, [this]() { OnRailButtonClicked(0); });
    layout->addWidget(m_thumbBtn);

    m_bookmarkBtn = new QPushButton(rail);
    m_bookmarkBtn->setCheckable(true);
    m_bookmarkBtn->setIcon(VectorIcons::GetIcon(IconType::Bookmarks));
    m_bookmarkBtn->setToolTip(tr("Bookmarks & Outline"));
    connect(m_bookmarkBtn, &QPushButton::clicked, this, [this]() { OnRailButtonClicked(1); });
    layout->addWidget(m_bookmarkBtn);

    m_annotBtn = new QPushButton(rail);
    m_annotBtn->setCheckable(true);
    m_annotBtn->setIcon(VectorIcons::GetIcon(IconType::Annotations));
    m_annotBtn->setToolTip(tr("Annotations & Comments"));
    connect(m_annotBtn, &QPushButton::clicked, this, [this]() { OnRailButtonClicked(2); });
    layout->addWidget(m_annotBtn);

    layout->addStretch();

    m_toggleCollapseBtn = new QPushButton(rail);
    m_toggleCollapseBtn->setText("»");
    m_toggleCollapseBtn->setToolTip(tr("Expand / Collapse Navigation Drawer"));
    m_toggleCollapseBtn->setStyleSheet("font-size: 16px; font-weight: bold; color: #64748b;");
    connect(m_toggleCollapseBtn, &QPushButton::clicked, this, [this]() {
        SetDrawerExpanded(!m_isExpanded);
    });
    layout->addWidget(m_toggleCollapseBtn);

    return rail;
}

QWidget* LeftNavigationRail::CreateDrawerWidget() {
    auto* drawer = new QWidget(this);
    drawer->setFixedWidth(240);
    drawer->setStyleSheet(
        "QWidget {"
        "  background-color: #ffffff;"
        "  border-right: 1px solid #cbd5e1;"
        "}"
    );

    auto* layout = new QVBoxLayout(drawer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header with title and close button
    auto* header = new QWidget(drawer);
    header->setStyleSheet("background-color: #f1f5f9; border-bottom: 1px solid #e2e8f0;");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 6, 8, 6);

    auto* titleLabel = new QLabel(tr("<b>Navigation</b>"), header);
    titleLabel->setStyleSheet("color: #334155; font-size: 12px; border: none;");

    auto* closeBtn = new QPushButton("✕", header);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet("border: none; background: transparent; color: #64748b; font-weight: bold;");
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        SetDrawerExpanded(false);
    });

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layout->addWidget(header);

    // Stacked widget for panels
    m_drawerStack = new QStackedWidget(drawer);

    // Panel 0: Thumbnails
    m_thumbnailList = new QListWidget(m_drawerStack);
    m_thumbnailList->setViewMode(QListView::IconMode);
    m_thumbnailList->setIconSize(QSize(100, 140));
    m_thumbnailList->setGridSize(QSize(110, 160));
    m_thumbnailList->setMovement(QListView::Static);
    m_thumbnailList->setResizeMode(QListView::Adjust);
    m_thumbnailList->setStyleSheet("border: none; background: #ffffff; padding: 4px;");
    connect(m_thumbnailList, &QListWidget::itemClicked, this, &LeftNavigationRail::OnThumbnailClicked);
    m_drawerStack->addWidget(m_thumbnailList);

    // Panel 1: Bookmarks
    m_bookmarkTree = new QTreeWidget(m_drawerStack);
    m_bookmarkTree->setHeaderHidden(true);
    m_bookmarkTree->setStyleSheet("border: none; background: #ffffff; padding: 4px;");
    connect(m_bookmarkTree, &QTreeWidget::itemClicked, this, &LeftNavigationRail::OnBookmarkClicked);
    m_drawerStack->addWidget(m_bookmarkTree);

    // Panel 2: Annotations
    m_annotationTree = new QTreeWidget(m_drawerStack);
    m_annotationTree->setHeaderLabels({tr("Markup"), tr("Page")});
    m_annotationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_annotationTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_annotationTree->setStyleSheet("border: none; background: #ffffff; padding: 4px;");
    connect(m_annotationTree, &QTreeWidget::itemClicked, this, &LeftNavigationRail::OnAnnotationClicked);
    m_drawerStack->addWidget(m_annotationTree);

    layout->addWidget(m_drawerStack);
    return drawer;
}

void LeftNavigationRail::OnRailButtonClicked(int panelIndex) {
    if (m_isExpanded && m_activePanel == panelIndex) {
        // Toggle collapse
        SetDrawerExpanded(false);
        return;
    }

    m_activePanel = panelIndex;
    m_drawerStack->setCurrentIndex(panelIndex);
    m_thumbBtn->setChecked(panelIndex == 0);
    m_bookmarkBtn->setChecked(panelIndex == 1);
    m_annotBtn->setChecked(panelIndex == 2);
    SetDrawerExpanded(true);
}

void LeftNavigationRail::SetDrawerExpanded(bool expanded) {
    m_isExpanded = expanded;
    m_drawerWidget->setVisible(expanded);
    m_toggleCollapseBtn->setText(expanded ? "«" : "»");

    if (!expanded) {
        m_thumbBtn->setChecked(false);
        m_bookmarkBtn->setChecked(false);
        m_annotBtn->setChecked(false);
        m_activePanel = -1;
    } else if (m_activePanel < 0) {
        OnRailButtonClicked(0);
    }

    emit DrawerStateChanged(expanded);
}

bool LeftNavigationRail::IsDrawerExpanded() const {
    return m_isExpanded;
}

void LeftNavigationRail::SetActivePage(int pageIndex) {
    if (pageIndex >= 0 && pageIndex < m_thumbnailList->count()) {
        m_thumbnailList->setCurrentRow(pageIndex);
    }
}

void LeftNavigationRail::PopulateThumbnails() {
    if (!m_doc || !m_doc->IsValid()) return;

    m_thumbnailList->clear();
    int pageCount = m_doc->GetPageCount();

    for (int i = 0; i < pageCount; ++i) {
        auto* item = new QListWidgetItem(QString::number(i + 1), m_thumbnailList);
        item->setData(Qt::UserRole, i);
        item->setTextAlignment(Qt::AlignCenter);

        // Placeholder icon
        QPixmap ph(100, 140);
        ph.fill(QColor(241, 245, 249));
        item->setIcon(QIcon(ph));

        if (m_renderService) {
            m_renderService->RequestPageRender(m_doc, i, 0.2, 1.0, 10);
        }
    }
}

void LeftNavigationRail::OnThumbnailRendered(int pageIndex, double /*zoomScale*/, const QImage& image) {
    if (pageIndex >= 0 && pageIndex < m_thumbnailList->count()) {
        auto* item = m_thumbnailList->item(pageIndex);
        if (item) {
            item->setIcon(QIcon(QPixmap::fromImage(image)));
        }
    }
}

void LeftNavigationRail::PopulateBookmarks() {
    if (!m_doc || !m_doc->IsValid()) return;

    m_bookmarkTree->clear();
    auto res = m_doc->GetBookmarks();
    if (res.has_value() && !res.value.empty()) {
        AddBookmarkNodes(nullptr, res.value);
        m_bookmarkTree->expandAll();
    }
}

void LeftNavigationRail::AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks) {
    for (const auto& bm : bookmarks) {
        auto* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_bookmarkTree);
        item->setText(0, QString::fromStdString(bm.title));
        item->setData(0, Qt::UserRole, bm.targetPageIndex);
        item->setIcon(0, VectorIcons::GetIcon(IconType::Bookmarks));
        if (!bm.children.empty()) {
            AddBookmarkNodes(item, bm.children);
        }
    }
}

void LeftNavigationRail::RefreshAnnotations() {
    if (!m_doc || !m_doc->IsValid()) return;

    m_annotationTree->clear();
    int pageCount = m_doc->GetPageCount();

    for (int p = 0; p < pageCount; ++p) {
        auto annotsRes = m_doc->GetAnnotations(p);
        if (!annotsRes.has_value() || annotsRes.value.empty()) continue;

        auto* pageNode = new QTreeWidgetItem(m_annotationTree);
        pageNode->setText(0, QString(tr("Page %1 (%2)")).arg(p + 1).arg(annotsRes.value.size()));
        pageNode->setData(0, Qt::UserRole, p);
        pageNode->setIcon(0, VectorIcons::GetIcon(IconType::Properties));

        for (size_t a = 0; a < annotsRes.value.size(); ++a) {
            const auto& annot = annotsRes.value[a];
            auto* annotNode = new QTreeWidgetItem(pageNode);
            QString label = QString::fromStdString(annot.contents);
            if (label.isEmpty()) label = QString::fromStdString(annot.author);
            if (label.isEmpty()) label = QString(tr("Markup #%1")).arg(a + 1);

            annotNode->setText(0, label);
            annotNode->setText(1, QString::number(p + 1));
            annotNode->setData(0, Qt::UserRole, p);
            annotNode->setData(1, Qt::UserRole, static_cast<int>(a));
            annotNode->setIcon(0, VectorIcons::GetIcon(IconType::Annotations));
        }
        pageNode->setExpanded(true);
    }
}

void LeftNavigationRail::OnThumbnailClicked(QListWidgetItem* item) {
    if (item) {
        int pageIndex = item->data(Qt::UserRole).toInt();
        emit PageSelected(pageIndex);
    }
}

void LeftNavigationRail::OnBookmarkClicked(QTreeWidgetItem* item, int /*column*/) {
    if (item) {
        int pageIndex = item->data(0, Qt::UserRole).toInt();
        emit PageSelected(pageIndex);
    }
}

void LeftNavigationRail::OnAnnotationClicked(QTreeWidgetItem* item, int /*column*/) {
    if (item && item->parent()) {
        int pageIndex = item->data(0, Qt::UserRole).toInt();
        int annotIndex = item->data(1, Qt::UserRole).toInt();
        emit AnnotationSelected(pageIndex, annotIndex);
    }
}

} // namespace pdfinity::ui
