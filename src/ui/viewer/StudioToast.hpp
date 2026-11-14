// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QString>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace pdfinity::ui {

enum class ToastType {
    Success,
    Info,
    Warning,
    Error
};

class StudioToast : public QWidget {
    Q_OBJECT

public:
    explicit StudioToast(
        QWidget* parent,
        const QString& title,
        const QString& message,
        ToastType type = ToastType::Success,
        int durationMs = 3500
    );
    ~StudioToast() override = default;

    static void Show(
        QWidget* parent,
        const QString& title,
        const QString& message,
        ToastType type = ToastType::Success,
        int durationMs = 3500
    );

public slots:
    void Dismiss();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void SetupUi(const QString& title, const QString& message, ToastType type);
    void Reposition();

    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_dismissTimer{nullptr};
};

} // namespace pdfinity::ui