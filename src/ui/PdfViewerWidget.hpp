// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QSplitter>
#include <QToolBar>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include "domain/DocumentModel.hpp"
#include "services/AsyncRenderService.hpp"
#include "services/DocumentSearchService.hpp"
#include "PdfCanvasWidget.hpp"
#include "NavigationSidebar.hpp"
#include "PdfSearchBar.hpp"
#include <memory>

namespace pdfinity::services {
class AnnotationCommandManager;
class ContentEditingService;
}

namespace pdfinity::ui {

enum class AnnotationToolMode;
class AnnotationToolbar;
class ContentEditingToolbar;
class XfaNoticeBanner;
class LeftNavRail;
class FloatingHudWidget;
class RightInspectorPanel;

class PdfViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit PdfViewerWidget(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );
    ~PdfViewerWidget() override;

    [[nodiscard]] std::shared_ptr<domain::DocumentModel> GetDocument() const { return m_doc; }
    [[nodiscard]] int GetCurrentPage() const { return m_canvas->GetCurrentPage(); }
    [[nodiscard]] double GetZoomScale() const { return m_canvas->GetZoomScale(); }
    [[nodiscard]] pdf_api::PageLayoutMode GetLayoutMode() const { return m_canvas->GetLayoutMode(); }
    [[nodiscard]] RightInspectorPanel* GetInspector() const { return m_inspector; }
    [[nodiscard]] LeftNavRail* GetNavigationRail() const { return m_leftRail; }
    [[nodiscard]] std::vector<int> GetSplitterSizes() const;
    void ApplySettingsState();

    void SetZoom(double zoom);
    void SetLayout(pdf_api::PageLayoutMode mode);
    void GoToPage(int pageIndex);
    void SetAnnotationToolMode(AnnotationToolMode mode);

public slots:
    void Print();
    void PrintPreview();
    void ToggleFind();
    void ToggleSidebar();
    void ToggleInspector();
    void ToggleAnnotationStudio();
    void ZoomIn();
    void ZoomOut();
    void ZoomFitWidth();
    void ZoomFitPage();
    void Undo();
    void Redo();
    void OpenFormInspector();
    void ToggleContentEditor();
    void OpenObjectInspector();

signals:
    void PageChanged(int pageIndex, int totalPages);
    void TitleChanged(const QString& title);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void OnPageNumberChanged(int pageNum);
    void OnCanvasPageChanged(int pageIndex);
    void OnCanvasZoomChanged(double zoomScale);
    void OnSearchRequested(const QString& query, bool matchCase, bool matchWholeWord);
    void OnFindNext();
    void OnFindPrev();
    void OnSearchCompleted(const std::vector<pdf_api::TextSearchResult>& allMatches);

private:
    void SetupUi();
    void CreateToolBar();

    std::shared_ptr<domain::DocumentModel> m_doc;
    std::unique_ptr<services::AsyncRenderService> m_renderService;
    std::unique_ptr<services::DocumentSearchService> m_searchService;

    QSplitter* m_splitter{nullptr};
    NavigationSidebar* m_sidebar{nullptr};
    LeftNavRail* m_leftRail{nullptr};
    QScrollArea* m_scrollArea{nullptr};
    PdfCanvasWidget* m_canvas{nullptr};
    FloatingHudWidget* m_floatingHud{nullptr};
    RightInspectorPanel* m_inspector{nullptr};
    PdfSearchBar* m_searchBar{nullptr};
    AnnotationToolbar* m_annotToolbar{nullptr};
    ContentEditingToolbar* m_contentToolbar{nullptr};
    XfaNoticeBanner* m_xfaBanner{nullptr};
    std::unique_ptr<services::AnnotationCommandManager> m_commandManager;
    std::shared_ptr<services::ContentEditingService> m_contentEditService;

    // Toolbar controls
    QToolBar* m_toolBar{nullptr};
    QAction* m_toggleSidebarAction{nullptr};
    QAction* m_toggleInspectorAction{nullptr};
    QAction* m_toggleAnnotAction{nullptr};
    QAction* m_formInspectorAction{nullptr};
    QAction* m_toggleContentEditAction{nullptr};
    QAction* m_openInspectorAction{nullptr};
    QSpinBox* m_pageSpinBox{nullptr};
    QLabel* m_pageCountLabel{nullptr};
    QLabel* m_zoomLabel{nullptr};
    QComboBox* m_layoutCombo{nullptr};

    std::vector<pdf_api::TextSearchResult> m_currentMatches;
    int m_activeMatchIdx{-1};
};

} // namespace pdfinity::ui
