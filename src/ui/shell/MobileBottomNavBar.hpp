// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QPushButton>

namespace pdfinity::ui {

enum class MobileNavTab {
    Tools,
    Comment,
    Pages,
    Sign,
    More
};

class MobileBottomNavBar : public QWidget {
    Q_OBJECT

public:
    explicit MobileBottomNavBar(QWidget* parent = nullptr);
    ~MobileBottomNavBar() override = default;

    void SetActiveTab(MobileNavTab tab);

signals:
    void ToolsClicked();
    void CommentClicked();
    void PagesClicked();
    void SignClicked();
    void MoreClicked();
    void TabSelected(MobileNavTab tab);

private:
    void SetupUi();
    QPushButton* CreateTabButton(const QString& icon, const QString& label);

    QPushButton* m_toolsBtn{nullptr};
    QPushButton* m_commentBtn{nullptr};
    QPushButton* m_pagesBtn{nullptr};
    QPushButton* m_signBtn{nullptr};
    QPushButton* m_moreBtn{nullptr};

    MobileNavTab m_activeTab{MobileNavTab::Tools};
};

} // namespace pdfinity::ui
