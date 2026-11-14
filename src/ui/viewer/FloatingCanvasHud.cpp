// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "FloatingCanvasHud.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QLineEdit>
#include <cmath>

namespace pdfinity::ui {

FloatingCanvasHud::FloatingCanvasHud(QWidget* parent) : QWidget(parent) {
    SetupUi();
    SetupAnimation();
    if (parent) {
        parent->installEventFilter(this);
        if (auto* sa = qobject_cast<QScrollArea*>(parent)) {
            sa->viewport()->installEventFilter(this);
        }
    }
}

void FloatingCanvasHud::SetupUi() {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* container = new QFrame(this);
    container->setObjectName("hudPillContainer");
    container->setStyleSheet(
        "#hudPillContainer {"
        "  background-color: rgba(24, 24, 27, 0.92);"
        "  border: 1px solid rgba(255, 255, 255, 0.14);"
        "  border-radius: 20px;"
        "  padding: 3px 8px;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 14px;"
        "  padding: 4px;"
        "  min-width: 26px;"
        "  min-height: 26px;"
        "  color: #F4F4F5;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.16);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.26);"
        "}"
        "QPushButton:checked {"
        "  background-color: #DC2626;"
        "  color: #FFFFFF;"
        "  font-weight: bold;"
        "}"
        "QSpinBox {"
        "  background: rgba(255, 255, 255, 0.10);"
        "  border: 1px solid rgba(255, 255, 255, 0.20);"
        "  border-radius: 4px;"
        "  color: #FFFFFF;"
        "  padding: 1px 4px;"
        "  min-width: 44px;"
        "  font-weight: 600;"
        "  font-size: 11px;"
        "}"
        "QComboBox {"
        "  background: rgba(255, 255, 255, 0.10);"
        "  border: 1px solid rgba(255, 255, 255, 0.20);"
        "  border-radius: 4px;"
        "  color: #FFFFFF;"
        "  padding: 1px 6px;"
        "  font-size: 11px;"
        "  min-width: 62px;"
        "}"
        "QComboBox::drop-down { border: none; width: 12px; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #27292E;"
        "  color: #F4F4F5;"
        "  selection-background-color: #DC2626;"
        "  selection-color: #FFFFFF;"
        "  border: 1px solid #34373E;"
        "}"
        "QLabel {"
        "  color: #A1A1AA;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
    );

    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(4);

    auto createDivider = [container]() {
        auto* div = new QFrame(container);
        div->setFrameShape(QFrame::VLine);
        div->setStyleSheet("color: rgba(255, 255, 255, 0.18); max-height: 16px; margin: 0 2px;");
        return div;
    };

    // 1. Page Navigation
    m_prevBtn = new QPushButton(container);
    m_prevBtn->setText("▲");
    m_prevBtn->setToolTip(tr("Previous Page (Page Up / Left Arrow)"));
    connect(m_prevBtn, &QPushButton::clicked, this, [this]() {
        WakeUp();
        emit PreviousPageRequested();
    });
    layout->addWidget(m_prevBtn);

    m_pageSpinBox = new QSpinBox(container);
    m_pageSpinBox->setRange(1, 1);
    m_pageSpinBox->setAlignment(Qt::AlignCenter);
    connect(m_pageSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
        WakeUp();
        emit PageSelected(val - 1);
    });
    layout->addWidget(m_pageSpinBox);

    m_pageTotalLabel = new QLabel("/ 1", container);
    layout->addWidget(m_pageTotalLabel);

    m_nextBtn = new QPushButton(container);
    m_nextBtn->setText("▼");
    m_nextBtn->setToolTip(tr("Next Page (Page Down / Right Arrow)"));
    connect(m_nextBtn, &QPushButton::clicked, this, [this]() {
        WakeUp();
        emit NextPageRequested();
    });
    layout->addWidget(m_nextBtn);

    layout->addWidget(createDivider());

    // 2. Pointer Mode: Select Text/Objects ('V') vs. Hand Pan ('H')
    m_selectModeBtn = new QPushButton("V", container);
    m_selectModeBtn->setCheckable(true);
    m_selectModeBtn->setChecked(true);
    m_selectModeBtn->setToolTip(tr("Select Text and Annotations (V)"));
    connect(m_selectModeBtn, &QPushButton::clicked, this, [this]() {
        SetPointerMode(CanvasPointerMode::Select);
    });
    layout->addWidget(m_selectModeBtn);

    m_panModeBtn = new QPushButton("H", container);
    m_panModeBtn->setCheckable(true);
    m_panModeBtn->setToolTip(tr("Hand Pan Tool (H)"));
    connect(m_panModeBtn, &QPushButton::clicked, this, [this]() {
        SetPointerMode(CanvasPointerMode::HandPan);
    });
    layout->addWidget(m_panModeBtn);

    layout->addWidget(createDivider());

    // 3. Zoom Controls
    m_zoomOutBtn = new QPushButton("−", container);
    m_zoomOutBtn->setToolTip(tr("Zoom Out (Ctrl+Minus)"));
    connect(m_zoomOutBtn, &QPushButton::clicked, this, [this]() {
        WakeUp();
        emit ZoomOutRequested();
    });
    layout->addWidget(m_zoomOutBtn);

    m_zoomCombo = new QComboBox(container);
    m_zoomCombo->setEditable(true);
    if (m_zoomCombo->lineEdit()) {
        m_zoomCombo->lineEdit()->setAlignment(Qt::AlignCenter);
        connect(m_zoomCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
            WakeUp();
            QString txt = m_zoomCombo->currentText().remove('%').trimmed();
            bool ok = false;
            double val = txt.toDouble(&ok);
            if (ok && val >= 10.0 && val <= 1000.0) {
                emit ZoomScaleChanged(val / 100.0);
            }
        });
    }
    m_zoomCombo->addItems({"50%", "75%", "100%", "125%", "150%", "200%", "300%", tr("Fit Width"), tr("Fit Page")});
    m_zoomCombo->setCurrentIndex(2); // 100%
    connect(m_zoomCombo, QOverload<int>::of(&QComboBox::activated), this, &FloatingCanvasHud::OnZoomComboActivated);
    layout->addWidget(m_zoomCombo);

    m_zoomInBtn = new QPushButton("+", container);
    m_zoomInBtn->setToolTip(tr("Zoom In (Ctrl+Plus)"));
    connect(m_zoomInBtn, &QPushButton::clicked, this, [this]() {
        WakeUp();
        emit ZoomInRequested();
    });
    layout->addWidget(m_zoomInBtn);

    layout->addWidget(createDivider());

    // 4. Page Layout & Rotate Controls
    m_singlePageBtn = new QPushButton("1", container);
    m_singlePageBtn->setCheckable(true);
    m_singlePageBtn->setToolTip(tr("Single Page Layout"));
    connect(m_singlePageBtn, &QPushButton::clicked, this, [this]() {
        SetLayoutMode(pdf_api::PageLayoutMode::SinglePage);
    });
    layout->addWidget(m_singlePageBtn);

    m_continuousBtn = new QPushButton("≡", container);
    m_continuousBtn->setCheckable(true);
    m_continuousBtn->setChecked(true);
    m_continuousBtn->setToolTip(tr("Continuous Vertical Scroll"));
    connect(m_continuousBtn, &QPushButton::clicked, this, [this]() {
        SetLayoutMode(pdf_api::PageLayoutMode::Continuous);
    });
    layout->addWidget(m_continuousBtn);

    m_facingSpreadBtn = new QPushButton("⚏", container);
    m_facingSpreadBtn->setCheckable(true);
    m_facingSpreadBtn->setToolTip(tr("Two-Page Facing Spread"));
    connect(m_facingSpreadBtn, &QPushButton::clicked, this, [this]() {
        SetLayoutMode(pdf_api::PageLayoutMode::FacingPages);
    });
    layout->addWidget(m_facingSpreadBtn);

    m_rotateBtn = new QPushButton("↷", container);
    m_rotateBtn->setToolTip(tr("Rotate Page 90° Clockwise"));
    connect(m_rotateBtn, &QPushButton::clicked, this, [this]() {
        WakeUp();
        emit RotateRequested();
    });
    layout->addWidget(m_rotateBtn);

    mainLayout->addWidget(container);
    adjustSize();
}

void FloatingCanvasHud::SetupAnimation() {
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(1.0);

    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(300);

    m_idleTimer = new QTimer(this);
    m_idleTimer->setInterval(3500); // 3.5 seconds idle
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &FloatingCanvasHud::OnIdleTimeout);
    m_idleTimer->start();
}

void FloatingCanvasHud::WakeUp() {
    m_fadeAnimation->stop();
    m_opacityEffect->setOpacity(1.0);
    m_idleTimer->start();
}

void FloatingCanvasHud::OnIdleTimeout() {
    if (m_isHovered) {
        m_idleTimer->start();
        return;
    }
    m_fadeAnimation->stop();
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(0.18); // soft ambient fade
    m_fadeAnimation->start();
}

void FloatingCanvasHud::enterEvent(QEnterEvent* /*event*/) {
    m_isHovered = true;
    WakeUp();
}

void FloatingCanvasHud::leaveEvent(QEvent* /*event*/) {
    m_isHovered = false;
    m_idleTimer->start();
}

void FloatingCanvasHud::SetPointerMode(CanvasPointerMode mode) {
    m_currentPointerMode = mode;
    m_selectModeBtn->setChecked(mode == CanvasPointerMode::Select);
    m_panModeBtn->setChecked(mode == CanvasPointerMode::HandPan);
    WakeUp();
    emit PointerModeChanged(mode);
}

void FloatingCanvasHud::SetLayoutMode(pdf_api::PageLayoutMode mode) {
    m_currentLayoutMode = mode;
    m_singlePageBtn->setChecked(mode == pdf_api::PageLayoutMode::SinglePage);
    m_continuousBtn->setChecked(mode == pdf_api::PageLayoutMode::Continuous);
    m_facingSpreadBtn->setChecked(mode == pdf_api::PageLayoutMode::FacingPages);
    WakeUp();
    emit LayoutModeChanged(mode);
}

void FloatingCanvasHud::OnZoomComboActivated(int index) {
    WakeUp();
    if (index == 7) { // Fit Width
        emit FitWidthRequested();
    } else if (index == 8) { // Fit Page
        emit FitPageRequested();
    } else {
        const double scales[] = { 0.50, 0.75, 1.00, 1.25, 1.50, 2.00, 3.00 };
        if (index >= 0 && index < 7) {
            emit ZoomScaleChanged(scales[index]);
        }
    }
}

void FloatingCanvasHud::UpdatePageInfo(int currentPage, int totalPages) {
    m_totalPages = std::max(1, totalPages);
    m_pageSpinBox->blockSignals(true);
    m_pageSpinBox->setRange(1, m_totalPages);
    m_pageSpinBox->setValue(currentPage + 1);
    m_pageSpinBox->blockSignals(false);

    m_pageTotalLabel->setText(QString("/ %1").arg(m_totalPages));
    m_prevBtn->setEnabled(currentPage > 0);
    m_nextBtn->setEnabled(currentPage + 1 < m_totalPages);
    Reposition();
}

void FloatingCanvasHud::UpdateZoom(double zoomScale) {
    int percent = static_cast<int>(std::round(zoomScale * 100.0));
    m_zoomCombo->blockSignals(true);
    QString text = QString("%1%").arg(percent);
    int idx = m_zoomCombo->findText(text);
    if (idx != -1) {
        m_zoomCombo->setCurrentIndex(idx);
    } else {
        m_zoomCombo->setEditText(text);
    }
    m_zoomCombo->blockSignals(false);
    Reposition();
}

void FloatingCanvasHud::Reposition() {
    QWidget* p = parentWidget();
    if (!p) return;

    adjustSize();
    int x = (p->width() - width()) / 2;
    int y = p->height() - height() - 20; // 20px above bottom
    move(std::max(10, x), std::max(10, y));
    raise();
}

bool FloatingCanvasHud::eventFilter(QObject* watched, QEvent* event) {
    QWidget* p = parentWidget();
    auto* sa = qobject_cast<QScrollArea*>(p);
    QObject* vp = sa ? sa->viewport() : nullptr;

    if (watched == p || watched == vp) {
        if (event->type() == QEvent::Resize) {
            Reposition();
        } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::KeyPress) {
            WakeUp();
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace pdfinity::ui