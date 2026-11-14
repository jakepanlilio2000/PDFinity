// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QString>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include "domain/DocumentModel.hpp"
#include "shell/StudioTabBar.hpp"

class QStackedWidget;
class QListWidget;
class QListWidgetItem;
class QLabel;

namespace pdfinity::ui {

class HomeDashboardWidget;
class PdfViewerWidget;

class DocumentWorkspace : public QWidget {
    Q_OBJECT
public:
    explicit DocumentWorkspace(QWidget* parent = nullptr);
    ~DocumentWorkspace() override = default;

    bool OpenDocumentTab(const std::filesystem::path& filePath, const std::string& password = "");
    void CloseCurrentTab();
    void CloseAllTabs();
    int GetOpenDocumentCount() const;
    QString GetCurrentDocumentPath() const;
    domain::DocumentModel* GetCurrentDocument() const;
    PdfViewerWidget* GetCurrentViewer() const;
    HomeDashboardWidget* GetHomeDashboard() const { return m_homeDashboard; }
    void ShowHomeTab();

    void RefreshRecentFilesList();
    void SaveSession();
    void RestoreSession();

signals:
    void OpenFileRequested();
    void FileSelected(const QString& filePath);
    void DocumentCountChanged(int count);
    void ToggleThemeRequested();

    // Forwarded Tool Requests (from Viewport Inspector & Tools)
    void OrganizePagesRequested();
    void MergeDocumentsRequested();
    void SplitDocumentRequested();
    void ContentEditorRequested();
    void FormInspectorRequested();
    void ObjectInspectorRequested();
    void DigitalSignRequested();
    void VisualSignRequested();
    void ValidateSignaturesRequested();
    void RedactionStudioRequested();
    void SanitizeRequested();
    void CertificateManagerRequested();
    void OcrStudioRequested();
    void ConversionCenterRequested();
    void OptimizerRequested();
    void DocumentIntelligenceRequested();
    void CompareDocumentsRequested();
    void BatchCenterRequested();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void OnTabCloseRequested(int index);
    void OnRecentItemDoubleClicked(QListWidgetItem* item);
    void OnClearRecentClicked();

private:
    void SetupUi();
    QWidget* CreateWelcomeView();
    QWidget* CreateDocumentPane(std::unique_ptr<domain::DocumentModel> doc);

    StudioTabBar* m_tabWidget{nullptr};
    HomeDashboardWidget* m_homeDashboard{nullptr};
    QStackedWidget* m_stackedWidget{nullptr};
    QWidget* m_welcomeView{nullptr};
    QListWidget* m_recentListWidget{nullptr};

    std::unordered_map<int, std::unique_ptr<domain::DocumentModel>> m_openDocuments;
};

} // namespace pdfinity::ui
