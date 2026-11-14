// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include <QLabel>
#include <memory>
#include "domain/DocumentModel.hpp"
#include "services/AsyncRenderService.hpp"

namespace pdfinity::ui {

class LeftNavRail : public QWidget {
    Q_OBJECT

public:
    explicit LeftNavRail(
        std::shared_ptr<domain::DocumentModel> doc,
        services::AsyncRenderService* renderService,
        QWidget* parent = nullptr
    );
    ~LeftNavRail() override = default;

    void SetActivePage(int pageIndex);
    void RefreshAnnotations();
    void RefreshDocumentInfo();
    void SetDrawerExpanded(bool expanded);
    bool IsDrawerExpanded() const;

signals:
    void PageSelected(int pageIndex);
    void AnnotationSelected(int pageIndex, int annotIndex);
    void DrawerStateChanged(bool expanded);

private slots:
    void OnRailButtonClicked(int panelIndex);
    void OnThumbnailClicked(QListWidgetItem* item);
    void OnBookmarkClicked(QTreeWidgetItem* item, int column);
    void OnAnnotationClicked(QTreeWidgetItem* item, int column);
    void OnThumbnailRendered(int pageIndex, double zoomScale, const QImage& image);

private:
    void SetupUi();
    QWidget* CreateRailWidget();
    QWidget* CreateDrawerWidget();

    // 7 Drawer Panels
    QWidget* CreateThumbnailsPanel();
    QWidget* CreateBookmarksPanel();
    QWidget* CreateAttachmentsPanel();
    QWidget* CreateLayersPanel();
    QWidget* CreateCommentsPanel();
    QWidget* CreateSignaturesPanel();
    QWidget* CreateDocumentInfoPanel();

    void PopulateThumbnails();
    void PopulateBookmarks();
    void AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks);
    void UpdateRailButtonStyles();

    std::shared_ptr<domain::DocumentModel> m_doc;
    services::AsyncRenderService* m_renderService{nullptr};

    QWidget* m_railWidget{nullptr};
    QWidget* m_drawerWidget{nullptr};
    QStackedWidget* m_drawerStack{nullptr};
    QLabel* m_drawerTitle{nullptr};

    std::vector<QPushButton*> m_railButtons;
    QPushButton* m_collapseToggleBtn{nullptr};

    QListWidget* m_thumbnailList{nullptr};
    QTreeWidget* m_bookmarkTree{nullptr};
    QListWidget* m_attachmentsList{nullptr};
    QTreeWidget* m_layersTree{nullptr};
    QTreeWidget* m_annotationTree{nullptr};
    QTreeWidget* m_signaturesTree{nullptr};
    QWidget* m_docInfoWidget{nullptr};

    int m_activePanel{0};
    bool m_isExpanded{true};
};

} // namespace pdfinity::ui
