// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace pdfinity::ui {

class MobileTopAppBar : public QWidget {
    Q_OBJECT

public:
    explicit MobileTopAppBar(QWidget* parent = nullptr);
    ~MobileTopAppBar() override = default;

    void SetDocumentTitle(const QString& title);
    QString GetDocumentTitle() const;

signals:
    void HomeClicked();
    void SearchClicked();
    void ThemeToggleClicked();
    void SaveClicked();
    void ShareClicked();
    void PrintClicked();
    void PreflightClicked();
    void SettingsClicked();
    void AboutClicked();

private slots:
    void ShowOverflowMenu();

private:
    void SetupUi();

    QPushButton* m_homeBtn{nullptr};
    QLabel* m_titleLabel{nullptr};
    QPushButton* m_searchBtn{nullptr};
    QPushButton* m_themeBtn{nullptr};
    QPushButton* m_overflowBtn{nullptr};
};

} // namespace pdfinity::ui
