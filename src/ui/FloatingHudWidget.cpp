// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "FloatingHudWidget.hpp"
#include "VectorIcons.hpp"
#include <QFrame>
#include <QEvent>
#include <QResizeEvent>
#include <cmath>
#include <algorithm>

namespace pdfinity::ui {

FloatingHudWidget::FloatingHudWidget(QWidget* parent) : QWidget(parent) {
    SetupUi();
    if (parent) {
        parent->installEventFilter(this);
    }
}

void FloatingHudWidget::SetupUi() {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* container = new QFrame(this);
    container->setObjectName("hudContainer");
    container->setStyleSheet(
        "#hudContainer {"
        "  background-color: rgba(26, 28, 35, 0.88);"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 20px;"
        "  padding: 4px 10px;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 14px;"
        "  padding: 4px;"
        "  min-width: 28px;"
        "  min-height: 28px;"
        "  color: #f3f4f6;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.15);"
        "}"
        "QPushButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.25);"
        "}"
        "QSpinBox {"
        "  background: rgba(255, 255, 255, 0.1);"
        "  border: 1px solid rgba(255, 255, 255, 0.2);"
        "  border-radius: 4px;"
        "  color: #ffffff;"
        "  padding: 2px 6px;"
        "  min-width: 48px;"
        "  font-weight: 600;"
        "}"
        "QLabel {"
        "  color: #e5e7eb;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
    );

    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(6);

    // Page navigation
    m_prevBtn = new QPushButton(container);
    m_prevBtn->setIcon(VectorIcons::GetIcon(IconType::PreviousPage, Qt::white));
    m_prevBtn->setToolTip(tr("Previous Page (Left Arrow)"));
    connect(m_prevBtn, &QPushButton::clicked, this, &FloatingHudWidget::PreviousPageRequested);
    layout->addWidget(m_prevBtn);

    m_pageSpinBox = new QSpinBox(container);
    m_pageSpinBox->setRange(1, 1);
    m_pageSpinBox->setAlignment(Qt::AlignCenter);
    connect(m_pageSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
        emit PageSelected(val - 1);
    });
    layout->addWidget(m_pageSpinBox);

    m_pageTotalLabel = new QLabel("/ 1", container);
    layout->addWidget(m_pageTotalLabel);

    m_nextBtn = new QPushButton(container);
    m_nextBtn->setIcon(VectorIcons::GetIcon(IconType::NextPage, Qt::white));
    m_nextBtn->setToolTip(tr("Next Page (Right Arrow)"));
    connect(m_nextBtn, &QPushButton::clicked, this, &FloatingHudWidget::NextPageRequested);
    layout->addWidget(m_nextBtn);

    // Divider
    auto* div1 = new QFrame(container);
    div1->setFrameShape(QFrame::VLine);
    div1->setStyleSheet("color: rgba(255, 255, 255, 0.2);");
    layout->addWidget(div1);

    // Zoom controls
    m_zoomOutBtn = new QPushButton(container);
    m_zoomOutBtn->setIcon(VectorIcons::GetIcon(IconType::ZoomOut, Qt::white));
    m_zoomOutBtn->setToolTip(tr("Zoom Out (Ctrl+Minus)"));
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &FloatingHudWidget::ZoomOutRequested);
    layout->addWidget(m_zoomOutBtn);

    m_zoomPercentLabel = new QLabel("100%", container);
    m_zoomPercentLabel->setFixedWidth(44);
    m_zoomPercentLabel->setAlignment(Qt::AlignCenter);
    m_zoomPercentLabel->setToolTip(tr("Current Zoom Level (Click to reset)"));
    m_zoomPercentLabel->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_zoomPercentLabel);

    m_zoomInBtn = new QPushButton(container);
    m_zoomInBtn->setIcon(VectorIcons::GetIcon(IconType::ZoomIn, Qt::white));
    m_zoomInBtn->setToolTip(tr("Zoom In (Ctrl+Plus)"));
    connect(m_zoomInBtn, &QPushButton::clicked, this, &FloatingHudWidget::ZoomInRequested);
    layout->addWidget(m_zoomInBtn);

    // Divider
    auto* div2 = new QFrame(container);
    div2->setFrameShape(QFrame::VLine);
    div2->setStyleSheet("color: rgba(255, 255, 255, 0.2);");
    layout->addWidget(div2);

    // Fit controls
    m_fitWidthBtn = new QPushButton(container);
    m_fitWidthBtn->setIcon(VectorIcons::GetIcon(IconType::FitWidth, Qt::white));
    m_fitWidthBtn->setToolTip(tr("Fit to Width"));
    connect(m_fitWidthBtn, &QPushButton::clicked, this, &FloatingHudWidget::FitWidthRequested);
    layout->addWidget(m_fitWidthBtn);

    m_fitPageBtn = new QPushButton(container);
    m_fitPageBtn->setIcon(VectorIcons::GetIcon(IconType::FitPage, Qt::white));
    m_fitPageBtn->setToolTip(tr("Fit Entire Page"));
    connect(m_fitPageBtn, &QPushButton::clicked, this, &FloatingHudWidget::FitPageRequested);
    layout->addWidget(m_fitPageBtn);

    m_rotateBtn = new QPushButton(container);
    m_rotateBtn->setIcon(VectorIcons::GetIcon(IconType::Rotate, Qt::white));
    m_rotateBtn->setToolTip(tr("Rotate Page 90° Clockwise"));
    connect(m_rotateBtn, &QPushButton::clicked, this, &FloatingHudWidget::RotateRequested);
    layout->addWidget(m_rotateBtn);

    mainLayout->addWidget(container);
    adjustSize();
}

void FloatingHudWidget::UpdatePageInfo(int currentPage, int totalPages) {
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

void FloatingHudWidget::UpdateZoom(double zoomScale) {
    int percent = static_cast<int>(std::round(zoomScale * 100.0));
    m_zoomPercentLabel->setText(QString("%1%").arg(percent));
    Reposition();
}

void FloatingHudWidget::Reposition() {
    QWidget* p = parentWidget();
    if (!p) return;

    adjustSize();
    int x = (p->width() - width()) / 2;
    int y = p->height() - height() - 24; // 24px above bottom edge
    move(std::max(10, x), std::max(10, y));
    raise();
}

bool FloatingHudWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        Reposition();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace pdfinity::ui
