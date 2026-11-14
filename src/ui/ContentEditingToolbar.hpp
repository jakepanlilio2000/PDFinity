// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "services/ContentEditingService.hpp"
#include <QToolBar>
#include <QAction>
#include <memory>

namespace pdfinity::ui {

class ContentEditingToolbar : public QToolBar {
    Q_OBJECT

public:
    explicit ContentEditingToolbar(
        std::shared_ptr<services::ContentEditingService> service,
        QWidget* parent = nullptr
    );
    ~ContentEditingToolbar() override = default;

    void SetDocument(std::shared_ptr<domain::DocumentModel> doc, int currentPageIndex);

signals:
    void InsertTextRequested();
    void InsertImageRequested();
    void InsertShapeRequested();
    void OpenInspectorRequested();
    void ContentModified();

private slots:
    void OnInsertText();
    void OnInsertImage();
    void OnInsertShape();

private:
    void SetupActions();

    std::shared_ptr<services::ContentEditingService> m_service;
    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};

    QAction* m_insertTextAction{nullptr};
    QAction* m_insertImageAction{nullptr};
    QAction* m_insertShapeAction{nullptr};
    QAction* m_gridSnapAction{nullptr};
    QAction* m_undoAction{nullptr};
    QAction* m_redoAction{nullptr};
    QAction* m_inspectorAction{nullptr};
};

} // namespace pdfinity::ui
