// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QMenu>
#include "domain/DocumentModel.hpp"
#include "services/AsyncRenderService.hpp"
#include <memory>
#include <vector>

namespace pdfinity::services {
class AnnotationCommandManager;
}

namespace pdfinity::ui {

struct PageLayoutRect {
    int pageIndex{0};
    QRectF rect;       // In canvas logical coordinate space
    pdf_api::PdfSize pageSizePoints;
};

class PdfCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit PdfCanvasWidget(
        std::shared_ptr<domain::DocumentModel> doc,
        services::AsyncRenderService* renderService,
        QWidget* parent = nullptr
    );
    ~PdfCanvasWidget() override = default;

    void SetLayoutMode(pdf_api::PageLayoutMode mode);
    [[nodiscard]] pdf_api::PageLayoutMode GetLayoutMode() const { return m_layoutMode; }

    void SetZoomScale(double scale);
    [[nodiscard]] double GetZoomScale() const { return m_zoomScale; }

    void GoToPage(int pageIndex);
    [[nodiscard]] int GetCurrentPage() const { return m_currentPage; }
    [[nodiscard]] int GetPageCount() const;

    void SetSearchMatches(const std::vector<pdf_api::TextSearchResult>& matches, int activeIndex);
    void SetActiveMatchIndex(int activeIndex);

    void ZoomIn();
    void ZoomOut();
    void ZoomFitWidth(int viewportWidth);
    void ZoomFitPage(int viewportWidth, int viewportHeight);

    // Annotation Studio
    void SetAnnotationToolMode(int mode);
    void SetAnnotationColor(const QColor& color);
    void SetAnnotationStrokeWidth(float width);
    void SetStampName(const QString& name);
    void SetCommandManager(services::AnnotationCommandManager* cmdMgr);
    void SelectAnnotation(int pageIndex, int annotIndex);
    void DeleteSelectedAnnotation();
    void OpenSelectedAnnotationProperties();

signals:
    void CurrentPageChanged(int pageIndex);
    void ZoomChanged(double zoomScale);
    void TextSelected(const QString& text);
    void AnnotationSelected(int pageIndex, int annotIndex);
    void AnnotationModified();

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void OnPageRendered(int pageIndex, double zoomScale, const QImage& image);
    void OnCopySelection();

private:
    void RecalculateLayout();
    void UpdateCurrentPageFromScroll();
    [[nodiscard]] int GetPageIndexAtPoint(const QPointF& pos) const;
    [[nodiscard]] pdf_api::PdfRect ScreenToPdfRect(int pageIndex, const QRectF& screenRect) const;
    [[nodiscard]] QRectF PdfToScreenRect(int pageIndex, const pdf_api::PdfRect& pdfRect) const;
    [[nodiscard]] pdf_api::PdfPoint ScreenToPdfPoint(int pageIndex, const QPointF& screenPoint) const;
    [[nodiscard]] QPointF PdfToScreenPoint(int pageIndex, const pdf_api::PdfPoint& pdfPoint) const;

    std::shared_ptr<domain::DocumentModel> m_doc;
    services::AsyncRenderService* m_renderService{nullptr};

    pdf_api::PageLayoutMode m_layoutMode{pdf_api::PageLayoutMode::Continuous};
    double m_zoomScale{1.0};
    int m_currentPage{0};

    std::vector<PageLayoutRect> m_pageLayouts;
    std::vector<pdf_api::PdfSize> m_pageSizes;

    // Selection
    bool m_isSelecting{false};
    int m_selectionPageIndex{-1};
    QPointF m_selectionStart;
    QPointF m_selectionEnd;
    QString m_selectedText;

    // Search highlights
    std::vector<pdf_api::TextSearchResult> m_searchMatches;
    int m_activeMatchIndex{-1};

    // Annotation Studio
    int m_annotToolMode{0}; // 0 = Select
    QColor m_annotColor{255, 235, 59, 180};
    float m_annotStrokeWidth{2.0f};
    QString m_stampName{"APPROVED"};
    services::AnnotationCommandManager* m_commandManager{nullptr};

    int m_selectedAnnotPageIndex{-1};
    int m_selectedAnnotIndex{-1};
    pdf_api::AnnotationData m_selectedAnnotData;

    bool m_isDrawingAnnot{false};
    QPointF m_drawStartPoint;
    QPointF m_drawCurrentPoint;
    std::vector<pdf_api::PdfPoint> m_currentInkStroke;

    static constexpr double MinZoom = 0.10;
    static constexpr double MaxZoom = 10.0;
    static constexpr double ZoomStep = 1.25;
    static constexpr double PageSpacing = 16.0;
};

} // namespace pdfinity::ui
