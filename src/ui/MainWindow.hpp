// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "application/AppConfig.hpp"
#include <QMainWindow>
#include <QString>

class QAction;
class QLabel;

namespace pdfinity::ui {

class DocumentWorkspace;
class RedactionToolbar;
class WorkflowRibbon;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    bool OpenFileByPath(const QString& filePath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void OnOpenFile();
    void OnSettings();
    void OnAbout();
    void OnShowPlatformCapabilities();
    void OnCheckForUpdates();
    void OnToggleTheme();
    void OnZoomIn();
    void OnZoomOut();
    void OnZoomReset();
    void OnPrint();
    void OnPrintPreview();
    void OnFind();
    void OnToggleFullScreen();
    void OnDocumentCountChanged(int count);
    void OnOrganizePages();
    void OnMergeDocuments();
    void OnSplitDocument();
    void OnToggleAnnotationStudio();
    void OnUndo();
    void OnRedo();
    void OnOpenFormInspector();
    void OnToggleContentEditor();
    void OnOpenObjectInspector();
    void OnDigitalSignDocument();
    void OnVisualSignDocument();
    void OnValidateSignatures();
    void OnManageCertificates();
    void OnEncryptDocument();
    void OnToggleRedactionStudio();
    void OnApplyRedactions();
    void OnSanitizeDocument();
    void OnOpenOcrStudio();
    void OnOpenConversionCenter();
    void OnOpenOptimizer();
    void OnOpenDocumentIntelligence();
    void OnOpenComparisonWorkspace();
    void OnOpenPreflightCompliance();
    void OnOpenBatchCenter();
    void OnOpenBatchProfileEditor();
    void OnOpenMcpManager();
    void OnOpenRemoteManager();
    void OnShareDocument();

private:
    void SetupUi();
    void CreateMenus();
    void CreateToolBar();
    void CreateStatusBar();
    void RestoreWindowState();
    void SaveWindowState();
    void UpdateStatusDisplay();
    void UpdateRemoteStatusIndicator();
    bool OnRemoteSensitiveActionConfirmation(const std::string& clientName, const std::string& action, const std::string& details);

    DocumentWorkspace* m_workspace{nullptr};
    WorkflowRibbon* m_workflowRibbon{nullptr};
    RedactionToolbar* m_redactionToolbar{nullptr};

    // Actions
    QAction* m_openAction{nullptr};
    QAction* m_shareAction{nullptr};
    QAction* m_printAction{nullptr};
    QAction* m_printPreviewAction{nullptr};
    QAction* m_findAction{nullptr};
    QAction* m_undoAction{nullptr};
    QAction* m_redoAction{nullptr};
    QAction* m_closeTabAction{nullptr};
    QAction* m_closeAllTabsAction{nullptr};
    QAction* m_settingsAction{nullptr};
    QAction* m_exitAction{nullptr};
    QAction* m_zoomInAction{nullptr};
    QAction* m_zoomOutAction{nullptr};
    QAction* m_zoomResetAction{nullptr};
    QAction* m_fullscreenAction{nullptr};
    QAction* m_organizePagesAction{nullptr};
    QAction* m_mergeDocumentsAction{nullptr};
    QAction* m_splitDocumentAction{nullptr};
    QAction* m_annotStudioAction{nullptr};
    QAction* m_formInspectorAction{nullptr};
    QAction* m_contentEditorAction{nullptr};
    QAction* m_objectInspectorAction{nullptr};
    QAction* m_digitalSignAction{nullptr};
    QAction* m_visualSignAction{nullptr};
    QAction* m_validateSignaturesAction{nullptr};
    QAction* m_manageCertsAction{nullptr};
    QAction* m_encryptDocumentAction{nullptr};
    QAction* m_redactionStudioAction{nullptr};
    QAction* m_applyRedactionsAction{nullptr};
    QAction* m_sanitizeDocumentAction{nullptr};
    QAction* m_ocrStudioAction{nullptr};
    QAction* m_conversionCenterAction{nullptr};
    QAction* m_optimizerAction{nullptr};
    QAction* m_docIntelligenceAction{nullptr};
    QAction* m_compareDocsAction{nullptr};
    QAction* m_preflightAction{nullptr};
    QAction* m_batchCenterAction{nullptr};
    QAction* m_batchProfileEditorAction{nullptr};
    QAction* m_mcpManagerAction{nullptr};
    QAction* m_remoteManagerAction{nullptr};

    // Status bar widgets
    QLabel* m_statusDocLabel{nullptr};
    QLabel* m_statusZoomLabel{nullptr};
    QLabel* m_statusPlatformLabel{nullptr};
    QLabel* m_statusSecurityLabel{nullptr};
    QLabel* m_statusRemoteLabel{nullptr};

    int m_currentZoomPercent{100};
};

} // namespace pdfinity::ui
