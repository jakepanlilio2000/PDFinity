// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include "domain/DocumentModel.hpp"
#include "services/AsyncRenderService.hpp"
#include <memory>

namespace pdfinity::ui {

class NavigationSidebar : public QWidget {
    Q_OBJECT

public:
    explicit NavigationSidebar(
        std::shared_ptr<domain::DocumentModel> doc,
        services::AsyncRenderService* renderService,
        QWidget* parent = nullptr
    );
    ~NavigationSidebar() override = default;

    void SetActivePage(int pageIndex);
    void PopulateAnnotations();

signals:
    void PageSelected(int pageIndex);
    void AnnotationSelected(int pageIndex, int annotIndex);

private slots:
    void OnThumbnailClicked(QListWidgetItem* item);
    void OnBookmarkClicked(QTreeWidgetItem* item, int column);
    void OnAnnotationItemClicked(QTreeWidgetItem* item, int column);
    void OnThumbnailRendered(int pageIndex, double zoomScale, const QImage& image);

private:
    void SetupUi();
    void PopulateThumbnails();
    void PopulateBookmarks();
    void AddBookmarkNodes(QTreeWidgetItem* parentItem, const std::vector<pdf_api::BookmarkItem>& bookmarks);

    std::shared_ptr<domain::DocumentModel> m_doc;
    services::AsyncRenderService* m_renderService{nullptr};

    QTabWidget* m_tabWidget{nullptr};
    QListWidget* m_thumbnailList{nullptr};
    QTreeWidget* m_bookmarkTree{nullptr};
    QTreeWidget* m_annotationTree{nullptr};
};

} // namespace pdfinity::ui
