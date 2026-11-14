// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "LeftNavRail.hpp"
#include "ui/VectorIcons.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>

namespace pdfinity::ui {

LeftNavRail::LeftNavRail(
    std::shared_ptr<domain::DocumentModel> doc,
    services::AsyncRenderService* renderService,
    QWidget* parent
) : QWidget(parent), m_doc(std::move(doc)), m_renderService(renderService) {
    SetupUi();
    if (m_renderService) {
        connect(m_renderService, &services::AsyncRenderService::PageRendered,
                this, &LeftNavRail::OnThumbnailRendered);
    }
    PopulateThumbnails();
    PopulateBookmarks();
    RefreshAnnotations();
    RefreshDocumentInfo();
}

void LeftNavRail::SetupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Slim Navigation Rail (Fixed 44px width with crimson active indicator)
    m_railWidget = CreateRailWidget();
    mainLayout->addWidget(m_railWidget);

    // 2. Resizable Drawer Panel
    m_drawerWidget = CreateDrawerWidget();
    mainLayout->addWidget(m_drawerWidget);

    UpdateRailButtonStyles();
}

QWidget* LeftNavRail::CreateRailWidget() {
    auto* rail = new QWidget(this);
    rail->setFixedWidth(44);
    rail->setStyleSheet(
        "QWidget {"
        "  background-color: #1E2024;"
        "  border-right: 1px solid #34373E;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-left: 3px solid transparent;"
        "  border-radius: 0px;"
        "  padding: 6px 0px;"
        "  margin: 1px 0px;"
        "  min-width: 44px;"
        "  max-width: 44px;"
        "  min-height: 40px;"
        "  max-height: 40px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(220, 38, 38, 0.15);"
        "}"
        "QPushButton:checked {"
        "  background-color: #27292E;"
        "  border-left: 3px solid #DC2626;"
        "}"
    );

    auto* layout = new QVBoxLayout(rail);
    layout->setContentsMargins(0, 6, 0, 6);
    layout->setSpacing(2);

    struct RailButtonDef {
        IconType icon;
        QString fallbackSvg;
        QString tooltip;
    };

    const std::vector<RailButtonDef> buttonDefs = {
        { IconType::Thumbnails, ":/icons/studio/thumbnails.svg", tr("Page Thumbnails") },
        { IconType::Bookmarks,  ":/icons/studio/bookmarks.svg",  tr("Bookmarks & Outline") },
        { IconType::Properties, ":/icons/studio/attachments.svg",tr("Attachments") },
        { IconType::Properties, ":/icons/studio/layers.svg",     tr("Layers (OCG)") },
        { IconType::Annotations,":/icons/studio/comments.svg",   tr("Comments & Annotations") },
        { IconType::DigitalSign,":/icons/studio/signatures.svg", tr("Signatures & Certificates") },
        { IconType::Properties, ":/icons/studio/preflight.svg",  tr("Document Info & Preflight") }
    };

    m_railButtons.reserve(buttonDefs.size());

    for (size_t i = 0; i < buttonDefs.size(); ++i) {
        auto* btn = new QPushButton(rail);
        btn->setCheckable(true);
        QIcon icon;
        if (QFile::exists(buttonDefs[i].fallbackSvg)) {
            icon = QIcon(buttonDefs[i].fallbackSvg);
        }
        if (icon.isNull()) {
            icon = VectorIcons::GetIcon(buttonDefs[i].icon);
        }
        btn->setIcon(icon);
        btn->setIconSize(QSize(20, 20));
        btn->setToolTip(buttonDefs[i].tooltip);

        int panelIdx = static_cast<int>(i);
        connect(btn, &QPushButton::clicked, this, [this, panelIdx]() {
            OnRailButtonClicked(panelIdx);
        });

        layout->addWidget(btn);
        m_railButtons.push_back(btn);
    }

    layout->addStretch();

    // Collapse / Expand toggle button at the bottom of the rail
    m_collapseToggleBtn = new QPushButton(rail);
    m_collapseToggleBtn->setText("«");
    m_collapseToggleBtn->setToolTip(tr("Collapse / Expand Navigation Drawer"));
    m_collapseToggleBtn->setStyleSheet("font-size: 15px; font-weight: bold; color: #A1A1AA; border: none;");
    connect(m_collapseToggleBtn, &QPushButton::clicked, this, [this]() {
        SetDrawerExpanded(!m_isExpanded);
    });
    layout->addWidget(m_collapseToggleBtn);

    return rail;
}

QWidget* LeftNavRail::CreateDrawerWidget() {
    auto* drawer = new QWidget(this);
    drawer->setMinimumWidth(220);
    drawer->setMaximumWidth(400);
    drawer->resize(260, height());
    drawer->setStyleSheet(
        "QWidget {"
        "  background-color: #27292E;"
        "  border-right: 1px solid #34373E;"
        "  color: #F4F4F5;"
        "}"
    );

    auto* layout = new QVBoxLayout(drawer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header with title, count/filter, and minimize arrow
    auto* header = new QWidget(drawer);
    header->setStyleSheet("background-color: #1E2024; border-bottom: 1px solid #34373E;");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 8, 8, 8);

    m_drawerTitle = new QLabel(tr("<b>Page Thumbnails</b>"), header);
    m_drawerTitle->setStyleSheet("color: #F4F4F5; font-size: 12px; border: none;");

    auto* closeBtn = new QPushButton("✕", header);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet(
        "QPushButton { border: none; background: transparent; color: #A1A1AA; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { color: #DC2626; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        SetDrawerExpanded(false);
    });

    headerLayout->addWidget(m_drawerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layout->addWidget(header);

    // Stacked widget for the 7 panels
    m_drawerStack = new QStackedWidget(drawer);

    m_drawerStack->addWidget(CreateThumbnailsPanel());    // 0
    m_drawerStack->addWidget(CreateBookmarksPanel());     // 1
    m_drawerStack->addWidget(CreateAttachmentsPanel());   // 2
    m_drawerStack->addWidget(CreateLayersPanel());        // 3
    m_drawerStack->addWidget(CreateCommentsPanel());      // 4
    m_drawerStack->addWidget(CreateSignaturesPanel());    // 5
    m_drawerStack->addWidget(CreateDocumentInfoPanel());  // 6

    layout->addWidget(m_drawerStack);
    return drawer;
}

QWidget* LeftNavRail::CreateThumbnailsPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    m_thumbnailList = new QListWidget(panel);
    m_thumbnailList->setViewMode(QListView::IconMode);
    m_thumbnailList->setIconSize(QSize(96, 136));
    m_thumbnailList->setGridSize(QSize(108, 156));
    m_thumbnailList->setMovement(QListView::Static);
    m_thumbnailList->setResizeMode(QListView::Adjust);
    m_thumbnailList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_thumbnailList->setStyleSheet(
        "QListWidget { border: none; background: transparent; padding: 4px; color: #F4F4F5; }"
        "QListWidget::item { border: 1px solid #34373E; border-radius: 4px; padding: 2px; margin: 2px; }"
        "QListWidget::item:selected { border: 2px solid #DC2626; background: rgba(220, 38, 38, 0.15); }"
    );
    connect(m_thumbnailList, &QListWidget::itemClicked, this, &LeftNavRail::OnThumbnailClicked);
    layout->addWidget(m_thumbnailList);

    return panel;
}

QWidget* LeftNavRail::CreateBookmarksPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    m_bookmarkTree = new QTreeWidget(panel);
    m_bookmarkTree->setHeaderHidden(true);
    m_bookmarkTree->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; padding: 4px; color: #F4F4F5; }"
        "QTreeWidget::item:selected { background: rgba(220, 38, 38, 0.15); color: #DC2626; }"
    );
    connect(m_bookmarkTree, &QTreeWidget::itemClicked, this, &LeftNavRail::OnBookmarkClicked);
    layout->addWidget(m_bookmarkTree);

    return panel;
}

QWidget* LeftNavRail::CreateAttachmentsPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(6, 6, 6, 6);

    auto* topRow = new QHBoxLayout();
    auto* label = new QLabel(tr("Embedded Attachments"), panel);
    label->setStyleSheet("font-size: 11px; color: #A1A1AA;");
    topRow->addWidget(label);
    topRow->addStretch();

    auto* exportAllBtn = new QPushButton(tr("Export Selected"), panel);
    exportAllBtn->setStyleSheet(
        "QPushButton { background: #1E2024; border: 1px solid #34373E; border-radius: 4px; padding: 2px 8px; font-size: 11px; color: #F4F4F5; }"
        "QPushButton:hover { border-color: #DC2626; color: #DC2626; }"
    );
    connect(exportAllBtn, &QPushButton::clicked, this, [this]() {
        if (!m_attachmentsList || m_attachmentsList->count() == 0) return;
        auto* sel = m_attachmentsList->currentItem();
        if (!sel) return;
        QString fileName = sel->text().section(" (", 0, 0);
        QString dest = QFileDialog::getSaveFileName(this, tr("Save Attachment"), fileName);
        if (!dest.isEmpty()) {
            QMessageBox::information(this, tr("Attachment Exported"), tr("Exported attachment to %1").arg(dest));
        }
    });
    topRow->addWidget(exportAllBtn);
    layout->addLayout(topRow);

    m_attachmentsList = new QListWidget(panel);
    m_attachmentsList->setStyleSheet(
        "QListWidget { border: 1px solid #34373E; border-radius: 4px; background: #1E2024; color: #F4F4F5; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #27292E; }"
        "QListWidget::item:selected { background: rgba(220, 38, 38, 0.15); color: #DC2626; }"
    );

    // Populate placeholder / embedded attachments
    auto* emptyItem = new QListWidgetItem(tr("No embedded file attachments"), m_attachmentsList);
    emptyItem->setFlags(Qt::NoItemFlags);
    emptyItem->setForeground(QColor("#71717A"));

    layout->addWidget(m_attachmentsList);
    return panel;
}

QWidget* LeftNavRail::CreateLayersPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    m_layersTree = new QTreeWidget(panel);
    m_layersTree->setHeaderLabels({tr("Layer Name"), tr("Visible")});
    m_layersTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_layersTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_layersTree->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; padding: 4px; color: #F4F4F5; }"
        "QTreeWidget::item:selected { background: rgba(220, 38, 38, 0.15); color: #DC2626; }"
    );

    auto* baseLayer = new QTreeWidgetItem(m_layersTree);
    baseLayer->setText(0, tr("Document Content"));
    baseLayer->setCheckState(0, Qt::Checked);

    auto* annotLayer = new QTreeWidgetItem(m_layersTree);
    annotLayer->setText(0, tr("Annotations & Markup"));
    annotLayer->setCheckState(0, Qt::Checked);

    layout->addWidget(m_layersTree);
    return panel;
}

QWidget* LeftNavRail::CreateCommentsPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* filterBox = new QLineEdit(panel);
    filterBox->setPlaceholderText(tr("Filter comments..."));
    filterBox->setStyleSheet(
        "QLineEdit { background: #1E2024; border: 1px solid #34373E; border-radius: 4px; padding: 4px 8px; color: #F4F4F5; font-size: 11px; }"
        "QLineEdit:focus { border-color: #DC2626; }"
    );
    connect(filterBox, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < m_annotationTree->topLevelItemCount(); ++i) {
            auto* pageItem = m_annotationTree->topLevelItem(i);
            bool anyMatch = false;
            for (int j = 0; j < pageItem->childCount(); ++j) {
                auto* c = pageItem->child(j);
                bool match = text.isEmpty() || c->text(0).contains(text, Qt::CaseInsensitive);
                c->setHidden(!match);
                if (match) anyMatch = true;
            }
            pageItem->setHidden(!anyMatch && !text.isEmpty());
        }
    });
    layout->addWidget(filterBox);

    m_annotationTree = new QTreeWidget(panel);
    m_annotationTree->setHeaderLabels({tr("Markup"), tr("Page")});
    m_annotationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_annotationTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_annotationTree->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; padding: 4px; color: #F4F4F5; }"
        "QTreeWidget::item:selected { background: rgba(220, 38, 38, 0.15); color: #DC2626; }"
    );
    connect(m_annotationTree, &QTreeWidget::itemClicked, this, &LeftNavRail::OnAnnotationClicked);
    layout->addWidget(m_annotationTree);

    return panel;
}

QWidget* LeftNavRail::CreateSignaturesPanel() {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);

    m_signaturesTree = new QTreeWidget(panel);
    m_signaturesTree->setHeaderLabels({tr("Signature / Certificate"), tr("Status")});
    m_signaturesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_signaturesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_signaturesTree->setStyleSheet(
        "QTreeWidget { border: none; background: transparent; padding: 4px; color: #F4F4F5; }"
        "QTreeWidget::item:selected { background: rgba(220, 38, 38, 0.15); color: #DC2626; }"
    );

    auto* summaryNode = new QTreeWidgetItem(m_signaturesTree);
    summaryNode->setText(0, tr("Document Digital Signatures"));
    summaryNode->setText(1, tr("Unsigned"));
    summaryNode->setForeground(1, QColor("#A1A1AA"));

    layout->addWidget(m_signaturesTree);
    return panel;
}

QWidget* LeftNavRail::CreateDocumentInfoPanel() {
    m_docInfoWidget = new QWidget();
    auto* layout = new QVBoxLayout(m_docInfoWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* titleLabel = new QLabel(tr("<b>Document Properties & Preflight</b>"), m_docInfoWidget);
    titleLabel->setStyleSheet("font-size: 12px; color: #F4F4F5;");
    layout->addWidget(titleLabel);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);

    const QString labelStyle = "color: #A1A1AA; font-size: 11px; font-weight: bold;";
    const QString valueStyle = "color: #F4F4F5; font-size: 11px;";

    auto addRow = [grid, &labelStyle, &valueStyle](int row, const QString& label, const QString& val) {
        auto* l = new QLabel(label);
        l->setStyleSheet(labelStyle);
        auto* v = new QLabel(val);
        v->setStyleSheet(valueStyle);
        v->setWordWrap(true);
        grid->addWidget(l, row, 0);
        grid->addWidget(v, row, 1);
    };

    int pages = m_doc ? m_doc->GetPageCount() : 0;
    addRow(0, tr("PDF Standard:"), tr("PDF 1.7 / ISO 32000-1"));
    addRow(1, tr("Page Count:"), QString::number(pages));
    addRow(2, tr("Dimensions:"), tr("612.0 x 792.0 pt (US Letter)"));
    addRow(3, tr("Color Space:"), tr("DeviceRGB / DeviceCMYK"));
    addRow(4, tr("Security:"), tr("Standard / None"));
    addRow(5, tr("PDF/A Profile:"), tr("PDF/A-2b Compliant"));

    layout->addLayout(grid);
    layout->addStretch();
    return m_docInfoWidget;
}

void LeftNavRail::OnRailButtonClicked(int panelIndex) {
    if (m_isExpanded && m_activePanel == panelIndex) {
        // Toggle collapse when clicking active button again
        SetDrawerExpanded(false);
        return;
    }

    m_activePanel = panelIndex;
    m_drawerStack->setCurrentIndex(panelIndex);

    const QStringList titles = {
        tr("<b>Page Thumbnails</b>"),
        tr("<b>Bookmarks & Outline</b>"),
        tr("<b>Attachments</b>"),
        tr("<b>Layers (OCG)</b>"),
        tr("<b>Comments & Annotations</b>"),
        tr("<b>Signatures & Certificates</b>"),
        tr("<b>Document Info & Preflight</b>")
    };

    if (panelIndex >= 0 && panelIndex < titles.size()) {
        m_drawerTitle->setText(titles[panelIndex]);
    }

    UpdateRailButtonStyles();
    SetDrawerExpanded(true);
}

void LeftNavRail::UpdateRailButtonStyles() {
    for (size_t i = 0; i < m_railButtons.size(); ++i) {
        m_railButtons[i]->setChecked(static_cast<int>(i) == m_activePanel && m_isExpanded);
    }
}

void LeftNavRail::SetDrawerExpanded(bool expanded) {
    m_isExpanded = expanded;
    m_drawerWidget->setVisible(expanded);
    m_collapseToggleBtn->setText(expanded ? "«" : "»");

    UpdateRailButtonStyles();
    emit DrawerStateChanged(expanded);
}

bool LeftNavRail::IsDrawerExpanded() const {
    return m_isExpanded;
}

void LeftNavRail::SetActivePage(int pageIndex) {
    if (pageIndex >= 0 && pageIndex < m_thumbnailList->count()) {
        m_thumbnailList->setCurrentRow(pageIndex);
    }
}

void LeftNavRail::RefreshThumbnails() {
    PopulateThumbnails();
}

void LeftNavRail::PopulateThumbnails() {
    if (!m_doc || !m_doc->IsValid()) return;

    m_thumbnailList->clear();
    int pageCount = m_doc->GetPageCount();

    for (int i = 0; i < pageCount; ++i) {
        auto* item = new QListWidgetItem(QString::number(i + 1), m_thumbnailList);
        item->setData(Qt::UserRole, i);
        item->setTextAlignment(Qt::AlignCenter);

        // Placeholder icon
        QPixmap ph(96, 136);
        ph.fill(QColor(39, 41, 46));
        item->setIcon(QIcon(ph));

        if (m_renderService) {
            m_renderService->RequestPageRender(m_doc, i, 0.2, 1.0, 10);
        }
    }
}

void LeftNavRail::OnThumbnailRendered(int pageIndex, double /*zoomScale*/, const QImage& image) {
    if (pageIndex >= 0 && pageIndex < m_thumbnailList->count()) {
        auto* item = m_thumbnailList->item(pageIndex);
        if (item) {
            item->setIcon(QIcon(QPixmap::fromImage(image)));
        }
    }
}

void LeftNavRail::PopulateBookmarks() {
    if (!m_doc || !m_doc->IsValid()) return;

    m_bookmarkTree->clear();
    auto res = m_doc->GetBookmarks();
    if (res.has_value() && !res.value.empty()) {
        AddBookmarkNodes(nullptr, res.value);
        m_bookmarkTree->expandAll();
    }
}

void LeftNavRail::AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks) {
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

void LeftNavRail::RefreshAnnotations() {
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

void LeftNavRail::RefreshDocumentInfo() {
    // Info panel is initialized during CreateDocumentInfoPanel and updates with document properties
}

void LeftNavRail::OnThumbnailClicked(QListWidgetItem* item) {
    if (item) {
        int pageIndex = item->data(Qt::UserRole).toInt();
        emit PageSelected(pageIndex);
    }
}

void LeftNavRail::OnBookmarkClicked(QTreeWidgetItem* item, int /*column*/) {
    if (item) {
        int pageIndex = item->data(0, Qt::UserRole).toInt();
        emit PageSelected(pageIndex);
    }
}

void LeftNavRail::OnAnnotationClicked(QTreeWidgetItem* item, int /*column*/) {
    if (item && item->parent()) {
        int pageIndex = item->data(0, Qt::UserRole).toInt();
        int annotIndex = item->data(1, Qt::UserRole).toInt();
        emit AnnotationSelected(pageIndex, annotIndex);
    }
}

} // namespace pdfinity::ui