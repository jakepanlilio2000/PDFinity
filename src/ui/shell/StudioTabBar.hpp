// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QMenu>
#include <QString>
#include <vector>

namespace pdfinity::ui {

class StudioTabBar : public QTabWidget {
    Q_OBJECT

public:
    explicit StudioTabBar(QWidget* parent = nullptr);
    ~StudioTabBar() override = default;

    void SetTabDirty(int index, bool isDirty);
    void SetTabPageInfo(int index, int pageCount, const QString& fileSize, const QString& fullPath);
    void UpdateTabOverflowMenu();
    void SetCornerControlsVisible(bool visible);
    [[nodiscard]] bool AreCornerControlsVisible() const;

signals:
    void OpenFileRequested();
    void FindRequested();
    void ToggleThemeRequested();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void SetupCornerControls();

    QWidget* m_cornerContainer{nullptr};
    QToolButton* m_newTabBtn{nullptr};
    QToolButton* m_findBtn{nullptr};
    QToolButton* m_themeBtn{nullptr};
    QToolButton* m_overflowBtn{nullptr};
    QMenu* m_overflowMenu{nullptr};
};

} // namespace pdfinity::ui
