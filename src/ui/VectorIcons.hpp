// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QIcon>
#include <QString>
#include <QColor>

namespace pdfinity::ui {

enum class IconType {
    Home,
    Open,
    Save,
    Print,
    Share,
    Search,
    Thumbnails,
    Bookmarks,
    Annotations,
    Forms,
    ContentEdit,
    DigitalSign,
    VisualSign,
    Redaction,
    Sanitize,
    Ocr,
    Optimize,
    Compare,
    Batch,
    Settings,
    Properties,
    Organize,
    Merge,
    Split,
    ZoomIn,
    ZoomOut,
    FitWidth,
    FitPage,
    Rotate,
    PreviousPage,
    NextPage,
    Undo,
    Redo,
    Close,
    Shield
};

class VectorIcons {
public:
    static QIcon GetIcon(IconType type, const QColor& color = QColor(60, 60, 65));
    static QIcon GetAccentIcon(IconType type);
    static QPixmap CreatePixmap(IconType type, int size = 24, const QColor& color = QColor(60, 60, 65));
};

} // namespace pdfinity::ui
