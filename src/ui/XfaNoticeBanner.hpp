// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace pdfinity::ui {

class XfaNoticeBanner : public QWidget {
    Q_OBJECT
public:
    explicit XfaNoticeBanner(QWidget* parent = nullptr);
    ~XfaNoticeBanner() override = default;

    void SetFormTypeInfo(bool isFullXfa);

private slots:
    void OnShowDetails();

private:
    QLabel* m_messageLabel{nullptr};
    QPushButton* m_detailsButton{nullptr};
    QPushButton* m_closeButton{nullptr};
    bool m_isFullXfa{true};
};

} // namespace pdfinity::ui
