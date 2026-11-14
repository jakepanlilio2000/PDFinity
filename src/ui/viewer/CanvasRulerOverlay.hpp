// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QPainter>
#include <vector>

namespace pdfinity::ui {

enum class RulerUnit {
    Points,     // 72 pt = 1 in
    Inches,     // 1 in
    Millimeters // 25.4 mm = 1 in
};

struct SnapGuideLine {
    bool isVertical{true};
    double position{0.0};
    QColor color{220, 38, 38, 180}; // Crimson guideline
};

class CanvasRulerOverlay {
public:
    CanvasRulerOverlay() = default;

    void SetUnit(RulerUnit unit) { m_unit = unit; }
    [[nodiscard]] RulerUnit GetUnit() const noexcept { return m_unit; }

    void SetVisible(bool visible) { m_visible = visible; }
    [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

    void SetSnapGuidesVisible(bool visible) { m_guidesVisible = visible; }
    [[nodiscard]] bool AreSnapGuidesVisible() const noexcept { return m_guidesVisible; }

    void AddGuide(bool isVertical, double pos);
    void ClearGuides();

    double Snap(double coord, bool isVertical, double threshold = 6.0) const;

    void DrawRulers(
        QPainter& p,
        const QRectF& pageRect,
        double zoomScale,
        const QPointF& cursorScreenPos,
        bool isDarkMode
    );

    void DrawSnapGuides(
        QPainter& p,
        const QRectF& pageRect,
        double zoomScale,
        const QRectF& viewportRect
    );

private:
    RulerUnit m_unit{RulerUnit::Points};
    bool m_visible{false};
    bool m_guidesVisible{true};
    std::vector<SnapGuideLine> m_guides;
};

} // namespace pdfinity::ui