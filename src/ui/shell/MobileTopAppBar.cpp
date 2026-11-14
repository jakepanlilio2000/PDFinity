// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "MobileTopAppBar.hpp"
#include <QHBoxLayout>
#include <QMenu>

namespace pdfinity::ui {

MobileTopAppBar::MobileTopAppBar(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void MobileTopAppBar::SetupUi() {
    setFixedHeight(52);
    setObjectName("mobileTopAppBar");
    setStyleSheet(
        "#mobileTopAppBar {"
        "  background-color: #18181B;"
        "  border-bottom: 1px solid #34373E;"
        "}"
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "  min-width: 48px;"
        "  min-height: 48px;"
        "  color: #F4F4F5;"
        "  font-size: 16px;"
        "}"
        "QPushButton:hover, QPushButton:pressed {"
        "  background-color: rgba(255, 255, 255, 0.12);"
        "}"
        "QLabel {"
        "  color: #F4F4F5;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "}"
    );

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);

    m_homeBtn = new QPushButton("←", this);
    m_homeBtn->setToolTip(tr("Home / Back"));
    connect(m_homeBtn, &QPushButton::clicked, this, &MobileTopAppBar::HomeClicked);
    layout->addWidget(m_homeBtn);

    m_titleLabel = new QLabel(tr("PDFinity"), this);
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(m_titleLabel, 1);

    m_searchBtn = new QPushButton("🔍", this);
    m_searchBtn->setToolTip(tr("Search Document (Ctrl+F)"));
    connect(m_searchBtn, &QPushButton::clicked, this, &MobileTopAppBar::SearchClicked);
    layout->addWidget(m_searchBtn);

    m_themeBtn = new QPushButton("🌓", this);
    m_themeBtn->setToolTip(tr("Toggle Theme"));
    connect(m_themeBtn, &QPushButton::clicked, this, &MobileTopAppBar::ThemeToggleClicked);
    layout->addWidget(m_themeBtn);

    m_overflowBtn = new QPushButton("⋮", this);
    m_overflowBtn->setToolTip(tr("More Actions"));
    connect(m_overflowBtn, &QPushButton::clicked, this, &MobileTopAppBar::ShowOverflowMenu);
    layout->addWidget(m_overflowBtn);
}

void MobileTopAppBar::SetDocumentTitle(const QString& title) {
    if (m_titleLabel) {
        QFontMetrics fm(m_titleLabel->font());
        QString elided = fm.elidedText(title, Qt::ElideMiddle, 220);
        m_titleLabel->setText(elided.isEmpty() ? tr("PDFinity") : elided);
        m_titleLabel->setToolTip(title);
    }
}

QString MobileTopAppBar::GetDocumentTitle() const {
    return m_titleLabel ? m_titleLabel->toolTip() : QString();
}

void MobileTopAppBar::ShowOverflowMenu() {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #27292E; color: #F4F4F5; border: 1px solid #34373E; padding: 4px; }"
        "QMenu::item { min-height: 40px; padding: 6px 16px; font-size: 13px; }"
        "QMenu::item:selected { background-color: #DC2626; color: #FFFFFF; }"
    );

    auto* saveAct = menu.addAction(tr("💾 Save Document"));
    connect(saveAct, &QAction::triggered, this, &MobileTopAppBar::SaveClicked);

    auto* shareAct = menu.addAction(tr("📤 Share / Export"));
    connect(shareAct, &QAction::triggered, this, &MobileTopAppBar::ShareClicked);

    auto* printAct = menu.addAction(tr("🖨 Print"));
    connect(printAct, &QAction::triggered, this, &MobileTopAppBar::PrintClicked);

    menu.addSeparator();

    auto* preflightAct = menu.addAction(tr("🔍 Preflight & Compliance"));
    connect(preflightAct, &QAction::triggered, this, &MobileTopAppBar::PreflightClicked);

    auto* settingsAct = menu.addAction(tr("⚙ Settings"));
    connect(settingsAct, &QAction::triggered, this, &MobileTopAppBar::SettingsClicked);

    auto* aboutAct = menu.addAction(tr("ℹ About PDFinity"));
    connect(aboutAct, &QAction::triggered, this, &MobileTopAppBar::AboutClicked);

    menu.exec(m_overflowBtn->mapToGlobal(QPoint(0, m_overflowBtn->height())));
}

} // namespace pdfinity::ui
