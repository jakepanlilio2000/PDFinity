// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QPropertyAnimation>

namespace pdfinity::ui {

class MobileBottomSheet : public QWidget {
    Q_OBJECT

public:
    explicit MobileBottomSheet(QWidget* parent = nullptr);
    ~MobileBottomSheet() override = default;

    void ShowSheet(const QString& title, QWidget* content);
    void HideSheet();
    [[nodiscard]] bool IsOpen() const noexcept { return m_isOpen; }
    void Reposition();

signals:
    void SheetClosed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void SetupUi();

    QFrame* m_container{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_closeBtn{nullptr};
    QScrollArea* m_contentScrollArea{nullptr};
    QWidget* m_currentContent{nullptr};
    QPropertyAnimation* m_slideAnimation{nullptr};

    bool m_isOpen{false};
};

} // namespace pdfinity::ui
