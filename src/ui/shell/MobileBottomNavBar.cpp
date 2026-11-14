// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "MobileBottomNavBar.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace pdfinity::ui {

MobileBottomNavBar::MobileBottomNavBar(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void MobileBottomNavBar::SetupUi() {
    setFixedHeight(56);
    setObjectName("mobileBottomNavBar");
    setStyleSheet(
        "#mobileBottomNavBar {"
        "  background-color: #222428;"
        "  border-top: 1px solid #34373E;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "  min-width: 48px;"
        "  min-height: 48px;"
        "  color: #A1A1AA;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "  padding: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(255, 255, 255, 0.08);"
        "  color: #F4F4F5;"
        "}"
        "QPushButton:checked {"
        "  color: #DC2626;"
        "  font-weight: bold;"
        "}"
    );

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    m_toolsBtn = CreateTabButton("⚒", tr("Tools"));
    m_toolsBtn->setCheckable(true);
    m_toolsBtn->setChecked(true);
    connect(m_toolsBtn, &QPushButton::clicked, this, [this]() {
        SetActiveTab(MobileNavTab::Tools);
        emit ToolsClicked();
    });
    layout->addWidget(m_toolsBtn);

    m_commentBtn = CreateTabButton("💬", tr("Comment"));
    m_commentBtn->setCheckable(true);
    connect(m_commentBtn, &QPushButton::clicked, this, [this]() {
        SetActiveTab(MobileNavTab::Comment);
        emit CommentClicked();
    });
    layout->addWidget(m_commentBtn);

    m_pagesBtn = CreateTabButton("📄", tr("Pages"));
    m_pagesBtn->setCheckable(true);
    connect(m_pagesBtn, &QPushButton::clicked, this, [this]() {
        SetActiveTab(MobileNavTab::Pages);
        emit PagesClicked();
    });
    layout->addWidget(m_pagesBtn);

    m_signBtn = CreateTabButton("✍", tr("Sign"));
    m_signBtn->setCheckable(true);
    connect(m_signBtn, &QPushButton::clicked, this, [this]() {
        SetActiveTab(MobileNavTab::Sign);
        emit SignClicked();
    });
    layout->addWidget(m_signBtn);

    m_moreBtn = CreateTabButton("⋯", tr("More"));
    m_moreBtn->setCheckable(true);
    connect(m_moreBtn, &QPushButton::clicked, this, [this]() {
        SetActiveTab(MobileNavTab::More);
        emit MoreClicked();
    });
    layout->addWidget(m_moreBtn);
}

QPushButton* MobileBottomNavBar::CreateTabButton(const QString& icon, const QString& label) {
    auto* btn = new QPushButton(this);
    btn->setText(QString("%1\n%2").arg(icon, label));
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

void MobileBottomNavBar::SetActiveTab(MobileNavTab tab) {
    m_activeTab = tab;
    m_toolsBtn->setChecked(tab == MobileNavTab::Tools);
    m_commentBtn->setChecked(tab == MobileNavTab::Comment);
    m_pagesBtn->setChecked(tab == MobileNavTab::Pages);
    m_signBtn->setChecked(tab == MobileNavTab::Sign);
    m_moreBtn->setChecked(tab == MobileNavTab::More);
    emit TabSelected(tab);
}

} // namespace pdfinity::ui
