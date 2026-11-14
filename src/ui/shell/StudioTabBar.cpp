// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "StudioTabBar.hpp"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QFileInfo>

namespace pdfinity::ui {

StudioTabBar::StudioTabBar(QWidget* parent) : QTabWidget(parent) {
    setTabsClosable(true);
    setMovable(true);
    setUsesScrollButtons(true);
    setElideMode(Qt::ElideRight);

    SetupCornerControls();

    connect(this, &QTabWidget::tabCloseRequested, this, [this](int index) {
        // Prevent closing pinned Tab 0 (Home)
        if (index == 0) return;
    });
}

void StudioTabBar::SetupCornerControls() {
    m_cornerContainer = new QWidget(this);
    auto* layout = new QHBoxLayout(m_cornerContainer);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    // 1. '+' Open Tab Button
    m_newTabBtn = new QToolButton(m_cornerContainer);
    m_newTabBtn->setText("+");
    m_newTabBtn->setToolTip(tr("Open Document in New Tab (Ctrl+O)"));
    m_newTabBtn->setStyleSheet(
        "QToolButton { font-weight: bold; font-size: 15px; border: none; padding: 2px 8px; color: #A1A1AA; border-radius: 4px; }"
        "QToolButton:hover { background-color: rgba(220, 38, 38, 0.12); color: #DC2626; }"
    );
    connect(m_newTabBtn, &QToolButton::clicked, this, &StudioTabBar::OpenFileRequested);
    layout->addWidget(m_newTabBtn);

    // 2. Tab Overflow Menu Button
    m_overflowBtn = new QToolButton(m_cornerContainer);
    m_overflowBtn->setText("▾");
    m_overflowBtn->setToolTip(tr("Open Document Tabs"));
    m_overflowBtn->setStyleSheet(
        "QToolButton { font-size: 11px; border: none; padding: 2px 6px; color: #A1A1AA; border-radius: 4px; }"
        "QToolButton:hover { background-color: rgba(220, 38, 38, 0.12); color: #DC2626; }"
    );
    m_overflowMenu = new QMenu(m_overflowBtn);
    m_overflowBtn->setMenu(m_overflowMenu);
    m_overflowBtn->setPopupMode(QToolButton::InstantPopup);
    connect(m_overflowMenu, &QMenu::aboutToShow, this, &StudioTabBar::UpdateTabOverflowMenu);
    layout->addWidget(m_overflowBtn);

    // 3. Search / Find Trigger
    m_findBtn = new QToolButton(m_cornerContainer);
    m_findBtn->setText("🔍");
    m_findBtn->setToolTip(tr("Find in Document (Ctrl+F)"));
    m_findBtn->setStyleSheet(
        "QToolButton { font-size: 12px; border: none; padding: 2px 6px; color: #A1A1AA; border-radius: 4px; }"
        "QToolButton:hover { background-color: rgba(220, 38, 38, 0.12); color: #DC2626; }"
    );
    connect(m_findBtn, &QToolButton::clicked, this, &StudioTabBar::FindRequested);
    layout->addWidget(m_findBtn);

    // 4. Quick Theme Toggle
    m_themeBtn = new QToolButton(m_cornerContainer);
    m_themeBtn->setText("🌓");
    m_themeBtn->setToolTip(tr("Toggle Dark / Light Theme"));
    m_themeBtn->setStyleSheet(
        "QToolButton { font-size: 12px; border: none; padding: 2px 6px; color: #A1A1AA; border-radius: 4px; }"
        "QToolButton:hover { background-color: rgba(220, 38, 38, 0.12); color: #DC2626; }"
    );
    connect(m_themeBtn, &QToolButton::clicked, this, &StudioTabBar::ToggleThemeRequested);
    layout->addWidget(m_themeBtn);

    setCornerWidget(m_cornerContainer, Qt::TopRightCorner);
}

void StudioTabBar::SetTabDirty(int index, bool isDirty) {
    if (index <= 0 || index >= count()) return;
    QString current = tabText(index);
    if (isDirty && !current.startsWith("● ")) {
        setTabText(index, "● " + current);
    } else if (!isDirty && current.startsWith("● ")) {
        setTabText(index, current.mid(2));
    }
}

void StudioTabBar::SetTabPageInfo(int index, int pageCount, const QString& fileSize, const QString& fullPath) {
    if (index < 0 || index >= count()) return;
    QString fileName = QFileInfo(fullPath).fileName();
    if (fileName.isEmpty()) fileName = tabText(index);

    QString tooltip = QString("<b>%1</b><br>%2 pages • %3<br><span style='color: #888;'>%4</span>")
        .arg(fileName)
        .arg(pageCount)
        .arg(fileSize)
        .arg(fullPath);

    setTabToolTip(index, tooltip);
}

void StudioTabBar::UpdateTabOverflowMenu() {
    m_overflowMenu->clear();
    for (int i = 0; i < count(); ++i) {
        QString title = tabText(i);
        if (title.isEmpty()) title = tr("Document %1").arg(i);
        auto* act = m_overflowMenu->addAction(title);
        connect(act, &QAction::triggered, this, [this, i]() {
            setCurrentIndex(i);
        });
    }
}

void StudioTabBar::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        int index = tabBar()->tabAt(event->pos());
        if (index > 0) {
            emit tabCloseRequested(index);
            event->accept();
            return;
        }
    }
    QTabWidget::mouseReleaseEvent(event);
}

void StudioTabBar::SetCornerControlsVisible(bool visible) {
    if (m_cornerContainer) {
        m_cornerContainer->setVisible(visible);
    }
}

bool StudioTabBar::AreCornerControlsVisible() const {
    return m_cornerContainer && !m_cornerContainer->isHidden();
}

} // namespace pdfinity::ui
