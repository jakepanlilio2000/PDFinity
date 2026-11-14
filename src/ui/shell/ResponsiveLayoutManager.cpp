// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ResponsiveLayoutManager.hpp"

namespace pdfinity::ui {

ResponsiveLayoutManager::ResponsiveLayoutManager(QObject* parent)
    : QObject(parent),
      m_forceMobile(IsMobilePlatform()),
      m_currentClass(ClassifyWidth(1280, m_forceMobile)) {}

bool ResponsiveLayoutManager::IsMobilePlatform() noexcept {
#if defined(__ANDROID__)
    return true;
#else
    return (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android);
#endif
}

ViewportClass ResponsiveLayoutManager::ClassifyWidth(int width, bool forceMobile) {
    if (forceMobile || width < BreakpointTablet) {
        return ViewportClass::PhoneMobile;
    }
    if (width < BreakpointDesktop) {
        return ViewportClass::TabletCompact;
    }
    return ViewportClass::DesktopExpanded;
}

void ResponsiveLayoutManager::SetForceMobile(bool force) {
    if (m_forceMobile == force) return;
    m_forceMobile = force;
    UpdateWidth(m_currentWidth);
}

void ResponsiveLayoutManager::UpdateWidth(int width) {
    m_currentWidth = width;
    ViewportClass newClass = ClassifyWidth(width, m_forceMobile);
    if (newClass != m_currentClass) {
        ViewportClass oldClass = m_currentClass;
        m_currentClass = newClass;
        emit ViewportClassChanged(newClass, oldClass);
    }
}

} // namespace pdfinity::ui
