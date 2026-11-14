// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ThemeManager.hpp"
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

    QPalette palette;
    if (isDark) {
        palette.setColor(QPalette::Window, QColor(30, 32, 36));
        palette.setColor(QPalette::WindowText, QColor(240, 240, 245));
        palette.setColor(QPalette::Base, QColor(22, 24, 28));
        palette.setColor(QPalette::AlternateBase, QColor(36, 38, 44));
        palette.setColor(QPalette::ToolTipBase, QColor(240, 240, 245));
        palette.setColor(QPalette::ToolTipText, QColor(240, 240, 245));
        palette.setColor(QPalette::Text, QColor(240, 240, 245));
        palette.setColor(QPalette::Button, QColor(42, 45, 52));
        palette.setColor(QPalette::ButtonText, QColor(240, 240, 245));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(77, 148, 255));
        palette.setColor(QPalette::Highlight, QColor(50, 115, 220));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 138));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 138));
    } else {
        palette.setColor(QPalette::Window, QColor(245, 246, 248));
        palette.setColor(QPalette::WindowText, QColor(30, 32, 36));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(240, 242, 245));
        palette.setColor(QPalette::ToolTipBase, QColor(30, 32, 36));
        palette.setColor(QPalette::ToolTipText, QColor(30, 32, 36));
        palette.setColor(QPalette::Text, QColor(30, 32, 36));
        palette.setColor(QPalette::Button, QColor(235, 238, 242));
        palette.setColor(QPalette::ButtonText, QColor(30, 32, 36));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(16, 100, 215));
        palette.setColor(QPalette::Highlight, QColor(30, 110, 225));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 160));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 160));
    }

    QApplication::setPalette(palette);
    qApp->setStyleSheet(GetStyleSheet(isDark));
}

QString ThemeManager::GetStyleSheet(bool isDark) {
    if (isDark) {
        return QString(R"(
            QMainWindow {
                background-color: #1e2024;
            }
            QMenuBar {
                background-color: #181a1d;
                color: #e4e6eb;
                border-bottom: 1px solid #2d3139;
                padding: 2px 4px;
            }
            QMenuBar::item:selected {
                background-color: #2b303c;
                border-radius: 4px;
            }
            QMenu {
                background-color: #22252a;
                color: #e4e6eb;
                border: 1px solid #363b46;
                padding: 4px;
                border-radius: 6px;
            }
            QMenu::item:selected {
                background-color: #3168b8;
                border-radius: 4px;
            }
            QToolBar {
                background-color: #181a1d;
                border-bottom: 1px solid #2d3139;
                padding: 4px 8px;
                spacing: 6px;
            }
            QToolButton {
                background-color: transparent;
                color: #e4e6eb;
                border: 1px solid transparent;
                border-radius: 5px;
                padding: 5px 10px;
                font-weight: 500;
            }
            QToolButton:hover {
                background-color: #2b303c;
                border: 1px solid #3b4252;
            }
            QToolButton:pressed {
                background-color: #22252c;
            }
            QStatusBar {
                background-color: #181a1d;
                color: #9ba1ad;
                border-top: 1px solid #2d3139;
                font-size: 12px;
            }
            QTabWidget::pane {
                border: 1px solid #2d3139;
                background-color: #16181c;
                border-radius: 4px;
            }
            QTabBar::tab {
                background-color: #22252a;
                color: #9ba1ad;
                border: 1px solid #2d3139;
                border-bottom: none;
                border-top-left-radius: 5px;
                border-top-right-radius: 5px;
                padding: 6px 14px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: #16181c;
                color: #ffffff;
                border-top: 2px solid #3a7bd5;
            }
            QPushButton {
                background-color: #2d323c;
                color: #ffffff;
                border: 1px solid #404654;
                border-radius: 6px;
                padding: 7px 16px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #383f4d;
                border-color: #4f5869;
            }
            QPushButton:pressed {
                background-color: #242831;
            }
            QPushButton#primaryButton {
                background-color: #2563eb;
                border-color: #1d4ed8;
            }
            QPushButton#primaryButton:hover {
                background-color: #3b82f6;
            }
            QListWidget {
                background-color: #1e2024;
                border: 1px solid #2d3139;
                border-radius: 6px;
                padding: 4px;
            }
            QListWidget::item {
                padding: 8px 12px;
                border-radius: 4px;
                color: #e4e6eb;
            }
            QListWidget::item:hover {
                background-color: #2b303c;
            }
            QListWidget::item:selected {
                background-color: #2563eb;
                color: #ffffff;
            }
        )");
    } else {
        return QString(R"(
            QMainWindow {
                background-color: #f5f6f8;
            }
            QMenuBar {
                background-color: #ffffff;
                color: #1f2937;
                border-bottom: 1px solid #e5e7eb;
                padding: 2px 4px;
            }
            QMenuBar::item:selected {
                background-color: #f3f4f6;
                border-radius: 4px;
            }
            QMenu {
                background-color: #ffffff;
                color: #1f2937;
                border: 1px solid #e5e7eb;
                padding: 4px;
                border-radius: 6px;
            }
            QMenu::item:selected {
                background-color: #2563eb;
                color: #ffffff;
                border-radius: 4px;
            }
            QToolBar {
                background-color: #ffffff;
                border-bottom: 1px solid #e5e7eb;
                padding: 4px 8px;
                spacing: 6px;
            }
            QToolButton {
                background-color: transparent;
                color: #374151;
                border: 1px solid transparent;
                border-radius: 5px;
                padding: 5px 10px;
                font-weight: 500;
            }
            QToolButton:hover {
                background-color: #f3f4f6;
                border: 1px solid #e5e7eb;
            }
            QToolButton:pressed {
                background-color: #e5e7eb;
            }
            QStatusBar {
                background-color: #ffffff;
                color: #6b7280;
                border-top: 1px solid #e5e7eb;
                font-size: 12px;
            }
            QTabWidget::pane {
                border: 1px solid #e5e7eb;
                background-color: #ffffff;
                border-radius: 4px;
            }
            QTabBar::tab {
                background-color: #f3f4f6;
                color: #6b7280;
                border: 1px solid #e5e7eb;
                border-bottom: none;
                border-top-left-radius: 5px;
                border-top-right-radius: 5px;
                padding: 6px 14px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: #ffffff;
                color: #111827;
                border-top: 2px solid #2563eb;
            }
            QPushButton {
                background-color: #ffffff;
                color: #374151;
                border: 1px solid #d1d5db;
                border-radius: 6px;
                padding: 7px 16px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #f9fafb;
                border-color: #9ca3af;
            }
            QPushButton:pressed {
                background-color: #f3f4f6;
            }
            QPushButton#primaryButton {
                background-color: #2563eb;
                color: #ffffff;
                border-color: #1d4ed8;
            }
            QPushButton#primaryButton:hover {
                background-color: #1d4ed8;
            }
            QListWidget {
                background-color: #ffffff;
                border: 1px solid #e5e7eb;
                border-radius: 6px;
                padding: 4px;
            }
            QListWidget::item {
                padding: 8px 12px;
                border-radius: 4px;
                color: #374151;
            }
            QListWidget::item:hover {
                background-color: #f3f4f6;
            }
            QListWidget::item:selected {
                background-color: #2563eb;
                color: #ffffff;
            }
        )");
    }
}

} // namespace pdfinity::ui
