// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ThemeManager.hpp"
#include "theme/ThemeTokens.hpp"
#include "application/AppContext.hpp"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace pdfinity::ui {

bool ThemeManager::IsEffectiveDark(app::AppThemeMode mode) {
    if (mode == app::AppThemeMode::Dark) return true;
    if (mode == app::AppThemeMode::Light) return false;

    // System mode: query platform adapter
    auto sysTheme = app::AppContext::Instance().GetPlatform().GetSystemTheme();
    return (sysTheme == platform::SystemTheme::Dark);
}

void ThemeManager::ApplyTheme(app::AppThemeMode mode) {
    bool isDark = IsEffectiveDark(mode);

    // Apply Fusion base style for consistent cross-platform metrics
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    const auto& p = ThemeTokens::Get(isDark);
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(p.appChrome));
    palette.setColor(QPalette::WindowText, QColor(p.primaryText));
    palette.setColor(QPalette::Base, QColor(p.cardSurface));
    palette.setColor(QPalette::AlternateBase, QColor(p.canvasBackdrop));
    palette.setColor(QPalette::ToolTipBase, QColor(p.cardSurface));
    palette.setColor(QPalette::ToolTipText, QColor(p.primaryText));
    palette.setColor(QPalette::Text, QColor(p.primaryText));
    palette.setColor(QPalette::Button, QColor(p.toolbarRibbon));
    palette.setColor(QPalette::ButtonText, QColor(p.primaryText));
    palette.setColor(QPalette::BrightText, QColor(p.primaryCrimson));
    palette.setColor(QPalette::Link, QColor(p.primaryCrimson));
    palette.setColor(QPalette::Highlight, QColor(p.primaryCrimson));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(p.secondaryText));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(p.secondaryText));

    QApplication::setPalette(palette);
    qApp->setStyleSheet(GetStyleSheet(isDark));
}

QString ThemeManager::GetStyleSheet(bool isDark) {
    return ThemeTokens::GenerateStyleSheet(isDark);
}

} // namespace pdfinity::ui
