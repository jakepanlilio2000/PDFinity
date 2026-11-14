// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ThemeTokens.hpp"
#include <QtGlobal>

namespace pdfinity::ui {

const ThemePalette& ThemeTokens::Dark() {
    static const ThemePalette dark = {
        "#18181B",                  // appChrome
        "#222428",                  // toolbarRibbon
        "#1E2024",                  // sidePanel
        "#111214",                  // canvasBackdrop
        "#27292E",                  // cardSurface
        "#34373E",                  // hairlineBorder
        "#F4F4F5",                  // primaryText
        "#A1A1AA",                  // secondaryText
        "#DC2626",                  // primaryCrimson
        "#B91C1C",                  // hoverCrimson
        "rgba(220, 38, 38, 0.15)",  // softCrimsonTint
        "#EF4444"                   // focusRing
    };
    return dark;
}

const ThemePalette& ThemeTokens::Light() {
    static const ThemePalette light = {
        "#F3F4F6",                  // appChrome
        "#FFFFFF",                  // toolbarRibbon
        "#F9FAFB",                  // sidePanel
        "#E5E7EB",                  // canvasBackdrop
        "#FFFFFF",                  // cardSurface
        "#D1D5DB",                  // hairlineBorder
        "#111827",                  // primaryText
        "#4B5563",                  // secondaryText
        "#DC2626",                  // primaryCrimson
        "#B91C1C",                  // hoverCrimson
        "rgba(220, 38, 38, 0.12)",  // softCrimsonTint
        "#EF4444"                   // focusRing
    };
    return light;
}

const ThemePalette& ThemeTokens::Get(bool isDark) {
    return isDark ? Dark() : Light();
}

QString ThemeTokens::GetFontFamily() {
#if defined(Q_OS_WIN)
    return "\"Segoe UI\", -apple-system, sans-serif";
#elif defined(Q_OS_MAC)
    return "-apple-system, \"SF Pro Text\", sans-serif";
#elif defined(Q_OS_ANDROID)
    return "Roboto, sans-serif";
#else
    return "Inter, \"Noto Sans\", sans-serif";
#endif
}

QString ThemeTokens::GenerateStyleSheet(bool isDark) {
    const auto& p = Get(isDark);
    const QString font = GetFontFamily();

    return QString(R"(
        * {
            font-family: %1;
        }

        QMainWindow {
            background-color: %2;
        }

        /* Top Menu Bar */
        QMenuBar {
            background-color: %3;
            color: %4;
            border-bottom: 1px solid %5;
            padding: 3px 6px;
            font-size: 12px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: %6;
            color: #FFFFFF;
        }

        /* Context and Popup Menus */
        QMenu {
            background-color: %7;
            color: %4;
            border: 1px solid %5;
            padding: 4px;
            border-radius: 6px;
            font-size: 13px;
        }
        QMenu::item {
            padding: 6px 20px 6px 10px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: %6;
            color: #FFFFFF;
        }
        QMenu::separator {
            height: 1px;
            background-color: %5;
            margin: 4px 6px;
        }

        /* Toolbars & Ribbons */
        QToolBar {
            background-color: %3;
            border-bottom: 1px solid %5;
            padding: 3px 6px;
            spacing: 4px;
        }
        QToolButton {
            background-color: transparent;
            color: %4;
            border: 1px solid transparent;
            border-radius: 5px;
            padding: 5px 8px;
            font-size: 12px;
            font-weight: 500;
        }
        QToolButton:hover {
            background-color: %8;
            border: 1px solid %5;
        }
        QToolButton:pressed, QToolButton:checked {
            background-color: %6;
            color: #FFFFFF;
            border-color: %9;
        }

        /* Status Bar */
        QStatusBar {
            background-color: %2;
            color: %10;
            border-top: 1px solid %5;
            font-size: 11px;
            padding: 2px 8px;
        }

        /* Tabs */
        QTabWidget::pane {
            border: none;
            background-color: %2;
        }
        QTabBar::tab {
            background-color: %3;
            color: %10;
            border: 1px solid %5;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            padding: 6px 14px;
            margin-right: 2px;
            font-size: 11px;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            background-color: %7;
            color: %4;
            font-weight: 600;
            border-top: 2px solid %6;
        }
        QTabBar::tab:hover:!selected {
            background-color: %8;
        }

        /* Push Buttons */
        QPushButton {
            background-color: %7;
            color: %4;
            border: 1px solid %5;
            border-radius: 5px;
            padding: 6px 14px;
            font-size: 13px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: %8;
            border-color: %6;
        }
        QPushButton:pressed {
            background-color: %6;
            color: #FFFFFF;
        }
        QPushButton#primaryButton {
            background-color: %6;
            border: 1px solid %9;
            color: #FFFFFF;
            font-weight: 600;
        }
        QPushButton#primaryButton:hover {
            background-color: %9;
        }
        QPushButton#primaryButton:pressed {
            background-color: #7F1D1D;
        }

        /* Input Controls */
        QLineEdit, QSpinBox, QComboBox {
            background-color: %7;
            color: %4;
            border: 1px solid %5;
            border-radius: 5px;
            padding: 5px 8px;
            font-size: 13px;
            selection-background-color: %6;
            selection-color: #FFFFFF;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid %11;
        }

        /* Scroll Area and Lists */
        QScrollArea {
            background-color: %2;
            border: none;
        }
        QListWidget, QTableWidget {
            background-color: %7;
            color: %4;
            border: 1px solid %5;
            border-radius: 6px;
            gridline-color: %5;
            font-size: 13px;
            selection-background-color: %6;
            selection-color: #FFFFFF;
        }
        QHeaderView::section {
            background-color: %3;
            color: %10;
            border: none;
            border-bottom: 1px solid %5;
            padding: 6px 10px;
            font-size: 11px;
            font-weight: 600;
        }
        QListWidget::item:hover, QTableWidget::item:hover {
            background-color: %8;
        }
        QListWidget::item:selected, QTableWidget::item:selected {
            background-color: %6;
            color: #FFFFFF;
        }

        /* Splitter Handle */
        QSplitter::handle {
            background-color: %5;
        }
        QSplitter::handle:hover {
            background-color: %6;
        }
    )")
    .arg(font)                  // %1
    .arg(p.appChrome)           // %2
    .arg(p.toolbarRibbon)       // %3
    .arg(p.primaryText)         // %4
    .arg(p.hairlineBorder)      // %5
    .arg(p.primaryCrimson)      // %6
    .arg(p.cardSurface)         // %7
    .arg(isDark ? "#2D3036" : "#E5E7EB") // %8 hover background
    .arg(p.hoverCrimson)        // %9
    .arg(p.secondaryText)       // %10
    .arg(p.focusRing);          // %11
}

} // namespace pdfinity::ui
