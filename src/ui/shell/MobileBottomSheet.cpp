// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "MobileBottomSheet.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QEvent>

namespace pdfinity::ui {

MobileBottomSheet::MobileBottomSheet(QWidget* parent) : QWidget(parent) {
    SetupUi();
    if (parent) {
        parent->installEventFilter(this);
    }
    hide();
}

void MobileBottomSheet::SetupUi() {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_container = new QFrame(this);
    m_container->setObjectName("bottomSheetContainer");
    m_container->setStyleSheet(
        "#bottomSheetContainer {"
        "  background-color: #1E2024;"
        "  border-top-left-radius: 16px;"
        "  border-top-right-radius: 16px;"
        "  border-top: 1px solid #34373E;"
        "  border-left: 1px solid #34373E;"
        "  border-right: 1px solid #34373E;"
        "}"
        "QLabel#sheetTitle {"
        "  color: #F4F4F5;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#sheetCloseBtn {"
        "  background: transparent;"
        "  border: none;"
        "  color: #A1A1AA;"
        "  font-size: 16px;"
        "  min-width: 48px;"
        "  min-height: 48px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton#sheetCloseBtn:hover, QPushButton#sheetCloseBtn:pressed {"
        "  background-color: rgba(255, 255, 255, 0.12);"
        "  color: #DC2626;"
        "}"
    );

    auto* containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(12, 8, 12, 12);
    containerLayout->setSpacing(8);

    // Top drag handle pill
    auto* handleLayout = new QHBoxLayout();
    handleLayout->addStretch();
    auto* handle = new QFrame(m_container);
    handle->setFixedSize(36, 4);
    handle->setStyleSheet("background-color: #71717A; border-radius: 2px;");
    handleLayout->addWidget(handle);
    handleLayout->addStretch();
    containerLayout->addLayout(handleLayout);

    // Header bar (Title + Close button)
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(4, 0, 0, 0);

    m_titleLabel = new QLabel(tr("Tools"), m_container);
    m_titleLabel->setObjectName("sheetTitle");
    headerLayout->addWidget(m_titleLabel, 1);

    m_closeBtn = new QPushButton("✕", m_container);
    m_closeBtn->setObjectName("sheetCloseBtn");
    m_closeBtn->setToolTip(tr("Close Sheet"));
    connect(m_closeBtn, &QPushButton::clicked, this, &MobileBottomSheet::HideSheet);
    headerLayout->addWidget(m_closeBtn);

    containerLayout->addLayout(headerLayout);

    // Content scroll area
    m_contentScrollArea = new QScrollArea(m_container);
    m_contentScrollArea->setWidgetResizable(true);
    m_contentScrollArea->setFrameShape(QFrame::NoFrame);
    m_contentScrollArea->setStyleSheet("background: transparent; border: none;");
    containerLayout->addWidget(m_contentScrollArea, 1);

    rootLayout->addWidget(m_container);

    m_slideAnimation = new QPropertyAnimation(this, "geometry", this);
    m_slideAnimation->setDuration(250);
}

void MobileBottomSheet::ShowSheet(const QString& title, QWidget* content) {
    m_titleLabel->setText(title);
    if (m_currentContent) {
        m_contentScrollArea->takeWidget();
    }
    m_currentContent = content;
    if (content) {
        m_contentScrollArea->setWidget(content);
        content->show();
    }

    QWidget* p = parentWidget();
    if (!p) return;

    int sheetHeight = std::min(400, p->height() * 3 / 4);
    int targetY = p->height() - sheetHeight;

    show();
    raise();
    m_isOpen = true;

    m_slideAnimation->stop();
    m_slideAnimation->setStartValue(QRect(0, p->height(), p->width(), sheetHeight));
    m_slideAnimation->setEndValue(QRect(0, targetY, p->width(), sheetHeight));
    m_slideAnimation->start();
}

void MobileBottomSheet::HideSheet() {
    if (!m_isOpen) return;
    m_isOpen = false;

    QWidget* p = parentWidget();
    if (p) {
        m_slideAnimation->stop();
        m_slideAnimation->setStartValue(geometry());
        m_slideAnimation->setEndValue(QRect(0, p->height(), width(), height()));
        connect(m_slideAnimation, &QPropertyAnimation::finished, this, [this]() {
            if (!m_isOpen) hide();
        });
        m_slideAnimation->start();
    } else {
        hide();
    }
    emit SheetClosed();
}

void MobileBottomSheet::Reposition() {
    QWidget* p = parentWidget();
    if (!p || !m_isOpen) return;

    int sheetHeight = std::min(400, p->height() * 3 / 4);
    int targetY = p->height() - sheetHeight;
    setGeometry(0, targetY, p->width(), sheetHeight);
    raise();
}

bool MobileBottomSheet::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        Reposition();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace pdfinity::ui
