// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QObject>
#include <QOperatingSystemVersion>

namespace pdfinity::ui {

enum class ViewportClass {
    DesktopExpanded, // >= 1100px: Full 3-pane Studio layout
    TabletCompact,   // 700px - 1099px: Auto-collapsed left drawer & right inspector
    PhoneMobile      // < 700px or Android handset: Top App Bar + Bottom Nav + Bottom Sheet
};

class ResponsiveLayoutManager : public QObject {
    Q_OBJECT

public:
    static constexpr int BreakpointTablet = 700;
    static constexpr int BreakpointDesktop = 1100;
    static constexpr int MinTouchTargetDp = 48;
    [[nodiscard]] static constexpr int GetMinTouchTargetSize() noexcept { return MinTouchTargetDp; }

    explicit ResponsiveLayoutManager(QObject* parent = nullptr);
    ~ResponsiveLayoutManager() override = default;

    static ViewportClass ClassifyWidth(int width, bool forceMobile = false);
    static bool IsMobilePlatform() noexcept;

    void UpdateWidth(int width);
    void SetForceMobile(bool force);

    [[nodiscard]] ViewportClass GetCurrentClass() const noexcept { return m_currentClass; }
    [[nodiscard]] int GetCurrentWidth() const noexcept { return m_currentWidth; }
    [[nodiscard]] bool IsMobile() const noexcept { return m_currentClass == ViewportClass::PhoneMobile; }
    [[nodiscard]] bool IsTablet() const noexcept { return m_currentClass == ViewportClass::TabletCompact; }
    [[nodiscard]] bool IsDesktop() const noexcept { return m_currentClass == ViewportClass::DesktopExpanded; }

signals:
    void ViewportClassChanged(ViewportClass newClass, ViewportClass oldClass);

private:
    int m_currentWidth{1280};
    ViewportClass m_currentClass{ViewportClass::DesktopExpanded};
    bool m_forceMobile{false};
};

} // namespace pdfinity::ui
