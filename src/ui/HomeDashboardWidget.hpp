// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QString>
#include <QTableWidget>
#include <QLineEdit>
#include <QListWidget>
#include <unordered_set>
#include <string>

namespace pdfinity::ui {

enum class DashboardSection {
    Recent,
    Starred,
    LocalFiles,
    BatchAutomation
};

class HomeDashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit HomeDashboardWidget(QWidget* parent = nullptr);
    ~HomeDashboardWidget() override = default;

    void RefreshRecentFiles();

signals:
    void OpenFileRequested();
    void FileSelected(const QString& filePath);
    void CreateFromImagesRequested();
    void CombineFilesRequested();
    void EditPdfRequested();
    void SignRequested();
    void RedactRequested();
    void OcrRequested();
    void BatchRequested();
    void OrganizeRequested();
    void AnnotateRequested();
    void FormsRequested();
    void IntelligenceRequested();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void OnSidebarItemChanged(int row);
    void OnFilterTextChanged(const QString& text);
    void OnRecentCellDoubleClicked(int row, int column);
    void OnTogglePin(const QString& filePath);
    void OnRemoveRecent(const QString& filePath);
    void OnClearRecentClicked();

private:
    void SetupUi();
    QWidget* CreateSidebar();
    QWidget* CreateHeaderSection();
    QWidget* CreateWorkflowCardsSection();
    QWidget* CreateRecentFilesSection();
    QWidget* CreateDropzoneSection();

    DashboardSection m_currentSection{DashboardSection::Recent};
    std::unordered_set<std::string> m_pinnedPaths;

    QListWidget* m_sidebarList{nullptr};
    QLineEdit* m_searchFilterEdit{nullptr};
    QTableWidget* m_recentTable{nullptr};
};

} // namespace pdfinity::ui
