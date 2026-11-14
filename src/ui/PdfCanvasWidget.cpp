// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfCanvasWidget.hpp"
#include "AnnotationToolbar.hpp"
#include "AnnotationPropertiesDialog.hpp"
#include "viewer/StudioToast.hpp"
#include "shell/ResponsiveLayoutManager.hpp"
#include "services/AnnotationCommandManager.hpp"
#include "services/AnnotationService.hpp"
#include "application/Logger.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QTapAndHoldGesture>
#include <cmath>

namespace pdfinity::ui {

PdfCanvasWidget::PdfCanvasWidget(
    std::shared_ptr<domain::DocumentModel> doc,
    services::AsyncRenderService* renderService,
    QWidget* parent
) : QWidget(parent),
    m_doc(std::move(doc)),
    m_renderService(renderService) {

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::TapAndHoldGesture);

    if (m_doc && m_doc->IsValid()) {
        int pageCount = m_doc->GetPageCount();
        m_pageSizes.reserve(pageCount);
        for (int i = 0; i < pageCount; ++i) {
            auto sizeRes = m_doc->GetPageSize(i);
            if (sizeRes.has_value()) {
                m_pageSizes.push_back(sizeRes.value);
            } else {
                m_pageSizes.push_back(pdf_api::PdfSize{612.0, 792.0}); // Standard Letter fallback
            }
        }
    }

    if (m_renderService) {
        connect(m_renderService, &services::AsyncRenderService::PageRendered,
                this, &PdfCanvasWidget::OnPageRendered);
    }

    RecalculateLayout();
}

int PdfCanvasWidget::GetPageCount() const {
    return static_cast<int>(m_pageSizes.size());
}

void PdfCanvasWidget::ReloadPageSizes() {
    if (m_doc && m_doc->IsValid()) {
        int pageCount = m_doc->GetPageCount();
        m_pageSizes.clear();
        m_pageSizes.reserve(pageCount);
        for (int i = 0; i < pageCount; ++i) {
            auto sizeRes = m_doc->GetPageSize(i);
            if (sizeRes.has_value()) {
                m_pageSizes.push_back(sizeRes.value);
            } else {
                m_pageSizes.push_back(pdf_api::PdfSize{612.0, 792.0});
            }
        }
        RecalculateLayout();
    }
}

void PdfCanvasWidget::SetLayoutMode(pdf_api::PageLayoutMode mode) {
    if (m_layoutMode == mode) return;
    m_layoutMode = mode;
    RecalculateLayout();
}

void PdfCanvasWidget::SetZoomScale(double scale) {
    double clamped = std::clamp(scale, MinZoom, MaxZoom);
    if (std::abs(m_zoomScale - clamped) < 0.001) return;

    m_zoomScale = clamped;
    RecalculateLayout();
    emit ZoomChanged(m_zoomScale);
}

void PdfCanvasWidget::ZoomIn() {
    SetZoomScale(m_zoomScale * ZoomStep);
}

void PdfCanvasWidget::ZoomOut() {
    SetZoomScale(m_zoomScale / ZoomStep);
}

void PdfCanvasWidget::ZoomFitWidth(int viewportWidth) {
    if (m_pageSizes.empty() || viewportWidth <= 40) return;
    double targetWidth = m_pageSizes[static_cast<size_t>(m_currentPage)].width;
    if (m_layoutMode == pdf_api::PageLayoutMode::FacingPages) {
        targetWidth *= 2.0;
    }
    double newScale = static_cast<double>(viewportWidth - 48) / targetWidth;
    SetZoomScale(newScale);
}

void PdfCanvasWidget::ZoomFitPage(int viewportWidth, int viewportHeight) {
    if (m_pageSizes.empty() || viewportWidth <= 40 || viewportHeight <= 40) return;
    const auto& size = m_pageSizes[static_cast<size_t>(m_currentPage)];
    double scaleW = static_cast<double>(viewportWidth - 48) / size.width;
    double scaleH = static_cast<double>(viewportHeight - 48) / size.height;
    SetZoomScale(std::min(scaleW, scaleH));
}

void PdfCanvasWidget::GoToPage(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= GetPageCount()) return;
    m_currentPage = pageIndex;

    if (m_layoutMode == pdf_api::PageLayoutMode::SinglePage) {
        RecalculateLayout();
    } else {
        // Scroll parent QScrollArea to make page visible
        auto* scrollArea = qobject_cast<QScrollArea*>(parentWidget() ? parentWidget()->parentWidget() : nullptr);
        if (scrollArea && static_cast<size_t>(pageIndex) < m_pageLayouts.size()) {
            const auto& pageRect = m_pageLayouts[static_cast<size_t>(pageIndex)].rect;
            scrollArea->verticalScrollBar()->setValue(static_cast<int>(pageRect.top() - PageSpacing));
        }
    }

    emit CurrentPageChanged(m_currentPage);
    update();
}

void PdfCanvasWidget::SetSearchMatches(const std::vector<pdf_api::TextSearchResult>& matches, int activeIndex) {
    m_searchMatches = matches;
    m_activeMatchIndex = activeIndex;
    update();
}

void PdfCanvasWidget::SetActiveMatchIndex(int activeIndex) {
    m_activeMatchIndex = activeIndex;
    if (activeIndex >= 0 && static_cast<size_t>(activeIndex) < m_searchMatches.size()) {
        GoToPage(m_searchMatches[static_cast<size_t>(activeIndex)].pageIndex);
    }
    update();
}

void PdfCanvasWidget::RecalculateLayout() {
    m_pageLayouts.clear();
    int count = GetPageCount();
    if (count == 0) {
        setMinimumSize(400, 400);
        update();
        return;
    }

    double totalWidth = 0.0;
    double totalHeight = PageSpacing;
    int canvasWidth = std::max(width(), 400);

    if (m_layoutMode == pdf_api::PageLayoutMode::SinglePage) {
        size_t idx = static_cast<size_t>(m_currentPage);
        if (idx < m_pageSizes.size()) {
            const auto& size = m_pageSizes[idx];
            double pw = size.width * m_zoomScale;
            double ph = size.height * m_zoomScale;
            double x = std::max(PageSpacing, (canvasWidth - pw) / 2.0);
            double y = PageSpacing;

            m_pageLayouts.push_back(PageLayoutRect{m_currentPage, QRectF(x, y, pw, ph), size});
            totalWidth = pw + (2.0 * PageSpacing);
            totalHeight = ph + (2.0 * PageSpacing);
        }
    } else if (m_layoutMode == pdf_api::PageLayoutMode::Continuous) {
        for (int i = 0; i < count; ++i) {
            const auto& size = m_pageSizes[static_cast<size_t>(i)];
            double pw = size.width * m_zoomScale;
            double ph = size.height * m_zoomScale;
            double x = std::max(PageSpacing, (canvasWidth - pw) / 2.0);

            m_pageLayouts.push_back(PageLayoutRect{i, QRectF(x, totalHeight, pw, ph), size});
            totalWidth = std::max(totalWidth, pw + (2.0 * PageSpacing));
            totalHeight += ph + PageSpacing;
        }
    } else { // FacingPages
        for (int i = 0; i < count; i += 2) {
            const auto& size1 = m_pageSizes[static_cast<size_t>(i)];
            double pw1 = size1.width * m_zoomScale;
            double ph1 = size1.height * m_zoomScale;

            double pw2 = 0.0, ph2 = 0.0;
            if (i + 1 < count) {
                const auto& size2 = m_pageSizes[static_cast<size_t>(i + 1)];
                pw2 = size2.width * m_zoomScale;
                ph2 = size2.height * m_zoomScale;
            }

            double pairWidth = pw1 + (pw2 > 0 ? pw2 + PageSpacing : 0);
            double startX = std::max(PageSpacing, (canvasWidth - pairWidth) / 2.0);
            double maxHeight = std::max(ph1, ph2);

            m_pageLayouts.push_back(PageLayoutRect{i, QRectF(startX, totalHeight, pw1, ph1), size1});
            if (i + 1 < count) {
                m_pageLayouts.push_back(PageLayoutRect{i + 1, QRectF(startX + pw1 + PageSpacing, totalHeight, pw2, ph2), m_pageSizes[static_cast<size_t>(i + 1)]});
            }

            totalWidth = std::max(totalWidth, pairWidth + (2.0 * PageSpacing));
            totalHeight += maxHeight + PageSpacing;
        }
    }

    setMinimumSize(static_cast<int>(std::ceil(totalWidth)), static_cast<int>(std::ceil(totalHeight)));
    update();
}

bool PdfCanvasWidget::event(QEvent* event) {
    if (event->type() == QEvent::Gesture) {
        auto* gestureEvent = static_cast<QGestureEvent*>(event);
        if (QGesture* pinch = gestureEvent->gesture(Qt::PinchGesture)) {
            auto* pinchGesture = static_cast<QPinchGesture*>(pinch);
            if (pinchGesture->changeFlags() & QPinchGesture::ScaleFactorChanged) {
                double factor = pinchGesture->scaleFactor();
                if (factor > 0.0) {
                    SetZoomScale(m_zoomScale * factor);
                }
            }
            gestureEvent->accept(Qt::PinchGesture);
            return true;
        }
        if (QGesture* swipe = gestureEvent->gesture(Qt::SwipeGesture)) {
            auto* swipeGesture = static_cast<QSwipeGesture*>(swipe);
            if (swipeGesture->state() == Qt::GestureFinished) {
                if (swipeGesture->horizontalDirection() == QSwipeGesture::Left ||
                    swipeGesture->verticalDirection() == QSwipeGesture::Up) {
                    GoToPage(m_currentPage + 1);
                } else if (swipeGesture->horizontalDirection() == QSwipeGesture::Right ||
                           swipeGesture->verticalDirection() == QSwipeGesture::Down) {
                    GoToPage(m_currentPage - 1);
                }
                gestureEvent->accept(Qt::SwipeGesture);
                return true;
            }
        }
        if (QGesture* tapAndHold = gestureEvent->gesture(Qt::TapAndHoldGesture)) {
            auto* tapGesture = static_cast<QTapAndHoldGesture*>(tapAndHold);
            if (tapGesture->state() == Qt::GestureFinished) {
                QPoint pos = tapGesture->position().toPoint();
                QContextMenuEvent contextEv(QContextMenuEvent::Other, pos, mapToGlobal(pos));
                contextMenuEvent(&contextEv);
                gestureEvent->accept(Qt::TapAndHoldGesture);
                return true;
            }
        }
    }
    return QWidget::event(event);
}

void PdfCanvasWidget::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const bool isDark = palette().window().color().lightness() < 128;
    const QColor pasteboardColor = isDark ? QColor("#111214") : QColor("#E5E7EB");
    const QColor hairlineBorderColor = isDark ? QColor("#34373E") : QColor("#D1D5DB");

    // Neutral pasteboard backdrop fill
    p.fillRect(event->rect(), pasteboardColor);

    QRect clip = event->rect();
    double dpr = devicePixelRatioF();

    for (const auto& layout : m_pageLayouts) {
        if (!layout.rect.intersects(clip)) continue;

        // 1. Elevated Sheet Drop Shadow (Multi-layered soft ambient & directional blur)
        p.fillRect(layout.rect.adjusted(-6, 2, 6, 10), QColor(0, 0, 0, isDark ? 40 : 12));
        p.fillRect(layout.rect.adjusted(-3, 2, 3, 6),  QColor(0, 0, 0, isDark ? 65 : 22));
        p.fillRect(layout.rect.adjusted(-1, 1, 1, 2),  QColor(0, 0, 0, isDark ? 90 : 35));

        // 2. Crisp white sheet surface
        p.fillRect(layout.rect, Qt::white);

        // 3. Render or fetch page bitmap
        bool drawn = false;
        if (m_renderService) {
            auto cached = m_renderService->GetCache().Get(layout.pageIndex, m_zoomScale, dpr);
            if (cached.has_value()) {
                p.drawImage(layout.rect, cached.value());
                drawn = true;
            } else {
                // Asynchronously request render
                m_renderService->RequestPageRender(m_doc, layout.pageIndex, m_zoomScale, dpr);
            }
        }

        if (!drawn) {
            // Placeholder skeleton while rasterizing
            p.setPen(isDark ? QColor(100, 100, 105) : QColor(160, 160, 165));
            QFont font = p.font();
            font.setPointSize(12);
            p.setFont(font);
            p.drawText(layout.rect, Qt::AlignCenter, tr("Page %1 (Rendering...)").arg(layout.pageIndex + 1));
        }

        // 4. Subtle 1px hairline border over image so page bounds stand out
        p.setPen(QPen(hairlineBorderColor, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(layout.rect);

        // 4. Draw Search Highlights
        for (size_t m = 0; m < m_searchMatches.size(); ++m) {
            const auto& match = m_searchMatches[m];
            if (match.pageIndex != layout.pageIndex) continue;

            bool isActive = (static_cast<int>(m) == m_activeMatchIndex);
            QColor highlightColor = isActive ? QColor(255, 152, 0, 180) : QColor(255, 235, 59, 140);
            QColor borderColor = isActive ? QColor(230, 81, 0) : QColor(245, 127, 23);

            p.setPen(QPen(borderColor, 1));
            p.setBrush(highlightColor);

            for (const auto& rect : match.rects) {
                QRectF sRect = PdfToScreenRect(layout.pageIndex, rect);
                p.drawRect(sRect);
            }
        }

        // 5. Draw Active Selection Box
        if (m_isSelecting && m_selectionPageIndex == layout.pageIndex) {
            QRectF selRect(m_selectionStart, m_selectionEnd);
            selRect = selRect.normalized();
            p.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
            p.setBrush(QColor(0, 120, 215, 60));
            p.drawRect(selRect);
        }

        // 6. Draw Selected Annotation Box & Handles
        if (m_selectedAnnotIndex >= 0 && m_selectedAnnotPageIndex == layout.pageIndex) {
            QRectF annotScreenRect = PdfToScreenRect(layout.pageIndex, m_selectedAnnotData.rect);
            p.setPen(QPen(QColor(37, 99, 235), 2, Qt::SolidLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(annotScreenRect);

            // Draw corner handles
            p.setBrush(QColor(37, 99, 235));
            const double handleSize = ResponsiveLayoutManager::IsMobilePlatform() ? 12.0 : 6.0;
            p.drawRect(QRectF(annotScreenRect.left() - handleSize/2, annotScreenRect.top() - handleSize/2, handleSize, handleSize));
            p.drawRect(QRectF(annotScreenRect.right() - handleSize/2, annotScreenRect.top() - handleSize/2, handleSize, handleSize));
            p.drawRect(QRectF(annotScreenRect.left() - handleSize/2, annotScreenRect.bottom() - handleSize/2, handleSize, handleSize));
            p.drawRect(QRectF(annotScreenRect.right() - handleSize/2, annotScreenRect.bottom() - handleSize/2, handleSize, handleSize));
        }

        // 7. Draw Live Annotation Drawing Preview
        if (m_isDrawingAnnot && m_selectionPageIndex == layout.pageIndex) {
            p.setPen(QPen(m_annotColor, m_annotStrokeWidth * m_zoomScale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            auto toolMode = static_cast<AnnotationToolMode>(m_annotToolMode);

            if (toolMode == AnnotationToolMode::FreehandInk && m_currentInkStroke.size() > 1) {
                p.setBrush(Qt::NoBrush);
                for (size_t k = 0; k + 1 < m_currentInkStroke.size(); ++k) {
                    QPointF p1 = PdfToScreenPoint(layout.pageIndex, m_currentInkStroke[k]);
                    QPointF p2 = PdfToScreenPoint(layout.pageIndex, m_currentInkStroke[k + 1]);
                    p.drawLine(p1, p2);
                }
            } else if (toolMode == AnnotationToolMode::Rectangle || toolMode == AnnotationToolMode::Highlight ||
                       toolMode == AnnotationToolMode::Underline || toolMode == AnnotationToolMode::Strikeout ||
                       toolMode == AnnotationToolMode::FreeText || toolMode == AnnotationToolMode::Stamp) {
                QRectF dragR = QRectF(m_drawStartPoint, m_drawCurrentPoint).normalized();
                p.setBrush(toolMode == AnnotationToolMode::Highlight ? m_annotColor : QColor(m_annotColor.red(), m_annotColor.green(), m_annotColor.blue(), 40));
                p.drawRect(dragR);
            } else if (toolMode == AnnotationToolMode::Circle) {
                QRectF dragR = QRectF(m_drawStartPoint, m_drawCurrentPoint).normalized();
                p.setBrush(QColor(m_annotColor.red(), m_annotColor.green(), m_annotColor.blue(), 40));
                p.drawEllipse(dragR);
            } else if (toolMode == AnnotationToolMode::Line || toolMode == AnnotationToolMode::Arrow) {
                p.setBrush(Qt::NoBrush);
                p.drawLine(m_drawStartPoint, m_drawCurrentPoint);
            }
        }
    }

    // 8. Canvas Rulers & Alignment Snap Guides Overlay
    if (m_rulerOverlay.IsVisible() && m_currentPage >= 0 &&
        static_cast<size_t>(m_currentPage) < m_pageLayouts.size()) {
        const auto& curLayout = m_pageLayouts[static_cast<size_t>(m_currentPage)];
        m_rulerOverlay.DrawRulers(p, curLayout.rect, m_zoomScale, m_lastMousePos, isDark);
        if (m_rulerOverlay.AreSnapGuidesVisible()) {
            m_rulerOverlay.DrawSnapGuides(p, curLayout.rect, m_zoomScale, rect());
        }
    }
}

void PdfCanvasWidget::resizeEvent(QResizeEvent* /*event*/) {
    RecalculateLayout();
}

int PdfCanvasWidget::GetPageIndexAtPoint(const QPointF& pos) const {
    for (const auto& layout : m_pageLayouts) {
        if (layout.rect.contains(pos)) {
            return layout.pageIndex;
        }
    }
    return -1;
}

pdf_api::PdfRect PdfCanvasWidget::ScreenToPdfRect(int pageIndex, const QRectF& screenRect) const {
    if (pageIndex < 0 || static_cast<size_t>(pageIndex) >= m_pageSizes.size()) {
        return {};
    }

    const auto& size = m_pageSizes[static_cast<size_t>(pageIndex)];
    for (const auto& layout : m_pageLayouts) {
        if (layout.pageIndex == pageIndex) {
            double rx = (screenRect.left() - layout.rect.left()) / m_zoomScale;
            double rw = screenRect.width() / m_zoomScale;
            double rh = screenRect.height() / m_zoomScale;
            double ry = size.height - ((screenRect.bottom() - layout.rect.top()) / m_zoomScale);

            return pdf_api::PdfRect{rx, ry, rw, rh};
        }
    }
    return {};
}

QRectF PdfCanvasWidget::PdfToScreenRect(int pageIndex, const pdf_api::PdfRect& pdfRect) const {
    if (pageIndex < 0 || static_cast<size_t>(pageIndex) >= m_pageSizes.size()) {
        return {};
    }

    const auto& size = m_pageSizes[static_cast<size_t>(pageIndex)];
    for (const auto& layout : m_pageLayouts) {
        if (layout.pageIndex == pageIndex) {
            double sx = layout.rect.left() + (pdfRect.x * m_zoomScale);
            double sy = layout.rect.top() + ((size.height - (pdfRect.y + pdfRect.height)) * m_zoomScale);
            double sw = pdfRect.width * m_zoomScale;
            double sh = pdfRect.height * m_zoomScale;

            return QRectF(sx, sy, sw, sh);
        }
    }
    return {};
}

pdf_api::PdfPoint PdfCanvasWidget::ScreenToPdfPoint(int pageIndex, const QPointF& screenPoint) const {
    if (pageIndex < 0 || static_cast<size_t>(pageIndex) >= m_pageSizes.size()) {
        return {};
    }
    const auto& size = m_pageSizes[static_cast<size_t>(pageIndex)];
    for (const auto& layout : m_pageLayouts) {
        if (layout.pageIndex == pageIndex) {
            double rx = (screenPoint.x() - layout.rect.left()) / m_zoomScale;
            double ry = size.height - ((screenPoint.y() - layout.rect.top()) / m_zoomScale);
            return pdf_api::PdfPoint{rx, ry};
        }
    }
    return {};
}

QPointF PdfCanvasWidget::PdfToScreenPoint(int pageIndex, const pdf_api::PdfPoint& pdfPoint) const {
    if (pageIndex < 0 || static_cast<size_t>(pageIndex) >= m_pageSizes.size()) {
        return {};
    }
    const auto& size = m_pageSizes[static_cast<size_t>(pageIndex)];
    for (const auto& layout : m_pageLayouts) {
        if (layout.pageIndex == pageIndex) {
            double sx = layout.rect.left() + (pdfPoint.x * m_zoomScale);
            double sy = layout.rect.top() + ((size.height - pdfPoint.y) * m_zoomScale);
            return QPointF(sx, sy);
        }
    }
    return {};
}

void PdfCanvasWidget::SetPointerMode(CanvasPointerMode mode) {
    m_pointerMode = mode;
    if (mode == CanvasPointerMode::HandPan) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    update();
}

void PdfCanvasWidget::SetRulersVisible(bool visible) {
    m_rulerOverlay.SetVisible(visible);
    update();
}

void PdfCanvasWidget::SetSnapGuidesVisible(bool visible) {
    m_rulerOverlay.SetSnapGuidesVisible(visible);
    update();
}

void PdfCanvasWidget::SetRulerUnit(RulerUnit unit) {
    m_rulerOverlay.SetUnit(unit);
    update();
}

void PdfCanvasWidget::SetAnnotationToolMode(int mode) {
    m_annotToolMode = mode;
    m_selectedAnnotIndex = -1;
    m_selectedAnnotPageIndex = -1;
    update();
}

void PdfCanvasWidget::SetAnnotationColor(const QColor& color) {
    m_annotColor = color;
}

void PdfCanvasWidget::SetAnnotationStrokeWidth(float width) {
    m_annotStrokeWidth = width;
}

void PdfCanvasWidget::SetStampName(const QString& name) {
    m_stampName = name;
}

void PdfCanvasWidget::SetCommandManager(services::AnnotationCommandManager* cmdMgr) {
    m_commandManager = cmdMgr;
}

void PdfCanvasWidget::SelectAnnotation(int pageIndex, int annotIndex) {
    if (!m_doc || !m_doc->IsValid() || pageIndex < 0) return;
    auto annotsRes = m_doc->GetAnnotations(pageIndex);
    if (!annotsRes.has_value()) return;

    for (const auto& a : annotsRes.value) {
        if (a.index == annotIndex) {
            m_selectedAnnotPageIndex = pageIndex;
            m_selectedAnnotIndex = annotIndex;
            m_selectedAnnotData = a;
            emit AnnotationSelected(pageIndex, annotIndex);
            update();
            break;
        }
    }
}

void PdfCanvasWidget::DeleteSelectedAnnotation() {
    if (!m_doc || m_selectedAnnotIndex < 0 || m_selectedAnnotPageIndex < 0 || !m_commandManager) return;
    auto annotsRes = m_doc->GetAnnotations(m_selectedAnnotPageIndex);
    if (!annotsRes.has_value() || m_selectedAnnotIndex >= static_cast<int>(annotsRes.value.size())) return;

    const auto& saved = annotsRes.value[static_cast<size_t>(m_selectedAnnotIndex)];
    m_commandManager->ExecuteCommand(std::make_unique<services::DeleteAnnotationCommand>(
        m_doc, m_selectedAnnotPageIndex, m_selectedAnnotIndex, saved
    ));

    m_selectedAnnotIndex = -1;
    m_selectedAnnotPageIndex = -1;

    if (m_renderService) {
        m_renderService->GetCache().Clear();
        m_renderService->RequestPageRender(m_doc, m_currentPage, m_zoomScale, devicePixelRatioF(), 10);
    }
    emit AnnotationModified();
    update();
}

void PdfCanvasWidget::OpenSelectedAnnotationProperties() {
    if (!m_doc || m_selectedAnnotIndex < 0 || m_selectedAnnotPageIndex < 0) return;
    auto annotsRes = m_doc->GetAnnotations(m_selectedAnnotPageIndex);
    if (!annotsRes.has_value() || m_selectedAnnotIndex >= static_cast<int>(annotsRes.value.size())) return;

    const auto& currentData = annotsRes.value[static_cast<size_t>(m_selectedAnnotIndex)];
    AnnotationPropertiesDialog dlg(currentData, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.DeleteRequested()) {
            DeleteSelectedAnnotation();
        } else if (m_commandManager) {
            auto updated = dlg.GetUpdatedData();
            m_commandManager->ExecuteCommand(std::make_unique<services::ModifyAnnotationCommand>(
                m_doc, m_selectedAnnotPageIndex, m_selectedAnnotIndex, currentData, updated
            ));
            if (m_renderService) {
                m_renderService->GetCache().Clear();
                m_renderService->RequestPageRender(m_doc, m_selectedAnnotPageIndex, m_zoomScale, devicePixelRatioF(), 10);
            }
            emit AnnotationModified();
            update();
        }
    }
}

void PdfCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_pointerMode == CanvasPointerMode::HandPan) {
            m_isPanning = true;
            m_panStartPos = event->globalPosition().toPoint();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        int page = GetPageIndexAtPoint(event->position());
        if (page != -1) {
            // Check if Annotation Tool is active
            if (m_annotToolMode == static_cast<int>(AnnotationToolMode::Select)) {
                // Hit-test existing annotations
                if (m_doc && m_doc->IsValid()) {
                    auto annotsRes = m_doc->GetAnnotations(page);
                    if (annotsRes.has_value()) {
                        pdf_api::PdfPoint pdfPt = ScreenToPdfPoint(page, event->position());
                        for (const auto& a : annotsRes.value) {
                            if (pdfPt.x >= a.rect.x && pdfPt.x <= a.rect.x + a.rect.width &&
                                pdfPt.y >= a.rect.y && pdfPt.y <= a.rect.y + a.rect.height) {
                                m_selectedAnnotPageIndex = page;
                                m_selectedAnnotIndex = a.index;
                                m_selectedAnnotData = a;
                                emit AnnotationSelected(page, a.index);
                                update();
                                return;
                            }
                        }
                    }
                }
                m_selectedAnnotPageIndex = -1;
                m_selectedAnnotIndex = -1;

                // Normal text selection
                m_isSelecting = true;
                m_selectionPageIndex = page;
                m_selectionStart = event->position();
                m_selectionEnd = event->position();
                m_selectedText.clear();
                update();
            } else {
                // Annotation drawing tool active
                m_isDrawingAnnot = true;
                m_selectionPageIndex = page;
                m_drawStartPoint = event->position();
                m_drawCurrentPoint = event->position();
                m_currentInkStroke.clear();
                m_currentInkStroke.push_back(ScreenToPdfPoint(page, event->position()));
                update();
                return;
            }
        } else {
            m_isSelecting = false;
            m_selectionPageIndex = -1;
            m_selectedText.clear();
            m_selectedAnnotPageIndex = -1;
            m_selectedAnnotIndex = -1;
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void PdfCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    m_lastMousePos = event->position();
    if (m_rulerOverlay.IsVisible()) {
        update();
    }

    if (m_isPanning && (event->buttons() & Qt::LeftButton)) {
        QPoint currentPos = event->globalPosition().toPoint();
        QPoint delta = currentPos - m_panStartPos;
        m_panStartPos = currentPos;

        QWidget* p = parentWidget();
        while (p) {
            if (auto* sa = qobject_cast<QScrollArea*>(p)) {
                sa->horizontalScrollBar()->setValue(sa->horizontalScrollBar()->value() - delta.x());
                sa->verticalScrollBar()->setValue(sa->verticalScrollBar()->value() - delta.y());
                break;
            }
            p = p->parentWidget();
        }
        event->accept();
        return;
    }

    if (m_isDrawingAnnot) {
        m_drawCurrentPoint = event->position();
        int page = m_selectionPageIndex;
        if (page >= 0 && m_annotToolMode == static_cast<int>(AnnotationToolMode::FreehandInk)) {
            m_currentInkStroke.push_back(ScreenToPdfPoint(page, event->position()));
        }
        update();
        return;
    }

    if (m_isSelecting && (event->buttons() & Qt::LeftButton)) {
        m_selectionEnd = event->position();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void PdfCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        if (m_pointerMode == CanvasPointerMode::HandPan) {
            setCursor(Qt::OpenHandCursor);
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_isDrawingAnnot) {
        m_isDrawingAnnot = false;
        int page = m_selectionPageIndex;
        if (page >= 0 && m_doc && m_doc->IsValid() && m_commandManager) {
            QRectF screenR = QRectF(m_drawStartPoint, m_drawCurrentPoint).normalized();
            pdf_api::PdfRect pdfR = ScreenToPdfRect(page, screenR);
            if (pdfR.width < 5.0 && pdfR.height < 5.0) {
                pdfR.width = 120.0;
                pdfR.height = 40.0;
            }

            pdf_api::PdfColor color{
                static_cast<uint8_t>(m_annotColor.red()),
                static_cast<uint8_t>(m_annotColor.green()),
                static_cast<uint8_t>(m_annotColor.blue()),
                static_cast<uint8_t>(m_annotColor.alpha())
            };

            pdf_api::AnnotationData newAnnot;
            auto toolMode = static_cast<AnnotationToolMode>(m_annotToolMode);

            switch (toolMode) {
                case AnnotationToolMode::Highlight:
                    newAnnot = services::AnnotationService::CreateHighlight(page, pdfR, {pdfR}, color, "");
                    break;
                case AnnotationToolMode::Underline:
                    newAnnot = services::AnnotationService::CreateUnderline(page, pdfR, {pdfR}, color, "");
                    break;
                case AnnotationToolMode::Strikeout:
                    newAnnot = services::AnnotationService::CreateStrikeout(page, pdfR, {pdfR}, color, "");
                    break;
                case AnnotationToolMode::StickyNote:
                    newAnnot = services::AnnotationService::CreateStickyNote(page, ScreenToPdfPoint(page, m_drawStartPoint), "Sticky Note Comment", "PDFinity User", color);
                    break;
                case AnnotationToolMode::FreehandInk:
                    if (!m_currentInkStroke.empty()) {
                        newAnnot = services::AnnotationService::CreateFreehand(page, {m_currentInkStroke}, color, m_annotStrokeWidth);
                    }
                    break;
                case AnnotationToolMode::Rectangle:
                    newAnnot = services::AnnotationService::CreateShape(page, pdf_api::AnnotationType::Square, pdfR, color, {0,0,0,0}, m_annotStrokeWidth);
                    break;
                case AnnotationToolMode::Circle:
                    newAnnot = services::AnnotationService::CreateShape(page, pdf_api::AnnotationType::Circle, pdfR, color, {0,0,0,0}, m_annotStrokeWidth);
                    break;
                case AnnotationToolMode::Line:
                    newAnnot = services::AnnotationService::CreateShape(page, pdf_api::AnnotationType::Line, pdfR, color, {0,0,0,0}, m_annotStrokeWidth);
                    break;
                case AnnotationToolMode::Arrow:
                    newAnnot = services::AnnotationService::CreateShape(page, pdf_api::AnnotationType::Arrow, pdfR, color, {0,0,0,0}, m_annotStrokeWidth);
                    break;
                case AnnotationToolMode::Stamp:
                    newAnnot = services::AnnotationService::CreateStamp(page, pdfR, m_stampName.toStdString());
                    break;
                case AnnotationToolMode::FreeText:
                    newAnnot = services::AnnotationService::CreateFreeText(page, pdfR, "Double-click to edit text...", color, {255, 255, 255, 220});
                    break;
                default:
                    break;
            }

            if (newAnnot.type != pdf_api::AnnotationType::Unknown) {
                m_commandManager->ExecuteCommand(std::make_unique<services::AddAnnotationCommand>(m_doc, page, newAnnot));
                if (m_renderService) {
                    m_renderService->GetCache().Clear();
                    m_renderService->RequestPageRender(m_doc, page, m_zoomScale, devicePixelRatioF(), 10);
                }
                emit AnnotationModified();
                update();
            }
        }
        return;
    }

    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        m_selectionEnd = event->position();

        QRectF screenSel(m_selectionStart, m_selectionEnd);
        screenSel = screenSel.normalized();

        if (screenSel.width() > 4 && screenSel.height() > 4 && m_selectionPageIndex >= 0) {
            pdf_api::PdfRect pdfBounds = ScreenToPdfRect(m_selectionPageIndex, screenSel);
            if (m_doc && m_doc->IsValid() && m_doc->GetPermissions().canCopyText) {
                auto textRes = m_doc->GetBoundedText(m_selectionPageIndex, pdfBounds);
                if (textRes.has_value()) {
                    m_selectedText = QString::fromStdString(textRes.value).trimmed();
                    emit TextSelected(m_selectedText);
                }
            }
        }
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void PdfCanvasWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_selectedAnnotIndex >= 0 && m_selectedAnnotPageIndex >= 0) {
        OpenSelectedAnnotationProperties();
    } else {
        // Smart double-tap zoom: toggle between standard view and 1.5x zoom
        if (m_zoomScale < 1.35) {
            SetZoomScale(1.50);
        } else {
            SetZoomScale(1.00);
        }
        event->accept();
    }
}

void PdfCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_selectedAnnotIndex >= 0 && m_selectedAnnotPageIndex >= 0) {
        DeleteSelectedAnnotation();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void PdfCanvasWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    auto* copyAction = menu.addAction(tr("Copy Selected Text"));
    copyAction->setShortcut(QKeySequence::Copy);
    copyAction->setEnabled(!m_selectedText.isEmpty() && m_doc->GetPermissions().canCopyText);
    connect(copyAction, &QAction::triggered, this, &PdfCanvasWidget::OnCopySelection);

    menu.addSeparator();
    auto* fitWidthAction = menu.addAction(tr("Fit Width"));
    connect(fitWidthAction, &QAction::triggered, [this]() {
        ZoomFitWidth(width());
    });

    auto* fitPageAction = menu.addAction(tr("Fit Page"));
    connect(fitPageAction, &QAction::triggered, [this]() {
        ZoomFitPage(width(), height());
    });

    menu.exec(event->globalPos());
}

void PdfCanvasWidget::OnCopySelection() {
    if (!m_selectedText.isEmpty() && m_doc->GetPermissions().canCopyText) {
        QGuiApplication::clipboard()->setText(m_selectedText);
        LOG_INFO("Viewer", "Copied text to clipboard (" + std::to_string(m_selectedText.length()) + " chars)");
        StudioToast::Show(this, tr("Copied"), tr("Selected text copied to clipboard."), ToastType::Info);
    }
}

void PdfCanvasWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int delta = event->angleDelta().y();
        if (delta > 0) {
            ZoomIn();
        } else if (delta < 0) {
            ZoomOut();
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void PdfCanvasWidget::OnPageRendered(int pageIndex, double /*zoomScale*/, const QImage& /*image*/) {
    // Redraw only if page is visible in layout
    for (const auto& layout : m_pageLayouts) {
        if (layout.pageIndex == pageIndex) {
            update(layout.rect.toRect().adjusted(-4, -4, 8, 8));
            break;
        }
    }
}

} // namespace pdfinity::ui
