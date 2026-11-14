// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include "domain/DocumentModel.hpp"
#include "services/AsyncRenderService.hpp"
#include <memory>

namespace pdfinity::ui {

class LeftNavigationRail : public QWidget {
    Q_OBJECT

public:
    explicit LeftNavigationRail(
        std::shared_ptr<domain::DocumentModel> doc,
        services::AsyncRenderService* renderService,
        QWidget* parent = nullptr
    );
    ~LeftNavigationRail() override = default;

    void SetActivePage(int pageIndex);
    void RefreshAnnotations();
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
    void PopulateThumbnails();
    void PopulateBookmarks();
    void AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks);

    std::shared_ptr<domain::DocumentModel> m_doc;
    services::AsyncRenderService* m_renderService{nullptr};

    QWidget* m_railWidget{nullptr};
    QWidget* m_drawerWidget{nullptr};
    QStackedWidget* m_drawerStack{nullptr};

    QPushButton* m_thumbBtn{nullptr};
    QPushButton* m_bookmarkBtn{nullptr};
    QPushButton* m_annotBtn{nullptr};
    QPushButton* m_toggleCollapseBtn{nullptr};

    QListWidget* m_thumbnailList{nullptr};
    QTreeWidget* m_bookmarkTree{nullptr};
    QTreeWidget* m_annotationTree{nullptr};

    int m_activePanel{-1};
    bool m_isExpanded{false};
};

} // namespace pdfinity::ui
