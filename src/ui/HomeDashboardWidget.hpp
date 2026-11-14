// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QString>
#include <QTableWidget>
#include <QLineEdit>

namespace pdfinity::ui {

class HomeDashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit HomeDashboardWidget(QWidget* parent = nullptr);
    ~HomeDashboardWidget() override = default;

    void RefreshRecentFiles();

signals:
    void OpenFileRequested();
    void FileSelected(const QString& filePath);
    void OrganizeRequested();
    void AnnotateRequested();
    void FormsRequested();
    void SignRequested();
    void RedactRequested();
    void OcrRequested();
    void IntelligenceRequested();
    void BatchRequested();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void OnFilterTextChanged(const QString& text);
    void OnRecentCellDoubleClicked(int row, int column);
    void OnClearRecentClicked();

private:
    void SetupUi();
    QWidget* CreateHeaderSection();
    QWidget* CreateWorkflowCardsSection();
    QWidget* CreateRecentFilesSection();
    QWidget* CreateDropzoneSection();

    QLineEdit* m_searchFilterEdit{nullptr};
    QTableWidget* m_recentTable{nullptr};
};

} // namespace pdfinity::ui
