// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QString>
#include <QColor>

namespace pdfinity::ui {

struct ThemePalette {
    QString appChrome;
    QString toolbarRibbon;
    QString sidePanel;
    QString canvasBackdrop;
    QString cardSurface;
    QString hairlineBorder;
    QString primaryText;
    QString secondaryText;

    // Brand Accent Tokens (Universal Crimson)
    QString primaryCrimson{"#DC2626"};
    QString hoverCrimson{"#B91C1C"};
    QString softCrimsonTint{"rgba(220, 38, 38, 0.12)"};
    QString focusRing{"#EF4444"};
};

class ThemeTokens {
public:
    static const ThemePalette& Dark();
    static const ThemePalette& Light();
    static const ThemePalette& Get(bool isDark);

    static QString GetFontFamily();
    static QString GenerateStyleSheet(bool isDark);
};

} // namespace pdfinity::ui
