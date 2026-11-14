// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "VectorIcons.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace pdfinity::ui {

QIcon VectorIcons::GetIcon(IconType type, const QColor& color) {
    QIcon icon;
    icon.addPixmap(CreatePixmap(type, 16, color));
    icon.addPixmap(CreatePixmap(type, 24, color));
    icon.addPixmap(CreatePixmap(type, 32, color));
    icon.addPixmap(CreatePixmap(type, 48, color));
    return icon;
}

QIcon VectorIcons::GetAccentIcon(IconType type) {
    return GetIcon(type, QColor("#DC2626")); // Crimson Brand Accent
}

QPixmap VectorIcons::CreatePixmap(IconType type, int size, const QColor& color) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPen pen(color, std::max(1.5, size * 0.08), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    double s = static_cast<double>(size);
    double m = s * 0.15; // margin
    double w = s - 2 * m;

    switch (type) {
        case IconType::Home: {
            QPainterPath path;
            path.moveTo(s * 0.5, m);
            path.lineTo(s - m, s * 0.45);
            path.lineTo(s - m * 1.3, s - m);
            path.lineTo(m * 1.3, s - m);
            path.lineTo(m, s * 0.45);
            path.closeSubpath();
            p.drawPath(path);
            p.drawRect(QRectF(s * 0.4, s * 0.6, s * 0.2, s * 0.25));
            break;
        }
        case IconType::Open: {
            QPainterPath path;
            path.moveTo(m, s * 0.35);
            path.lineTo(m + w * 0.35, s * 0.35);
            path.lineTo(m + w * 0.45, s * 0.45);
            path.lineTo(s - m, s * 0.45);
            path.lineTo(s - m, s - m);
            path.lineTo(m, s - m);
            path.closeSubpath();
            p.drawPath(path);
            break;
        }
        case IconType::Save: {
            p.drawRect(QRectF(m, m, w, w));
            p.fillRect(QRectF(m + w * 0.25, m, w * 0.5, w * 0.35), color);
            p.drawRect(QRectF(m + w * 0.2, s - m - w * 0.4, w * 0.6, w * 0.4));
            break;
        }
        case IconType::Print: {
            p.drawRect(QRectF(m + w * 0.15, m, w * 0.7, w * 0.3));
            p.drawRoundedRect(QRectF(m, m + w * 0.25, w, w * 0.45), 3, 3);
            p.drawRect(QRectF(m + w * 0.2, m + w * 0.5, w * 0.6, w * 0.4));
            break;
        }
        case IconType::Share: {
            double r = s * 0.12;
            QPointF p1(s - m - r, m + r);
            QPointF p2(m + r, s * 0.5);
            QPointF p3(s - m - r, s - m - r);
            p.drawLine(p1, p2);
            p.drawLine(p2, p3);
            p.setBrush(color);
            p.drawEllipse(p1, r, r);
            p.drawEllipse(p2, r, r);
            p.drawEllipse(p3, r, r);
            break;
        }
        case IconType::Search: {
            double cr = w * 0.35;
            QPointF center(m + cr, m + cr);
            p.drawEllipse(center, cr, cr);
            p.drawLine(center + QPointF(cr * 0.707, cr * 0.707), QPointF(s - m, s - m));
            break;
        }
        case IconType::Thumbnails: {
            double h = w * 0.42;
            p.drawRoundedRect(QRectF(m, m, h, h), 2, 2);
            p.drawRoundedRect(QRectF(s - m - h, m, h, h), 2, 2);
            p.drawRoundedRect(QRectF(m, s - m - h, h, h), 2, 2);
            p.drawRoundedRect(QRectF(s - m - h, s - m - h, h, h), 2, 2);
            break;
        }
        case IconType::Bookmarks: {
            QPainterPath path;
            path.moveTo(m + w * 0.2, m);
            path.lineTo(s - m - w * 0.2, m);
            path.lineTo(s - m - w * 0.2, s - m);
            path.lineTo(s * 0.5, s - m - w * 0.25);
            path.lineTo(m + w * 0.2, s - m);
            path.closeSubpath();
            p.drawPath(path);
            break;
        }
        case IconType::Annotations: {
            p.drawRoundedRect(QRectF(m, m, w, w * 0.75), 3, 3);
            QPainterPath tail;
            tail.moveTo(m + w * 0.25, m + w * 0.75);
            tail.lineTo(m + w * 0.15, s - m);
            tail.lineTo(m + w * 0.5, m + w * 0.75);
            p.drawPath(tail);
            p.drawLine(m + w * 0.2, m + w * 0.25, s - m - w * 0.2, m + w * 0.25);
            p.drawLine(m + w * 0.2, m + w * 0.5, s - m - w * 0.35, m + w * 0.5);
            break;
        }
        case IconType::Forms: {
            p.drawRect(QRectF(m, m, w, w));
            p.drawRect(QRectF(m + w * 0.15, m + w * 0.2, w * 0.2, w * 0.2));
            p.drawLine(m + w * 0.45, m + w * 0.3, s - m - w * 0.15, m + w * 0.3);
            p.drawRect(QRectF(m + w * 0.15, m + w * 0.6, w * 0.2, w * 0.2));
            p.drawLine(m + w * 0.45, m + w * 0.7, s - m - w * 0.15, m + w * 0.7);
            break;
        }
        case IconType::ContentEdit: {
            p.drawLine(m, s - m, m + w * 0.3, s - m);
            p.drawLine(m + w * 0.15, s - m - w * 0.15, s - m - w * 0.15, m + w * 0.15);
            p.drawLine(s - m - w * 0.3, m, s - m, m + w * 0.3);
            break;
        }
        case IconType::DigitalSign:
        case IconType::VisualSign: {
            p.drawRoundedRect(QRectF(m, m, w, w * 0.65), 3, 3);
            p.drawEllipse(QPointF(s * 0.7, s * 0.75), w * 0.2, w * 0.2);
            QPainterPath seal;
            seal.moveTo(s * 0.7, s * 0.75);
            seal.lineTo(s * 0.75, s - m);
            p.drawPath(seal);
            break;
        }
        case IconType::Redaction: {
            p.drawRect(QRectF(m, m, w, w));
            p.fillRect(QRectF(m + w * 0.15, m + w * 0.25, w * 0.7, w * 0.2), color);
            p.fillRect(QRectF(m + w * 0.15, m + w * 0.55, w * 0.5, w * 0.2), color);
            break;
        }
        case IconType::Sanitize:
        case IconType::Shield: {
            QPainterPath path;
            path.moveTo(s * 0.5, m);
            path.lineTo(s - m, m + w * 0.2);
            path.quadTo(s - m, s * 0.65, s * 0.5, s - m);
            path.quadTo(m, s * 0.65, m, m + w * 0.2);
            path.closeSubpath();
            p.drawPath(path);
            break;
        }
        case IconType::Ocr: {
            p.drawRoundedRect(QRectF(m, m, w, w), 3, 3);
            QFont f = p.font();
            f.setPixelSize(static_cast<int>(s * 0.42));
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(0, 0, s, s), Qt::AlignCenter, "OCR");
            break;
        }
        case IconType::Optimize: {
            p.drawEllipse(QRectF(m, m, w, w));
            p.drawLine(s * 0.5, s * 0.5, s * 0.75, m + w * 0.25);
            p.drawLine(s * 0.5, s * 0.5, s * 0.3, s * 0.7);
            break;
        }
        case IconType::Compare: {
            p.drawRect(QRectF(m, m + w * 0.1, w * 0.45, w * 0.8));
            p.drawRect(QRectF(m + w * 0.55, m + w * 0.1, w * 0.45, w * 0.8));
            break;
        }
        case IconType::Batch: {
            p.drawRoundedRect(QRectF(m + w * 0.2, m, w * 0.7, w * 0.7), 2, 2);
            p.drawRoundedRect(QRectF(m + w * 0.1, m + w * 0.1, w * 0.7, w * 0.7), 2, 2);
            p.drawRoundedRect(QRectF(m, m + w * 0.2, w * 0.7, w * 0.7), 2, 2);
            break;
        }
        case IconType::Settings: {
            p.drawEllipse(QRectF(m + w * 0.25, m + w * 0.25, w * 0.5, w * 0.5));
            for (int i = 0; i < 8; ++i) {
                p.drawLine(s * 0.5, m, s * 0.5, m + w * 0.18);
                p.translate(s * 0.5, s * 0.5);
                p.rotate(45);
                p.translate(-s * 0.5, -s * 0.5);
            }
            break;
        }
        case IconType::Properties: {
            p.drawRect(QRectF(m, m, w, w));
            p.drawLine(m + w * 0.2, m + w * 0.3, s - m - w * 0.2, m + w * 0.3);
            p.drawLine(m + w * 0.2, m + w * 0.5, s - m - w * 0.2, m + w * 0.5);
            p.drawLine(m + w * 0.2, m + w * 0.7, s - m - w * 0.4, m + w * 0.7);
            break;
        }
        case IconType::Organize: {
            p.drawRoundedRect(QRectF(m, m, w * 0.42, w * 0.6), 2, 2);
            p.drawRoundedRect(QRectF(s - m - w * 0.42, m + w * 0.2, w * 0.42, w * 0.6), 2, 2);
            p.drawLine(s * 0.3, s - m, s * 0.7, s - m);
            break;
        }
        case IconType::Merge: {
            p.drawRect(QRectF(m, m + w * 0.1, w * 0.4, w * 0.5));
            p.drawRect(QRectF(s - m - w * 0.4, m + w * 0.1, w * 0.4, w * 0.5));
            p.drawLine(s * 0.5, m + w * 0.7, s * 0.5, s - m);
            p.drawLine(s * 0.4, s - m - w * 0.1, s * 0.5, s - m);
            p.drawLine(s * 0.6, s - m - w * 0.1, s * 0.5, s - m);
            break;
        }
        case IconType::Split: {
            p.drawRect(QRectF(m + w * 0.2, m, w * 0.6, w * 0.8));
            p.drawLine(s * 0.5, m, s * 0.5, s - m);
            break;
        }
        case IconType::ZoomIn: {
            double cr = w * 0.35;
            QPointF center(m + cr, m + cr);
            p.drawEllipse(center, cr, cr);
            p.drawLine(center + QPointF(cr * 0.707, cr * 0.707), QPointF(s - m, s - m));
            p.drawLine(center.x() - cr * 0.5, center.y(), center.x() + cr * 0.5, center.y());
            p.drawLine(center.x(), center.y() - cr * 0.5, center.x(), center.y() + cr * 0.5);
            break;
        }
        case IconType::ZoomOut: {
            double cr = w * 0.35;
            QPointF center(m + cr, m + cr);
            p.drawEllipse(center, cr, cr);
            p.drawLine(center + QPointF(cr * 0.707, cr * 0.707), QPointF(s - m, s - m));
            p.drawLine(center.x() - cr * 0.5, center.y(), center.x() + cr * 0.5, center.y());
            break;
        }
        case IconType::FitWidth: {
            p.drawRect(QRectF(m + w * 0.2, m, w * 0.6, w));
            p.drawLine(m, s * 0.5, s - m, s * 0.5);
            p.drawLine(m + w * 0.1, s * 0.4, m, s * 0.5);
            p.drawLine(m + w * 0.1, s * 0.6, m, s * 0.5);
            p.drawLine(s - m - w * 0.1, s * 0.4, s - m, s * 0.5);
            p.drawLine(s - m - w * 0.1, s * 0.6, s - m, s * 0.5);
            break;
        }
        case IconType::FitPage: {
            p.drawRect(QRectF(m, m, w, w));
            p.drawRect(QRectF(m + w * 0.2, m + w * 0.15, w * 0.6, w * 0.7));
            break;
        }
        case IconType::Rotate: {
            p.drawArc(QRectF(m, m, w, w), 30 * 16, 270 * 16);
            p.drawLine(s - m, s * 0.5, s - m - w * 0.15, s * 0.4);
            p.drawLine(s - m, s * 0.5, s - m + w * 0.15, s * 0.4);
            break;
        }
        case IconType::PreviousPage: {
            QPainterPath path;
            path.moveTo(s - m, m);
            path.lineTo(m, s * 0.5);
            path.lineTo(s - m, s - m);
            p.drawPath(path);
            break;
        }
        case IconType::NextPage: {
            QPainterPath path;
            path.moveTo(m, m);
            path.lineTo(s - m, s * 0.5);
            path.lineTo(m, s - m);
            p.drawPath(path);
            break;
        }
        case IconType::Undo: {
            p.drawArc(QRectF(m, m, w, w * 0.8), 45 * 16, 180 * 16);
            p.drawLine(m, s * 0.5, m + w * 0.2, s * 0.35);
            p.drawLine(m, s * 0.5, m + w * 0.2, s * 0.65);
            break;
        }
        case IconType::Redo: {
            p.drawArc(QRectF(m, m, w, w * 0.8), -45 * 16, -180 * 16);
            p.drawLine(s - m, s * 0.5, s - m - w * 0.2, s * 0.35);
            p.drawLine(s - m, s * 0.5, s - m - w * 0.2, s * 0.65);
            break;
        }
        case IconType::Close: {
            p.drawLine(m + w * 0.2, m + w * 0.2, s - m - w * 0.2, s - m - w * 0.2);
            p.drawLine(s - m - w * 0.2, m + w * 0.2, m + w * 0.2, s - m - w * 0.2);
            break;
        }
    }

    return pixmap;
}

} // namespace pdfinity::ui
