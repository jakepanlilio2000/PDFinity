// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "CanvasRulerOverlay.hpp"
#include <cmath>

namespace pdfinity::ui {

void CanvasRulerOverlay::AddGuide(bool isVertical, double pos) {
    m_guides.push_back({isVertical, pos, QColor(220, 38, 38, 190)});
}

void CanvasRulerOverlay::ClearGuides() {
    m_guides.clear();
}

double CanvasRulerOverlay::Snap(double coord, bool isVertical, double threshold) const {
    if (!m_guidesVisible || m_guides.empty()) return coord;

    double closestDist = threshold + 1.0;
    double snapped = coord;

    for (const auto& g : m_guides) {
        if (g.isVertical == isVertical) {
            double dist = std::abs(coord - g.position);
            if (dist <= threshold && dist < closestDist) {
                closestDist = dist;
                snapped = g.position;
            }
        }
    }
    return snapped;
}

void CanvasRulerOverlay::DrawRulers(
    QPainter& p,
    const QRectF& pageRect,
    double zoomScale,
    const QPointF& cursorScreenPos,
    bool isDarkMode
) {
    if (!m_visible) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, false);

    const double rulerThickness = 18.0;
    QColor bg = isDarkMode ? QColor(30, 32, 36, 230) : QColor(243, 244, 246, 230);
    QColor lineCol = isDarkMode ? QColor(80, 83, 92) : QColor(190, 194, 200);
    QColor textCol = isDarkMode ? QColor(161, 161, 170) : QColor(107, 114, 128);

    // Units scaling factors: Points, Inches, Millimeters
    double unitStep = 72.0; // 72 pt = 1 inch
    if (m_unit == RulerUnit::Inches) {
        unitStep = 72.0;
    } else if (m_unit == RulerUnit::Millimeters) {
        unitStep = 72.0 / 25.4 * 10.0; // 10mm
    }

    double screenStep = unitStep * zoomScale;
    while (screenStep < 25.0) screenStep *= 2.0;
    while (screenStep > 120.0) screenStep /= 2.0;

    // Top Horizontal Ruler bar
    QRectF topRuler(pageRect.left(), pageRect.top() - rulerThickness, pageRect.width(), rulerThickness);
    p.fillRect(topRuler, bg);
    p.setPen(lineCol);
    p.drawLine(topRuler.bottomLeft(), topRuler.bottomRight());

    // Left Vertical Ruler bar
    QRectF leftRuler(pageRect.left() - rulerThickness, pageRect.top(), rulerThickness, pageRect.height());
    p.fillRect(leftRuler, bg);
    p.setPen(lineCol);
    p.drawLine(leftRuler.topRight(), leftRuler.bottomRight());

    QFont font = p.font();
    font.setPixelSize(8);
    p.setFont(font);

    // Draw horizontal ticks
    int numTicksX = static_cast<int>(pageRect.width() / screenStep);
    for (int i = 0; i <= numTicksX; ++i) {
        double x = pageRect.left() + (i * screenStep);
        p.setPen(lineCol);
        p.drawLine(QPointF(x, topRuler.bottom() - 6), QPointF(x, topRuler.bottom()));

        // Half tick
        p.drawLine(QPointF(x + screenStep / 2.0, topRuler.bottom() - 3), QPointF(x + screenStep / 2.0, topRuler.bottom()));

        p.setPen(textCol);
        p.drawText(QRectF(x + 2, topRuler.top() + 1, 24, 10), Qt::AlignLeft, QString::number(i));
    }

    // Draw vertical ticks
    int numTicksY = static_cast<int>(pageRect.height() / screenStep);
    for (int i = 0; i <= numTicksY; ++i) {
        double y = pageRect.top() + (i * screenStep);
        p.setPen(lineCol);
        p.drawLine(QPointF(leftRuler.right() - 6, y), QPointF(leftRuler.right(), y));

        p.drawLine(QPointF(leftRuler.right() - 3, y + screenStep / 2.0), QPointF(leftRuler.right(), y + screenStep / 2.0));

        p.setPen(textCol);
        p.drawText(QRectF(leftRuler.left() + 2, y + 2, 14, 10), Qt::AlignLeft, QString::number(i));
    }

    // Cursor position indicator
    if (pageRect.contains(cursorScreenPos)) {
        p.setPen(QPen(QColor(220, 38, 38), 1));
        // Top tick indicator
        p.drawLine(QPointF(cursorScreenPos.x(), topRuler.top()), QPointF(cursorScreenPos.x(), topRuler.bottom()));
        // Left tick indicator
        p.drawLine(QPointF(leftRuler.left(), cursorScreenPos.y()), QPointF(leftRuler.right(), cursorScreenPos.y()));
    }

    p.restore();
}

void CanvasRulerOverlay::DrawSnapGuides(
    QPainter& p,
    const QRectF& pageRect,
    double zoomScale,
    const QRectF& viewportRect
) {
    if (!m_guidesVisible || m_guides.empty()) return;

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    for (const auto& g : m_guides) {
        QPen pen(g.color, 1, Qt::DashLine);
        p.setPen(pen);

        if (g.isVertical) {
            double screenX = pageRect.left() + (g.position * zoomScale);
            p.drawLine(QPointF(screenX, viewportRect.top()), QPointF(screenX, viewportRect.bottom()));
        } else {
            double screenY = pageRect.top() + (g.position * zoomScale);
            p.drawLine(QPointF(viewportRect.left(), screenY), QPointF(viewportRect.right(), screenY));
        }
    }

    p.restore();
}

} // namespace pdfinity::ui