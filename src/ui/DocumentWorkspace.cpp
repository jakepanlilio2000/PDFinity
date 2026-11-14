// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentWorkspace.hpp"
#include "PdfViewerWidget.hpp"
#include "HomeDashboardWidget.hpp"
#include "ContextInspectorPanel.hpp"
#include "VectorIcons.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"
#include "security/PathSanitizer.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QGroupBox>

namespace pdfinity::ui {

namespace {
QString FormatFileSize(uint64_t bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
}
} // namespace

DocumentWorkspace::DocumentWorkspace(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    SetupUi();
}

void DocumentWorkspace::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    // Pinned Home Dashboard as Tab 0
    m_homeDashboard = new HomeDashboardWidget(this);
    connect(m_homeDashboard, &HomeDashboardWidget::OpenFileRequested, this, &DocumentWorkspace::OpenFileRequested);
    connect(m_homeDashboard, &HomeDashboardWidget::FileSelected, this, &DocumentWorkspace::FileSelected);

    int homeIndex = m_tabWidget->addTab(m_homeDashboard, VectorIcons::GetIcon(IconType::Home), tr("Home"));
    if (m_tabWidget->tabBar()) {
        m_tabWidget->tabBar()->setTabButton(homeIndex, QTabBar::RightSide, nullptr);
        m_tabWidget->tabBar()->setTabButton(homeIndex, QTabBar::LeftSide, nullptr);
    }

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &DocumentWorkspace::OnTabCloseRequested);

    // Corner widget '+' button for opening new tab
    auto* newTabBtn = new QToolButton(m_tabWidget);
    newTabBtn->setText("+");
    newTabBtn->setToolTip(tr("Open Document in New Tab (Ctrl+O)"));
    newTabBtn->setStyleSheet(
        "QToolButton { font-weight: bold; font-size: 14px; border: none; padding: 4px 8px; color: #64748b; }"
        "QToolButton:hover { background: #e2e8f0; border-radius: 4px; color: #1e293b; }"
    );
    connect(newTabBtn, &QToolButton::clicked, this, &DocumentWorkspace::OpenFileRequested);
    m_tabWidget->setCornerWidget(newTabBtn, Qt::TopRightCorner);

    mainLayout->addWidget(m_tabWidget);
    m_tabWidget->setCurrentIndex(0);
}

void DocumentWorkspace::ShowHomeTab() {
    if (m_tabWidget) {
        m_tabWidget->setCurrentIndex(0);
    }
}

QWidget* DocumentWorkspace::CreateWelcomeView() {
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(40, 40, 40, 30);
    layout->setSpacing(20);

    // Header banner
    auto* bannerLayout = new QVBoxLayout();
    bannerLayout->setAlignment(Qt::AlignCenter);

    auto* titleLabel = new QLabel(tr("<h1>PDFinity</h1>"), container);
    titleLabel->setAlignment(Qt::AlignCenter);
    bannerLayout->addWidget(titleLabel);

    auto* descLabel = new QLabel(tr("<p style='font-size: 14px; color: #6b7280;'>"
                                    "Professional • Modular • Offline-First PDF Productivity Suite</p>"), container);
    descLabel->setAlignment(Qt::AlignCenter);
    bannerLayout->addWidget(descLabel);

    // Primary Action Button
    auto* openBtn = new QPushButton(tr("  Open PDF Document...  "), container);
    openBtn->setObjectName("primaryButton");
    openBtn->setFixedHeight(42);
    openBtn->setMinimumWidth(220);
    openBtn->setCursor(Qt::PointingHandCursor);
    connect(openBtn, &QPushButton::clicked, this, &DocumentWorkspace::OpenFileRequested);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->addWidget(openBtn);
    bannerLayout->addLayout(btnLayout);

    layout->addLayout(bannerLayout);

    // Drag & Drop Hint Box
    auto* dropBox = new QFrame(container);
    dropBox->setStyleSheet("QFrame { border: 2px dashed #9ca3af; border-radius: 8px; background-color: rgba(156, 163, 175, 0.05); }");
    dropBox->setFixedHeight(60);
    auto* dropLayout = new QVBoxLayout(dropBox);
    auto* dropLabel = new QLabel(tr("Drop PDF files here to open"), dropBox);
    dropLabel->setStyleSheet("color: #6b7280; font-size: 13px; font-weight: 500; border: none;");
    dropLabel->setAlignment(Qt::AlignCenter);
    dropLayout->addWidget(dropLabel);
    layout->addWidget(dropBox);

    // Recent Files Section
    auto* recentHeaderLayout = new QHBoxLayout();
    auto* recentTitle = new QLabel(tr("<b>Recent Documents</b>"), container);
    recentTitle->setStyleSheet("font-size: 14px;");
    recentHeaderLayout->addWidget(recentTitle);
    recentHeaderLayout->addStretch();

    auto* clearRecentBtn = new QPushButton(tr("Clear History"), container);
    clearRecentBtn->setFixedHeight(28);
    connect(clearRecentBtn, &QPushButton::clicked, this, &DocumentWorkspace::OnClearRecentClicked);
    recentHeaderLayout->addWidget(clearRecentBtn);
    layout->addLayout(recentHeaderLayout);

    m_recentListWidget = new QListWidget(container);
    m_recentListWidget->setMinimumHeight(180);
    connect(m_recentListWidget, &QListWidget::itemDoubleClicked, this, &DocumentWorkspace::OnRecentItemDoubleClicked);
    layout->addWidget(m_recentListWidget);

    // Security & Architecture Badges
    auto* badgeLayout = new QHBoxLayout();
    badgeLayout->setAlignment(Qt::AlignCenter);
    badgeLayout->setSpacing(12);

    const char* badges[] = {
        "100% Offline Core",
        "Google PDFium Engine",
        "Zero Telemetry",
        "Air-Gapped Ready",
        "Atomic File Safety",
        "Per-Monitor DPI v2"
    };

    for (const auto* b : badges) {
        auto* badge = new QLabel(QString("✓ %1").arg(b), container);
        badge->setStyleSheet("background-color: rgba(220, 38, 38, 0.12); color: #DC2626; "
                             "padding: 4px 10px; border-radius: 12px; font-size: 11px; font-weight: 600;");
        badgeLayout->addWidget(badge);
    }
    layout->addLayout(badgeLayout);

    return container;
}

void DocumentWorkspace::RefreshRecentFilesList() {
    m_recentListWidget->clear();
    const auto& recents = app::AppContext::Instance().GetRecentFiles().GetRecentFiles();

    if (recents.empty()) {
        auto* item = new QListWidgetItem(tr("No recent documents. Open a PDF to begin."), m_recentListWidget);
        item->setFlags(Qt::NoItemFlags);
        return;
    }

    for (const auto& item : recents) {
        QString text = QString("%1  •  %2  •  %3")
            .arg(QString::fromStdString(item.fileName))
            .arg(FormatFileSize(item.fileSizeBytes))
            .arg(QString::fromStdString(item.canonicalPath));

        auto* listItem = new QListWidgetItem(text, m_recentListWidget);
        listItem->setData(Qt::UserRole, QString::fromStdString(item.canonicalPath));
        listItem->setToolTip(QString::fromStdString(item.canonicalPath));
    }
}

QWidget* DocumentWorkspace::CreateDocumentPane(std::unique_ptr<domain::DocumentModel> doc) {
    auto* tabPane = new QWidget(m_tabWidget);
    auto* paneLayout = new QVBoxLayout(tabPane);
    paneLayout->setContentsMargins(18, 18, 18, 18);
    paneLayout->setSpacing(14);

    // Header Summary Frame
    auto* docHeader = new QFrame(tabPane);
    docHeader->setStyleSheet("background-color: rgba(156, 163, 175, 0.08); border-radius: 6px; padding: 12px;");
    auto* headerLayout = new QHBoxLayout(docHeader);

    QString fileName = QFileInfo(QString::fromStdString(doc->GetPath().string())).fileName();
    auto* nameLabel = new QLabel(QString("<b>%1</b>").arg(fileName), docHeader);
    nameLabel->setStyleSheet("font-size: 15px;");
    headerLayout->addWidget(nameLabel);
    headerLayout->addStretch();

    QString details = QString("%1 pages  •  %2").arg(doc->GetPageCount()).arg(FormatFileSize(doc->GetFileSize()));
    auto* sizeLabel = new QLabel(details, docHeader);
    sizeLabel->setStyleSheet("color: #6b7280; font-weight: 500;");
    headerLayout->addWidget(sizeLabel);
    paneLayout->addWidget(docHeader);

    // Metadata and Permissions Panel
    auto* metaGroup = new QGroupBox(tr("Document Inspection & Properties"), tabPane);
    auto* formLayout = new QFormLayout(metaGroup);

    const auto& meta = doc->GetMetadata();
    if (!meta.title.empty()) formLayout->addRow(tr("Title:"), new QLabel(QString::fromStdString(meta.title)));
    if (!meta.author.empty()) formLayout->addRow(tr("Author:"), new QLabel(QString::fromStdString(meta.author)));
    if (!meta.subject.empty()) formLayout->addRow(tr("Subject:"), new QLabel(QString::fromStdString(meta.subject)));

    // Page 1 Dimensions
    auto sizeRes = doc->GetPageSize(0);
    if (sizeRes.has_value()) {
        QString dimStr = QString("%1 × %2 pt (PDF Units)")
            .arg(QString::number(sizeRes.value.width, 'f', 1))
            .arg(QString::number(sizeRes.value.height, 'f', 1));
        formLayout->addRow(tr("Initial Page Size:"), new QLabel(dimStr));
    }

    const auto& perms = doc->GetPermissions();
    QString permStr = QString("Printing: %1  |  Modifying: %2  |  Copying: %3  |  Encrypted: %4")
        .arg(perms.canPrint ? "Allowed" : "Prohibited")
        .arg(perms.canModifyContents ? "Allowed" : "Prohibited")
        .arg(perms.canCopyText ? "Allowed" : "Prohibited")
        .arg(perms.isEncrypted ? "Yes (Protected)" : "No (Standard)");
    formLayout->addRow(tr("Security & Permissions:"), new QLabel(permStr));

    paneLayout->addWidget(metaGroup);

    // Center viewport status area
    auto* centerArea = new QWidget(tabPane);
    auto* centerLayout = new QVBoxLayout(centerArea);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto* statusMsg = new QLabel(tr(
        "<div style='text-align: center; color: #6b7280;'>"
        "<h3>Document Verified and Mounted</h3>"
        "<p>Google PDFium engine successfully verified xref structure and page tree.</p>"
        "</div>"
    ), centerArea);
    centerLayout->addWidget(statusMsg);
    paneLayout->addWidget(centerArea);

    return tabPane;
}

bool DocumentWorkspace::OpenDocumentTab(const std::filesystem::path& filePath, const std::string& password) {
    auto& lifecycle = app::AppContext::Instance().GetLifecycleService();

    auto res = lifecycle.OpenDocument(filePath, security::SecureString(password));
    if (!res.has_value()) {
        if (res.error == pdf_api::EngineError::PasswordRequired || res.error == pdf_api::EngineError::IncorrectPassword) {
            bool ok = false;
            QString dialogMessage = (res.error == pdf_api::EngineError::IncorrectPassword)
                ? tr("Incorrect password. Please re-enter password:")
                : tr("This document is password protected. Enter password:");

            QString entered = QInputDialog::getText(
                this,
                tr("Password Required"),
                dialogMessage,
                QLineEdit::Password,
                {},
                &ok
            );

            if (ok && !entered.isEmpty()) {
                return OpenDocumentTab(filePath, entered.toStdString());
            }
            return false;
        }

        QMessageBox::warning(
            this,
            tr("Failed to Open Document"),
            QString::fromStdString(pdf_api::GetEngineErrorMessage(res.error))
        );
        return false;
    }

    auto doc = std::move(res.value);
    QString qPath = QString::fromStdString(filePath.string());
    QString fileName = QFileInfo(qPath).fileName();

    // Check if already open (tabs 1..N)
    for (int i = 1; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabToolTip(i) == qPath) {
            m_tabWidget->setCurrentIndex(i);
            return true;
        }
    }

    auto* viewer = new PdfViewerWidget(std::move(doc), m_tabWidget);
    if (auto* insp = viewer->GetInspector()) {
        connect(insp, &ContextInspectorPanel::OrganizePagesTriggered, this, &DocumentWorkspace::OrganizePagesRequested);
        connect(insp, &ContextInspectorPanel::MergeDocumentsTriggered, this, &DocumentWorkspace::MergeDocumentsRequested);
        connect(insp, &ContextInspectorPanel::SplitDocumentTriggered, this, &DocumentWorkspace::SplitDocumentRequested);
        connect(insp, &ContextInspectorPanel::ContentEditorTriggered, this, &DocumentWorkspace::ContentEditorRequested);
        connect(insp, &ContextInspectorPanel::FormInspectorTriggered, this, &DocumentWorkspace::FormInspectorRequested);
        connect(insp, &ContextInspectorPanel::ObjectInspectorTriggered, this, &DocumentWorkspace::ObjectInspectorRequested);
        connect(insp, &ContextInspectorPanel::DigitalSignTriggered, this, &DocumentWorkspace::DigitalSignRequested);
        connect(insp, &ContextInspectorPanel::VisualSignTriggered, this, &DocumentWorkspace::VisualSignRequested);
        connect(insp, &ContextInspectorPanel::ValidateSignaturesTriggered, this, &DocumentWorkspace::ValidateSignaturesRequested);
        connect(insp, &ContextInspectorPanel::RedactionStudioTriggered, this, &DocumentWorkspace::RedactionStudioRequested);
        connect(insp, &ContextInspectorPanel::SanitizeTriggered, this, &DocumentWorkspace::SanitizeRequested);
        connect(insp, &ContextInspectorPanel::CertificateManagerTriggered, this, &DocumentWorkspace::CertificateManagerRequested);
        connect(insp, &ContextInspectorPanel::OcrStudioTriggered, this, &DocumentWorkspace::OcrStudioRequested);
        connect(insp, &ContextInspectorPanel::ConversionCenterTriggered, this, &DocumentWorkspace::ConversionCenterRequested);
        connect(insp, &ContextInspectorPanel::OptimizerTriggered, this, &DocumentWorkspace::OptimizerRequested);
        connect(insp, &ContextInspectorPanel::DocumentIntelligenceTriggered, this, &DocumentWorkspace::DocumentIntelligenceRequested);
        connect(insp, &ContextInspectorPanel::CompareDocumentsTriggered, this, &DocumentWorkspace::CompareDocumentsRequested);
        connect(insp, &ContextInspectorPanel::BatchCenterTriggered, this, &DocumentWorkspace::BatchCenterRequested);
    }

    int newIndex = m_tabWidget->addTab(viewer, fileName);
    m_tabWidget->setTabToolTip(newIndex, qPath);
    m_tabWidget->setCurrentIndex(newIndex);

    // Add to recent files
    app::AppContext::Instance().GetRecentFiles().AddFile(filePath);
    RefreshRecentFilesList();

    emit DocumentCountChanged(GetOpenDocumentCount());
    LOG_INFO("Workspace", "Mounted document tab for: " + filePath.string());
    return true;
}

void DocumentWorkspace::CloseCurrentTab() {
    int current = m_tabWidget->currentIndex();
    if (current > 0) {
        OnTabCloseRequested(current);
    }
}

void DocumentWorkspace::CloseAllTabs() {
    while (m_tabWidget->count() > 1) {
        OnTabCloseRequested(1);
    }
}

int DocumentWorkspace::GetOpenDocumentCount() const {
    return std::max(0, m_tabWidget->count() - 1);
}

QString DocumentWorkspace::GetCurrentDocumentPath() const {
    int current = m_tabWidget->currentIndex();
    if (current > 0) {
        return m_tabWidget->tabToolTip(current);
    }
    return {};
}

PdfViewerWidget* DocumentWorkspace::GetCurrentViewer() const {
    int current = m_tabWidget->currentIndex();
    if (current > 0) {
        return qobject_cast<PdfViewerWidget*>(m_tabWidget->widget(current));
    }
    return nullptr;
}

domain::DocumentModel* DocumentWorkspace::GetCurrentDocument() const {
    auto* viewer = GetCurrentViewer();
    return viewer ? viewer->GetDocument().get() : nullptr;
}

void DocumentWorkspace::SaveSession() {
    auto& config = app::AppContext::Instance().GetConfig();
    config.sessionOpenFiles.clear();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        config.sessionOpenFiles.push_back(m_tabWidget->tabToolTip(i).toStdString());
    }
    config.sessionActiveTabIndex = m_tabWidget->currentIndex();
}

void DocumentWorkspace::RestoreSession() {
    const auto& config = app::AppContext::Instance().GetConfig();
    if (!config.restoreSessionOnStartup || config.sessionOpenFiles.empty()) {
        return;
    }

    LOG_INFO("Workspace", "Restoring previous session with " + std::to_string(config.sessionOpenFiles.size()) + " document(s)...");
    for (const auto& filePathStr : config.sessionOpenFiles) {
        std::filesystem::path p(filePathStr);
        if (std::filesystem::exists(p)) {
            OpenDocumentTab(p);
        }
    }

    if (config.sessionActiveTabIndex >= 0 && config.sessionActiveTabIndex < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(config.sessionActiveTabIndex);
    }
}

void DocumentWorkspace::OnTabCloseRequested(int index) {
    if (index > 0 && index < m_tabWidget->count()) {
        QWidget* w = m_tabWidget->widget(index);
        m_tabWidget->removeTab(index);
        delete w;

        if (m_tabWidget->count() <= 1) {
            m_tabWidget->setCurrentIndex(0); // Switch to Home Tab
            RefreshRecentFilesList();
        }
        emit DocumentCountChanged(GetOpenDocumentCount());
    }
}

void DocumentWorkspace::OnRecentItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        emit FileSelected(path);
    }
}

void DocumentWorkspace::OnClearRecentClicked() {
    app::AppContext::Instance().GetRecentFiles().Clear();
    RefreshRecentFilesList();
}

void DocumentWorkspace::RefreshRecentFilesList() {
    if (m_homeDashboard) {
        m_homeDashboard->RefreshRecentFiles();
    }
}

void DocumentWorkspace::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const auto& url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".pdf", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void DocumentWorkspace::dropEvent(QDropEvent* event) {
    const auto& urls = event->mimeData()->urls();
    for (const auto& url : urls) {
        QString localPath = url.toLocalFile();
        if (localPath.endsWith(".pdf", Qt::CaseInsensitive)) {
            emit FileSelected(localPath);
        }
    }
    event->acceptProposedAction();
}

} // namespace pdfinity::ui
