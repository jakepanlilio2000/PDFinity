// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "MainWindow.hpp"
#include "DocumentWorkspace.hpp"
#include "PdfViewerWidget.hpp"
#include "AboutDialog.hpp"
#include "SettingsDialog.hpp"
#include "ThemeManager.hpp"
#include "PageOrganizerDialog.hpp"
#include "MergeDocumentsDialog.hpp"
#include "SplitDocumentDialog.hpp"
#include "VisualSignatureDialog.hpp"
#include "DigitalSignatureDialog.hpp"
#include "CertificateManagerDialog.hpp"
#include "SignatureValidationDialog.hpp"
#include "DocumentSecurityDialog.hpp"
#include "services/SignatureService.hpp"
#include "services/DocumentEncryptionService.hpp"
#include "services/RedactionService.hpp"
#include "services/SanitizationService.hpp"
#include "RedactionToolbar.hpp"
#include "RedactionPreviewDialog.hpp"
#include "SanitizationDialog.hpp"
#include "RedactionReportDialog.hpp"
#include "OcrStudioDialog.hpp"
#include "ConversionCenterDialog.hpp"
#include "OptimizerDialog.hpp"
#include "DocumentIntelligenceDialog.hpp"
#include "ComparisonWorkspaceDialog.hpp"
#include "PreflightComplianceDialog.hpp"
#include "BatchCenterDialog.hpp"
#include "BatchProfileEditorDialog.hpp"
#include "McpConfigDialog.hpp"
#include "RemoteConfigDialog.hpp"
#include "UpdateDialog.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemoteControllerServer.hpp"
#include "remote/RemoteCommandDispatcher.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"
#include "security/PathSanitizer.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QKeySequence>
#include <QOperatingSystemVersion>

namespace pdfinity::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    SetupUi();
    RestoreWindowState();
}

void MainWindow::SetupUi() {
    setWindowTitle("PDFinity - Offline PDF Suite");

    m_workspace = new DocumentWorkspace(this);
    setCentralWidget(m_workspace);

    connect(m_workspace, &DocumentWorkspace::OpenFileRequested, this, &MainWindow::OnOpenFile);
    connect(m_workspace, &DocumentWorkspace::FileSelected, this, &MainWindow::OpenFileByPath);
    connect(m_workspace, &DocumentWorkspace::DocumentCountChanged, this, &MainWindow::OnDocumentCountChanged);

    CreateMenus();
    CreateToolBar();
    CreateStatusBar();

    // Wire Remote Controller Execution Hooks & Confirmation
    connect(&remote::RemoteControllerServer::Instance(), &remote::RemoteControllerServer::sessionConnected,
            this, &MainWindow::UpdateRemoteStatusIndicator);
    connect(&remote::RemoteControllerServer::Instance(), &remote::RemoteControllerServer::sessionDisconnected,
            this, &MainWindow::UpdateRemoteStatusIndicator);
    connect(&remote::RemoteControllerServer::Instance(), &remote::RemoteControllerServer::serverStateChanged,
            this, &MainWindow::UpdateRemoteStatusIndicator);

    remote::RemoteCommandDispatcher::ExecutionHooks remoteHooks;
    remoteHooks.getStatus = [this]() {
        QJsonObject res;
        auto* doc = m_workspace ? m_workspace->GetCurrentDocument() : nullptr;
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        if (doc && viewer) {
            res["hasDocument"] = true;
            res["title"] = QString::fromStdString(doc->GetPath().filename().string());
            res["pageCount"] = static_cast<int>(doc->GetPageCount());
            res["currentPage"] = viewer->GetCurrentPage() + 1;
            res["zoom"] = viewer->GetZoomScale();
        } else {
            res["hasDocument"] = false;
        }
        return res;
    };
    remoteHooks.gotoPage = [this](int page) {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        if (viewer) {
            viewer->GoToPage(page - 1);
            return true;
        }
        return false;
    };
    remoteHooks.nextPage = [this]() {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        auto* doc = m_workspace ? m_workspace->GetCurrentDocument() : nullptr;
        if (viewer && doc && viewer->GetCurrentPage() + 1 < static_cast<int>(doc->GetPageCount())) {
            viewer->GoToPage(viewer->GetCurrentPage() + 1);
            return true;
        }
        return false;
    };
    remoteHooks.prevPage = [this]() {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        if (viewer && viewer->GetCurrentPage() > 0) {
            viewer->GoToPage(viewer->GetCurrentPage() - 1);
            return true;
        }
        return false;
    };
    remoteHooks.firstPage = [this]() {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        if (viewer) {
            viewer->GoToPage(0);
            return true;
        }
        return false;
    };
    remoteHooks.lastPage = [this]() {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        auto* doc = m_workspace ? m_workspace->GetCurrentDocument() : nullptr;
        if (viewer && doc && doc->GetPageCount() > 0) {
            viewer->GoToPage(static_cast<int>(doc->GetPageCount()) - 1);
            return true;
        }
        return false;
    };
    remoteHooks.setZoom = [this](double level, const std::string& mode) {
        auto* viewer = m_workspace ? m_workspace->GetCurrentViewer() : nullptr;
        if (viewer) {
            if (mode == "fit_width") viewer->ZoomFitWidth();
            else if (mode == "fit_page") viewer->ZoomFitPage();
            else viewer->SetZoom(level);
            return true;
        }
        return false;
    };
    remoteHooks.toggleFullscreen = [this]() {
        OnToggleFullScreen();
        return true;
    };
    remoteHooks.closeDocument = [this]() {
        if (m_workspace) {
            m_workspace->CloseCurrentTab();
            return true;
        }
        return false;
    };
    remote::RemoteCommandDispatcher::Instance().SetExecutionHooks(remoteHooks);

    remote::RemoteSecurityPolicy::Instance().SetConfirmationCallback(
        [this](const std::string& clientName, const std::string& action, const std::string& details) {
            return OnRemoteSensitiveActionConfirmation(clientName, action, details);
        }
    );

    UpdateRemoteStatusIndicator();

    UpdateStatusDisplay();
    m_workspace->RestoreSession();
}

void MainWindow::CreateMenus() {
    auto* menuBarWidget = menuBar();

    // File Menu
    auto* fileMenu = menuBarWidget->addMenu(tr("&File"));

    m_openAction = fileMenu->addAction(tr("&Open Document..."));
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::OnOpenFile);

    m_printAction = fileMenu->addAction(tr("&Print..."));
    m_printAction->setShortcut(QKeySequence::Print);
    connect(m_printAction, &QAction::triggered, this, &MainWindow::OnPrint);

    m_printPreviewAction = fileMenu->addAction(tr("Print Pre&view..."));
    connect(m_printPreviewAction, &QAction::triggered, this, &MainWindow::OnPrintPreview);

    m_shareAction = fileMenu->addAction(tr("&Share Document..."));
    m_shareAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(m_shareAction, &QAction::triggered, this, &MainWindow::OnShareDocument);

    fileMenu->addSeparator();

    m_closeTabAction = fileMenu->addAction(tr("&Close Tab"));
    m_closeTabAction->setShortcut(QKeySequence::Close);
    connect(m_closeTabAction, &QAction::triggered, m_workspace, &DocumentWorkspace::CloseCurrentTab);

    m_closeAllTabsAction = fileMenu->addAction(tr("Close &All Tabs"));
    connect(m_closeAllTabsAction, &QAction::triggered, m_workspace, &DocumentWorkspace::CloseAllTabs);

    fileMenu->addSeparator();

    m_settingsAction = fileMenu->addAction(tr("&Preferences..."));
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::OnSettings);

    fileMenu->addSeparator();

    m_exitAction = fileMenu->addAction(tr("E&xit"));
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    // Edit Menu
    auto* editMenu = menuBarWidget->addMenu(tr("&Edit"));

    m_undoAction = editMenu->addAction(tr("&Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::OnUndo);

    m_redoAction = editMenu->addAction(tr("&Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::OnRedo);

    editMenu->addSeparator();

    m_findAction = editMenu->addAction(tr("&Find in Document..."));
    m_findAction->setShortcut(QKeySequence::Find);
    connect(m_findAction, &QAction::triggered, this, &MainWindow::OnFind);

    // View Menu
    auto* viewMenu = menuBarWidget->addMenu(tr("&View"));

    m_zoomInAction = viewMenu->addAction(tr("Zoom &In"));
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::OnZoomIn);

    m_zoomOutAction = viewMenu->addAction(tr("Zoom &Out"));
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::OnZoomOut);

    m_zoomResetAction = viewMenu->addAction(tr("&Actual Size (100%)"));
    m_zoomResetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(m_zoomResetAction, &QAction::triggered, this, &MainWindow::OnZoomReset);

    viewMenu->addSeparator();

    m_fullscreenAction = viewMenu->addAction(tr("Toggle &Full Screen"));
    m_fullscreenAction->setShortcut(QKeySequence::FullScreen);
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::OnToggleFullScreen);

    viewMenu->addSeparator();
    auto* themeMenu = viewMenu->addMenu(tr("&Theme"));
    auto* actSys = themeMenu->addAction(tr("System Default"));
    connect(actSys, &QAction::triggered, this, [this]() {
        app::AppContext::Instance().GetConfig().themeMode = app::AppThemeMode::System;
        ThemeManager::ApplyTheme(app::AppThemeMode::System);
        app::AppContext::Instance().SaveCurrentState();
    });

    auto* actLight = themeMenu->addAction(tr("Light Theme"));
    connect(actLight, &QAction::triggered, this, [this]() {
        app::AppContext::Instance().GetConfig().themeMode = app::AppThemeMode::Light;
        ThemeManager::ApplyTheme(app::AppThemeMode::Light);
        app::AppContext::Instance().SaveCurrentState();
    });

    auto* actDark = themeMenu->addAction(tr("Dark Theme"));
    connect(actDark, &QAction::triggered, this, [this]() {
        app::AppContext::Instance().GetConfig().themeMode = app::AppThemeMode::Dark;
        ThemeManager::ApplyTheme(app::AppThemeMode::Dark);
        app::AppContext::Instance().SaveCurrentState();
    });

    // Tools Menu - Active Page Tools
    auto* toolsMenu = menuBarWidget->addMenu(tr("&Tools"));

    m_organizePagesAction = toolsMenu->addAction(tr("&Organize Pages..."));
    m_organizePagesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(m_organizePagesAction, &QAction::triggered, this, &MainWindow::OnOrganizePages);

    m_mergeDocumentsAction = toolsMenu->addAction(tr("&Merge Documents..."));
    m_mergeDocumentsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(m_mergeDocumentsAction, &QAction::triggered, this, &MainWindow::OnMergeDocuments);

    m_splitDocumentAction = toolsMenu->addAction(tr("&Split Document..."));
    m_splitDocumentAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(m_splitDocumentAction, &QAction::triggered, this, &MainWindow::OnSplitDocument);

    toolsMenu->addSeparator();

    m_annotStudioAction = toolsMenu->addAction(tr("&Annotation & Markup Studio..."));
    m_annotStudioAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    connect(m_annotStudioAction, &QAction::triggered, this, &MainWindow::OnToggleAnnotationStudio);

    m_formInspectorAction = toolsMenu->addAction(tr("Form &Inspector & Fill..."));
    m_formInspectorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    connect(m_formInspectorAction, &QAction::triggered, this, &MainWindow::OnOpenFormInspector);

    m_contentEditorAction = toolsMenu->addAction(tr("Page &Content & Object Editor..."));
    m_contentEditorAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    connect(m_contentEditorAction, &QAction::triggered, this, &MainWindow::OnToggleContentEditor);

    m_objectInspectorAction = toolsMenu->addAction(tr("&Object Inspector & Streams..."));
    connect(m_objectInspectorAction, &QAction::triggered, this, &MainWindow::OnOpenObjectInspector);

    toolsMenu->addSeparator();

    m_redactionStudioAction = toolsMenu->addAction(tr("&Redaction & Sanitization Studio..."));
    m_redactionStudioAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    connect(m_redactionStudioAction, &QAction::triggered, this, &MainWindow::OnToggleRedactionStudio);

    m_applyRedactionsAction = toolsMenu->addAction(tr("Apply &Redactions..."));
    connect(m_applyRedactionsAction, &QAction::triggered, this, &MainWindow::OnApplyRedactions);

    m_sanitizeDocumentAction = toolsMenu->addAction(tr("&Sanitize Document (Metadata & Attachments)..."));
    connect(m_sanitizeDocumentAction, &QAction::triggered, this, &MainWindow::OnSanitizeDocument);

    toolsMenu->addSeparator();

    m_ocrStudioAction = toolsMenu->addAction(tr("&OCR Studio (Searchable PDF)..."));
    m_ocrStudioAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    connect(m_ocrStudioAction, &QAction::triggered, this, &MainWindow::OnOpenOcrStudio);

    m_conversionCenterAction = toolsMenu->addAction(tr("Document &Conversion Center..."));
    m_conversionCenterAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(m_conversionCenterAction, &QAction::triggered, this, &MainWindow::OnOpenConversionCenter);

    m_optimizerAction = toolsMenu->addAction(tr("&Optimize & Compress PDF..."));
    m_optimizerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    connect(m_optimizerAction, &QAction::triggered, this, &MainWindow::OnOpenOptimizer);

    toolsMenu->addSeparator();

    // Inspection & Standards Validation
    m_docIntelligenceAction = toolsMenu->addAction(tr("Document &Intelligence & Inspector..."));
    m_docIntelligenceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    connect(m_docIntelligenceAction, &QAction::triggered, this, &MainWindow::OnOpenDocumentIntelligence);

    m_compareDocsAction = toolsMenu->addAction(tr("&Compare Documents (Visual & Text)..."));
    m_compareDocsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));
    connect(m_compareDocsAction, &QAction::triggered, this, &MainWindow::OnOpenComparisonWorkspace);

    m_preflightAction = toolsMenu->addAction(tr("&Preflight & PDF/A Compliance Studio..."));
    m_preflightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X));
    connect(m_preflightAction, &QAction::triggered, this, &MainWindow::OnOpenPreflightCompliance);

    toolsMenu->addSeparator();

    // Batch Processing & Automation
    m_batchCenterAction = toolsMenu->addAction(tr("&Batch Processing Center..."));
    m_batchCenterAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    connect(m_batchCenterAction, &QAction::triggered, this, &MainWindow::OnOpenBatchCenter);

    m_batchProfileEditorAction = toolsMenu->addAction(tr("Batch &Profiles Editor..."));
    connect(m_batchProfileEditorAction, &QAction::triggered, this, &MainWindow::OnOpenBatchProfileEditor);

    toolsMenu->addSeparator();

    m_mcpManagerAction = toolsMenu->addAction(tr("&Model Context Protocol (MCP) Manager..."));
    m_mcpManagerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(m_mcpManagerAction, &QAction::triggered, this, &MainWindow::OnOpenMcpManager);

    m_remoteManagerAction = toolsMenu->addAction(tr("&Remote Controller Manager..."));
    m_remoteManagerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    connect(m_remoteManagerAction, &QAction::triggered, this, &MainWindow::OnOpenRemoteManager);

    // Security Menu
    auto* secMenu = menuBarWidget->addMenu(tr("&Security"));

    m_digitalSignAction = secMenu->addAction(tr("Digitally &Sign Document (PKCS#7)..."));
    m_digitalSignAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(m_digitalSignAction, &QAction::triggered, this, &MainWindow::OnDigitalSignDocument);

    m_visualSignAction = secMenu->addAction(tr("Place &Visual Signature / Stamp..."));
    connect(m_visualSignAction, &QAction::triggered, this, &MainWindow::OnVisualSignDocument);

    secMenu->addSeparator();

    m_validateSignaturesAction = secMenu->addAction(tr("&Validate Digital Signatures..."));
    m_validateSignaturesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    connect(m_validateSignaturesAction, &QAction::triggered, this, &MainWindow::OnValidateSignatures);

    m_manageCertsAction = secMenu->addAction(tr("&Manage Certificates & Vault..."));
    connect(m_manageCertsAction, &QAction::triggered, this, &MainWindow::OnManageCertificates);

    secMenu->addSeparator();

    m_encryptDocumentAction = secMenu->addAction(tr("&Encrypt & Set Permissions..."));
    connect(m_encryptDocumentAction, &QAction::triggered, this, &MainWindow::OnEncryptDocument);

    // Help Menu
    auto* helpMenu = menuBarWidget->addMenu(tr("&Help"));
    auto* aboutAct = helpMenu->addAction(tr("&About PDFinity..."));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::OnAbout);

    auto* capsAct = helpMenu->addAction(tr("Platform &Capabilities..."));
    connect(capsAct, &QAction::triggered, this, &MainWindow::OnShowPlatformCapabilities);

    auto* updateAct = helpMenu->addAction(tr("Check for &Updates..."));
    connect(updateAct, &QAction::triggered, this, &MainWindow::OnCheckForUpdates);

    helpMenu->addSeparator();
    auto* licAct = helpMenu->addAction(tr("Open Source &Licenses..."));
    connect(licAct, &QAction::triggered, this, &MainWindow::OnAbout);
}

void MainWindow::CreateToolBar() {
    auto* toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);

#if defined(__ANDROID__)
    const bool isMobile = true;
#else
    const bool isMobile = (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android);
#endif

    if (isMobile) {
        toolBar->setIconSize(QSize(36, 36));
        toolBar->setStyleSheet("QToolButton { min-width: 48px; min-height: 48px; padding: 6px; }");
    }

    toolBar->addAction(m_openAction);
    toolBar->addSeparator();
    toolBar->addAction(m_zoomInAction);
    toolBar->addAction(m_zoomResetAction);
    toolBar->addAction(m_zoomOutAction);
    toolBar->addSeparator();

    if (m_shareAction) {
        toolBar->addAction(m_shareAction);
        toolBar->addSeparator();
    }

    auto* themeAction = toolBar->addAction(tr("🌓 Toggle Theme"));
    themeAction->setToolTip(tr("Toggle Dark / Light Mode"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::OnToggleTheme);

    toolBar->addSeparator();
    toolBar->addAction(m_settingsAction);

    m_redactionToolbar = new RedactionToolbar(this);
    addToolBar(m_redactionToolbar);
    m_redactionToolbar->hide();

    connect(m_redactionToolbar, &RedactionToolbar::applyRedactionsRequested, this, &MainWindow::OnApplyRedactions);
    connect(m_redactionToolbar, &RedactionToolbar::sanitizeDocumentRequested, this, &MainWindow::OnSanitizeDocument);
    connect(m_redactionToolbar, &RedactionToolbar::clearMarksRequested, this, [this]() {
        services::RedactionService::Instance().ClearRedactionMarks();
        m_redactionToolbar->UpdateMarkCount(0);
        QMessageBox::information(this, "Marks Cleared", "All unapplied redaction marks have been cleared.");
    });
}

void MainWindow::CreateStatusBar() {
    auto* sb = statusBar();

    m_statusDocLabel = new QLabel(tr("Ready"), this);
    sb->addWidget(m_statusDocLabel, 1);

    m_statusZoomLabel = new QLabel("100%", this);
    m_statusZoomLabel->setStyleSheet("padding: 0 8px; color: #6b7280;");
    sb->addPermanentWidget(m_statusZoomLabel);

    const auto& platform = app::AppContext::Instance().GetPlatform();
    QString platformStr = QString("%1 (%2)").arg(
        QString::fromStdString(platform.GetPlatformName()),
        QString::fromStdString(platform.GetArchitecture())
    );
    m_statusPlatformLabel = new QLabel(platformStr, this);
    m_statusPlatformLabel->setStyleSheet("padding: 0 8px; color: #6b7280; font-weight: 500;");
    sb->addPermanentWidget(m_statusPlatformLabel);

    m_statusSecurityLabel = new QLabel(tr("🔒 100% Offline"), this);
    m_statusSecurityLabel->setStyleSheet(
        "background-color: rgba(16, 185, 129, 0.15); color: #059669; "
        "padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 11px;"
    );
    sb->addPermanentWidget(m_statusSecurityLabel);

    m_statusRemoteLabel = new QLabel(tr("📡 Remote: Inactive"), this);
    m_statusRemoteLabel->setStyleSheet(
        "background-color: #F1F5F9; color: #64748B; "
        "padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 11px;"
    );
    sb->addPermanentWidget(m_statusRemoteLabel);
}

void MainWindow::RestoreWindowState() {
    const auto& config = app::AppContext::Instance().GetConfig();

    if (config.windowWidth > 200 && config.windowHeight > 200) {
        resize(config.windowWidth, config.windowHeight);
    } else {
        resize(1280, 820);
    }

    if (config.windowPosX >= 0 && config.windowPosY >= 0) {
        move(config.windowPosX, config.windowPosY);
    }

    if (config.isMaximized) {
        showMaximized();
    }

    ThemeManager::ApplyTheme(config.themeMode);
}

void MainWindow::SaveWindowState() {
    auto& config = app::AppContext::Instance().GetConfig();

    config.isMaximized = isMaximized();
    if (!isMaximized()) {
        config.windowWidth = width();
        config.windowHeight = height();
        config.windowPosX = x();
        config.windowPosY = y();
    }

    app::AppContext::Instance().SaveCurrentState();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_workspace->SaveSession();
    SaveWindowState();
    event->accept();
}

void MainWindow::OnOpenFile() {
    const auto& platform = app::AppContext::Instance().GetPlatform();
    QString startDir = QString::fromStdString(platform.GetDocumentsDirectory().string());

    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open PDF Document"),
        startDir,
        tr("PDF Documents (*.pdf);;All Files (*.*)")
    );

    if (!path.isEmpty()) {
        OpenFileByPath(path);
    }
}

bool MainWindow::OpenFileByPath(const QString& filePath) {
    std::filesystem::path stdPath(filePath.toStdString());
    auto sanitized = security::PathSanitizer::Canonicalize(stdPath);
    if (!sanitized) {
        QMessageBox::critical(
            this,
            tr("Security Error"),
            tr("Invalid or unsafe file path: %1").arg(QString::fromStdString(security::PathSanitizer::GetErrorMessage(sanitized.error())))
        );
        return false;
    }

    bool success = m_workspace->OpenDocumentTab(sanitized.value());
    if (success) {
        m_statusDocLabel->setText(tr("Viewing: %1").arg(QFileInfo(filePath).fileName()));
    }
    return success;
}

void MainWindow::OnSettings() {
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnCheckForUpdates() {
    UpdateDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnShowPlatformCapabilities() {
    const auto& platform = app::AppContext::Instance().GetPlatform();

    QString details = tr(
        "<h3>Host Platform Diagnostic</h3>"
        "<ul>"
        "<li><b>Operating System:</b> %1</li>"
        "<li><b>Version String:</b> %2</li>"
        "<li><b>Architecture:</b> %3</li>"
        "<li><b>Portable Zero-Footprint Mode:</b> %4</li>"
        "<li><b>Application Data Path:</b> %5</li>"
        "<li><b>Per-Monitor DPI v2:</b> %6</li>"
        "<li><b>Hardware Acceleration:</b> %7</li>"
        "<li><b>DirectWrite Font Integration:</b> %8</li>"
        "<li><b>Native System Print Support:</b> %9</li>"
        "<li><b>Modern Cryptography (CNG):</b> %10</li>"
        "<li><b>Local IPC Pipes:</b> %11</li>"
        "<li><b>TLS 1.3 Stack:</b> %12</li>"
        "</ul>"
    ).arg(QString::fromStdString(platform.GetPlatformName()))
     .arg(QString::fromStdString(platform.GetPlatformVersion()))
     .arg(QString::fromStdString(platform.GetArchitecture()))
     .arg(platform.IsPortableMode() ? "Active (Zero Registry Footprint)" : "Standard System Installation")
     .arg(QString::fromStdString(platform.GetAppDataDirectory().string()))
     .arg(platform.HasCapability(platform::PlatformCapability::PerMonitorDpiV2) ? "Supported (Active)" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::HardwareAcceleratedRendering) ? "Supported" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::DirectWriteFonts) ? "Supported" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::SystemPrintSupport) ? "Supported" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::ModernCryptoCng) ? "Supported" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::LocalMcpPipes) ? "Supported" : "Unsupported")
     .arg(platform.HasCapability(platform::PlatformCapability::Tls13Support) ? "Supported" : "Unsupported");

    QMessageBox::information(this, tr("Detected Platform Capabilities"), details);
}

void MainWindow::OnToggleTheme() {
    auto& config = app::AppContext::Instance().GetConfig();
    bool currentlyDark = ThemeManager::IsEffectiveDark(config.themeMode);

    config.themeMode = currentlyDark ? app::AppThemeMode::Light : app::AppThemeMode::Dark;
    ThemeManager::ApplyTheme(config.themeMode);
    app::AppContext::Instance().SaveCurrentState();
}

void MainWindow::OnZoomIn() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->ZoomIn();
        m_currentZoomPercent = static_cast<int>(std::round(viewer->GetZoomScale() * 100.0));
    } else if (m_currentZoomPercent < 800) {
        m_currentZoomPercent += 25;
    }
    UpdateStatusDisplay();
}

void MainWindow::OnZoomOut() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->ZoomOut();
        m_currentZoomPercent = static_cast<int>(std::round(viewer->GetZoomScale() * 100.0));
    } else if (m_currentZoomPercent > 25) {
        m_currentZoomPercent -= 25;
    }
    UpdateStatusDisplay();
}

void MainWindow::OnZoomReset() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->SetZoom(1.0);
    }
    m_currentZoomPercent = 100;
    UpdateStatusDisplay();
}

void MainWindow::OnPrint() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->Print();
    }
}

void MainWindow::OnPrintPreview() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->PrintPreview();
    }
}

void MainWindow::OnShareDocument() {
    if (!m_workspace) return;
    auto* doc = m_workspace->GetCurrentDocument();
    if (!doc || !doc->IsValid()) {
        QMessageBox::information(this, tr("Share Document"), tr("No document is currently open."));
        return;
    }
    const auto& platform = app::AppContext::Instance().GetPlatform();
    if (!platform.ShareDocument(doc->GetPath())) {
        QMessageBox::warning(this, tr("Share Document"), tr("Unable to open system share sheet for the current document."));
    }
}

void MainWindow::OnFind() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->ToggleFind();
    }
}

void MainWindow::OnToggleFullScreen() {
    if (isFullScreen()) {
        showNormal();
        menuBar()->show();
        statusBar()->show();
    } else {
        showFullScreen();
    }
}

void MainWindow::OnDocumentCountChanged(int count) {
    m_closeTabAction->setEnabled(count > 0);
    m_closeAllTabsAction->setEnabled(count > 0);

    if (count == 0) {
        m_statusDocLabel->setText(tr("Ready"));
    } else {
        m_statusDocLabel->setText(tr("%1 document(s) open").arg(count));
    }
}

void MainWindow::UpdateStatusDisplay() {
    m_statusZoomLabel->setText(QString("%1%").arg(m_currentZoomPercent));
}

void MainWindow::OnOrganizePages() {
    auto engine = app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        QMessageBox::critical(this, tr("Error"), tr("PDF Engine is not available."));
        return;
    }

    QString currentPath = m_workspace->GetCurrentDocumentPath();
    if (currentPath.isEmpty()) {
        currentPath = QFileDialog::getOpenFileName(
            this,
            tr("Select PDF to Organize"),
            QString(),
            tr("PDF Documents (*.pdf)")
        );
        if (currentPath.isEmpty()) return;
    }

    PageOrganizerDialog dlg(engine, currentPath.toStdString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        const auto& outPath = dlg.GetResultOutputPath();
        if (!outPath.empty() && std::filesystem::exists(outPath)) {
            auto ans = QMessageBox::question(
                this,
                tr("Open Reorganized Document"),
                tr("Reorganized PDF created successfully.\nWould you like to open it now?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            if (ans == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(outPath.string()));
            }
        }
    }
}

void MainWindow::OnMergeDocuments() {
    auto engine = app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        QMessageBox::critical(this, tr("Error"), tr("PDF Engine is not available."));
        return;
    }

    MergeDocumentsDialog dlg(engine, this);
    if (dlg.exec() == QDialog::Accepted) {
        const auto& outPath = dlg.GetResultOutputPath();
        if (!outPath.empty() && std::filesystem::exists(outPath)) {
            auto ans = QMessageBox::question(
                this,
                tr("Open Merged Document"),
                tr("Merged PDF created successfully.\nWould you like to open it now?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            if (ans == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(outPath.string()));
            }
        }
    }
}

void MainWindow::OnSplitDocument() {
    auto engine = app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        QMessageBox::critical(this, tr("Error"), tr("PDF Engine is not available."));
        return;
    }

    QString currentPath = m_workspace->GetCurrentDocumentPath();
    SplitDocumentDialog dlg(engine, currentPath.isEmpty() ? std::filesystem::path{} : currentPath.toStdString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        const auto& generated = dlg.GetGeneratedFiles();
        if (!generated.empty() && std::filesystem::exists(generated.front())) {
            auto ans = QMessageBox::question(
                this,
                tr("Open Split Document"),
                tr("Document successfully split into %1 files.\nWould you like to open the first chunk now?").arg(generated.size()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
            );
            if (ans == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(generated.front().string()));
            }
        }
    }
}

void MainWindow::OnToggleAnnotationStudio() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->ToggleAnnotationStudio();
    }
}

void MainWindow::OnUndo() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->Undo();
    }
}

void MainWindow::OnRedo() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->Redo();
    }
}

void MainWindow::OnOpenFormInspector() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->OpenFormInspector();
    } else {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to inspect or fill form fields."));
    }
}

void MainWindow::OnToggleContentEditor() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->ToggleContentEditor();
    } else {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to edit page content."));
    }
}

void MainWindow::OnOpenObjectInspector() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (viewer) {
        viewer->OpenObjectInspector();
    } else {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to inspect page objects."));
    }
}

void MainWindow::OnDigitalSignDocument() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to sign."));
        return;
    }

    DigitalSignatureDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto doc = viewer->GetDocument();
        std::filesystem::path srcPath = doc->GetPath();

        QString dstPathStr = QFileDialog::getSaveFileName(
            this, tr("Save Cryptographically Signed PDF"),
            QString::fromStdString((srcPath.parent_path() / (srcPath.stem().string() + "_signed.pdf")).string()),
            tr("PDF Files (*.pdf)")
        );
        if (dstPathStr.isEmpty()) return;

        std::filesystem::path dstPath(dstPathStr.toStdString());
        auto signRes = services::SignatureService::SignDocumentIncremental(
            srcPath, dstPath, dlg.GetSignOptions(), dlg.GetPassword()
        );

        if (signRes.has_value()) {
            auto ans = QMessageBox::question(
                this, tr("Signature Complete"),
                tr("Cryptographic signature successfully embedded via ISO 32000 incremental update.\n\nOpen signed copy?"),
                QMessageBox::Yes | QMessageBox::No
            );
            if (ans == QMessageBox::Yes) {
                OpenFileByPath(dstPathStr);
            }
        } else {
            QMessageBox::critical(this, tr("Signing Failed"), QString::fromStdString(signRes.errorMessage));
        }
    }
}

void MainWindow::OnVisualSignDocument() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to place a visual signature."));
        return;
    }

    VisualSignatureDialog dlg(viewer->GetCurrentPage(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto doc = viewer->GetDocument();
        auto res = services::SignatureService::ApplyVisualSignature(*doc, dlg.GetSignatureData());
        if (res.has_value()) {
            viewer->update();
            QMessageBox::information(
                this, tr("Visual Signature Placed"),
                tr("Visual signature appearance placed on page %1. Remember to save your document to retain changes.")
                    .arg(viewer->GetCurrentPage() + 1)
            );
        } else {
            QMessageBox::critical(this, tr("Error"), QString::fromStdString(res.errorMessage));
        }
    }
}

void MainWindow::OnValidateSignatures() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to validate signatures."));
        return;
    }

    auto doc = viewer->GetDocument();
    auto valRes = services::SignatureService::ValidateDocumentSignatures(doc->GetPath());
    if (valRes.has_value()) {
        SignatureValidationDialog dlg(doc->GetPath(), valRes.value, this);
        dlg.exec();
    } else {
        QMessageBox::critical(this, tr("Validation Failed"), QString::fromStdString(valRes.errorMessage));
    }
}

void MainWindow::OnManageCertificates() {
    CertificateManagerDialog dlg(this);
    dlg.exec();
}

void MainWindow::OnEncryptDocument() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to encrypt."));
        return;
    }

    DocumentSecurityDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto doc = viewer->GetDocument();
        std::filesystem::path srcPath = doc->GetPath();

        QString dstPathStr = QFileDialog::getSaveFileName(
            this, tr("Save Encrypted PDF"),
            QString::fromStdString((srcPath.parent_path() / (srcPath.stem().string() + "_protected.pdf")).string()),
            tr("PDF Files (*.pdf)")
        );
        if (dstPathStr.isEmpty()) return;

        std::filesystem::path dstPath(dstPathStr.toStdString());
        auto encRes = services::DocumentEncryptionService::EncryptDocument(srcPath, dstPath, dlg.GetConfig());
        if (encRes.has_value()) {
            QMessageBox::information(
                this, tr("Document Encrypted"),
                tr("Password protection and security permissions successfully applied to:\n%1").arg(dstPathStr)
            );
        } else {
            QMessageBox::critical(this, tr("Encryption Failed"), QString::fromStdString(encRes.errorMessage));
        }
    }
}

void MainWindow::OnToggleRedactionStudio() {
    if (!m_redactionToolbar) return;
    bool isVis = m_redactionToolbar->isVisible();
    m_redactionToolbar->setVisible(!isVis);
    int markCount = static_cast<int>(services::RedactionService::Instance().GetRedactionMarks().size());
    m_redactionToolbar->UpdateMarkCount(markCount);
}

void MainWindow::OnApplyRedactions() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to apply redactions."));
        return;
    }

    auto doc = viewer->GetDocument();
    auto marks = services::RedactionService::Instance().GetRedactionMarks();
    if (marks.empty()) {
        QMessageBox::information(
            this, tr("No Redaction Marks"),
            tr("No regions have been marked for redaction. Use 'Mark Redaction' or add redaction coordinates first.")
        );
        return;
    }

    auto analysisRes = services::RedactionService::Instance().AnalyzeAllMarks(*doc);
    std::vector<pdf_api::RedactionTargetAnalysis> analyses;
    if (analysisRes.has_value()) {
        analyses = analysisRes.value;
    }

    RedactionPreviewDialog dlg(marks, analyses, doc->GetPath(), this);
    if (dlg.exec() == QDialog::Accepted) {
        std::filesystem::path dstPath = dlg.GetDestinationPath();
        auto repRes = services::RedactionService::Instance().ApplyRedactionsAndSave(*doc, doc->GetPath(), dstPath);
        if (repRes.has_value()) {
            if (m_redactionToolbar) m_redactionToolbar->UpdateMarkCount(0);
            RedactionReportDialog repDlg(repRes.value, this);
            repDlg.exec();

            // Offer to open newly redacted document
            auto reply = QMessageBox::question(
                this, tr("Open Redacted Copy?"),
                tr("Redactions applied successfully.\nWould you like to open the newly redacted document?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (reply == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(dstPath.string()));
            }
        } else {
            QMessageBox::critical(this, tr("Redaction Failed"), QString::fromStdString(repRes.errorMessage));
        }
    }
}

void MainWindow::OnSanitizeDocument() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to sanitize."));
        return;
    }

    auto doc = viewer->GetDocument();
    SanitizationDialog dlg(doc->GetPath(), this);
    if (dlg.exec() == QDialog::Accepted) {
        std::filesystem::path dstPath = dlg.GetDestinationPath();
        auto opt = dlg.GetOptions();

        // Engine-level clean copy + serialization scrub
        auto saveRes = doc->SaveCopy(dstPath);
        if (!saveRes.has_value()) {
            QMessageBox::critical(this, tr("Sanitization Save Failed"), QString::fromStdString(saveRes.errorMessage));
            return;
        }

        auto scrubRes = services::SanitizationService::SanitizeFile(dstPath, opt);
        if (scrubRes.has_value()) {
            auto reply = QMessageBox::question(
                this, tr("Sanitization Complete"),
                tr("Document successfully sanitized and saved to:\n%1\n\nWould you like to open the sanitized copy?")
                    .arg(QString::fromStdString(dstPath.string())),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (reply == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(dstPath.string()));
            }
        } else {
            QMessageBox::critical(this, tr("Sanitization Failed"), QString::fromStdString(scrubRes.errorMessage));
        }
    }
}

void MainWindow::OnOpenOcrStudio() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to run OCR."));
        return;
    }

    OcrStudioDialog dlg(viewer->GetDocument(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto outPath = dlg.GetDestinationPath();
        if (!outPath.empty() && std::filesystem::exists(outPath)) {
            auto reply = QMessageBox::question(
                this, tr("Open Searchable PDF"),
                tr("Searchable PDF created successfully.\nWould you like to open it now?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (reply == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(outPath.string()));
            }
        }
    }
}

void MainWindow::OnOpenConversionCenter() {
    auto engine = app::AppContext::Instance().GetPdfEngine();
    auto* viewer = m_workspace->GetCurrentViewer();
    auto doc = viewer ? viewer->GetDocument() : nullptr;

    ConversionCenterDialog dlg(doc, engine, this);
    if (dlg.exec() == QDialog::Accepted) {
        auto resPath = dlg.GetResultPath();
        if (!resPath.empty() && std::filesystem::exists(resPath) && resPath.extension() == ".pdf") {
            auto reply = QMessageBox::question(
                this, tr("Open Converted PDF"),
                tr("Converted PDF created successfully.\nWould you like to open it now?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (reply == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(resPath.string()));
            }
        }
    }
}

void MainWindow::OnOpenOptimizer() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to optimize."));
        return;
    }

    OptimizerDialog dlg(viewer->GetDocument(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto outPath = dlg.GetDestinationPath();
        if (!outPath.empty() && std::filesystem::exists(outPath)) {
            auto reply = QMessageBox::question(
                this, tr("Open Optimized PDF"),
                tr("Optimized PDF created successfully.\nWould you like to open it now?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
            );
            if (reply == QMessageBox::Yes) {
                OpenFileByPath(QString::fromStdString(outPath.string()));
            }
        }
    }
}

void MainWindow::OnOpenDocumentIntelligence() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document to inspect."));
        return;
    }

    DocumentIntelligenceDialog dlg(viewer->GetDocument(), this);
    dlg.exec();
}

void MainWindow::OnOpenComparisonWorkspace() {
    auto engine = app::AppContext::Instance().GetPdfEngine();
    auto* viewer = m_workspace->GetCurrentViewer();
    auto docA = viewer ? viewer->GetDocument() : nullptr;

    ComparisonWorkspaceDialog dlg(docA, engine, this);
    dlg.exec();
}

void MainWindow::OnOpenPreflightCompliance() {
    auto* viewer = m_workspace->GetCurrentViewer();
    if (!viewer || !viewer->GetDocument()) {
        QMessageBox::information(this, tr("No Document Open"), tr("Please open a PDF document for preflight & compliance analysis."));
        return;
    }

    PreflightComplianceDialog dlg(viewer->GetDocument(), this);
    dlg.exec();
}

void MainWindow::OnOpenBatchCenter() {
    BatchCenterDialog dlg(this);
    dlg.exec();
}

void MainWindow::OnOpenBatchProfileEditor() {
    BatchProfileEditorDialog dlg(this);
    dlg.exec();
}

void MainWindow::OnOpenMcpManager() {
    McpConfigDialog dlg(this);
    dlg.exec();
}

void MainWindow::OnOpenRemoteManager() {
    RemoteConfigDialog dlg(this);
    dlg.exec();
    UpdateRemoteStatusIndicator();
}

void MainWindow::UpdateRemoteStatusIndicator() {
    if (!m_statusRemoteLabel) return;

    auto& server = remote::RemoteControllerServer::Instance();
    auto state = server.GetSessionState();

    if (state.isConnected) {
        m_statusRemoteLabel->setText(tr("📡 Remote: Connected (%1)").arg(QString::fromStdString(state.activeClientName)));
        m_statusRemoteLabel->setStyleSheet(
            "background-color: #DCFCE7; color: #166534; "
            "padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 11px;"
        );
        m_statusRemoteLabel->setToolTip(tr("Connected remote client: %1 (%2)\nClick to manage or disconnect")
            .arg(QString::fromStdString(state.activeClientName))
            .arg(QString::fromStdString(state.activeIp)));
    } else if (server.IsListening()) {
        m_statusRemoteLabel->setText(tr("📡 Remote: Listening"));
        m_statusRemoteLabel->setStyleSheet(
            "background-color: #FEF3C7; color: #92400E; "
            "padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 11px;"
        );
        m_statusRemoteLabel->setToolTip(tr("Remote server listening on %1:%2 (No client connected)")
            .arg(QString::fromStdString(server.GetBoundAddress()))
            .arg(server.GetBoundPort()));
    } else {
        m_statusRemoteLabel->setText(tr("📡 Remote: Inactive"));
        m_statusRemoteLabel->setStyleSheet(
            "background-color: #F1F5F9; color: #64748B; "
            "padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 11px;"
        );
        m_statusRemoteLabel->setToolTip(tr("Remote Controller service is offline / disabled."));
    }
}

bool MainWindow::OnRemoteSensitiveActionConfirmation(const std::string& clientName,
                                                     const std::string& action,
                                                     const std::string& details) {
    int ret = QMessageBox::warning(
        this, tr("Remote Action Confirmation"),
        tr("Remote companion <b>%1</b> has requested a sensitive action:\n\n"
           "<b>Command:</b> %2\n"
           "<b>Details:</b> %3\n\n"
           "Do you want to allow this operation?")
            .arg(QString::fromStdString(clientName))
            .arg(QString::fromStdString(action))
            .arg(QString::fromStdString(details)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    return (ret == QMessageBox::Yes);
}

} // namespace pdfinity::ui
