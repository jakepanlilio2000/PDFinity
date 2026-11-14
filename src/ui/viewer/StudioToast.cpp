// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "StudioToast.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QEvent>

namespace pdfinity::ui {

void StudioToast::Show(
    QWidget* parent,
    const QString& title,
    const QString& message,
    ToastType type,
    int durationMs
) {
    auto* toast = new StudioToast(parent, title, message, type, durationMs);
    toast->show();
    toast->raise();
}

StudioToast::StudioToast(
    QWidget* parent,
    const QString& title,
    const QString& message,
    ToastType type,
    int durationMs
) : QWidget(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);

    SetupUi(title, message, type);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    m_opacityEffect->setOpacity(0.0);

    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(250);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();

    if (durationMs > 0) {
        m_dismissTimer = new QTimer(this);
        m_dismissTimer->setSingleShot(true);
        connect(m_dismissTimer, &QTimer::timeout, this, &StudioToast::Dismiss);
        m_dismissTimer->start(durationMs);
    }

    if (parent) {
        parent->installEventFilter(this);
    }

    Reposition();
}

void StudioToast::SetupUi(const QString& title, const QString& message, ToastType type) {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* card = new QFrame(this);
    card->setObjectName("toastCard");

    QString accentColor = "#10B981"; // Success emerald
    QString iconStr = "✓";
    if (type == ToastType::Info) {
        accentColor = "#3B82F6";
        iconStr = "ℹ";
    } else if (type == ToastType::Warning) {
        accentColor = "#F59E0B";
        iconStr = "⚠";
    } else if (type == ToastType::Error) {
        accentColor = "#DC2626";
        iconStr = "✕";
    }

    card->setStyleSheet(QString(
        "#toastCard {"
        "  background-color: #27292E;"
        "  border: 1px solid #34373E;"
        "  border-left: 4px solid %1;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "}"
        "QLabel {"
        "  color: #F4F4F5;"
        "  background: transparent;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: #A1A1AA;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  padding: 2px;"
        "}"
        "QPushButton:hover { color: #DC2626; }"
    ).arg(accentColor));

    auto* cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(6, 4, 6, 4);
    cardLayout->setSpacing(8);

    auto* iconLabel = new QLabel(iconStr, card);
    iconLabel->setStyleSheet(QString("font-weight: bold; font-size: 14px; color: %1;").arg(accentColor));
    cardLayout->addWidget(iconLabel);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto* titleLabel = new QLabel(QString("<b>%1</b>").arg(title), card);
    titleLabel->setStyleSheet("font-size: 12px; color: #F4F4F5;");
    textLayout->addWidget(titleLabel);

    if (!message.isEmpty()) {
        auto* msgLabel = new QLabel(message, card);
        msgLabel->setStyleSheet("font-size: 11px; color: #A1A1AA;");
        msgLabel->setWordWrap(true);
        textLayout->addWidget(msgLabel);
    }
    cardLayout->addLayout(textLayout, 1);

    auto* closeBtn = new QPushButton("✕", card);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &StudioToast::Dismiss);
    cardLayout->addWidget(closeBtn, 0, Qt::AlignTop);

    rootLayout->addWidget(card);
    adjustSize();
}

void StudioToast::Dismiss() {
    if (m_dismissTimer) m_dismissTimer->stop();
    m_fadeAnimation->stop();
    m_fadeAnimation->setDuration(200);
    m_fadeAnimation->setStartValue(m_opacityEffect->opacity());
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::close);
    m_fadeAnimation->start();
}

void StudioToast::Reposition() {
    QWidget* p = parentWidget();
    if (!p) return;

    adjustSize();
    int x = p->width() - width() - 24;
    int y = p->height() - height() - 24;
    move(std::max(10, x), std::max(10, y));
}

bool StudioToast::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        Reposition();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace pdfinity::ui