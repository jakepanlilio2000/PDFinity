// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "HomeDashboardWidget.hpp"
#include "VectorIcons.hpp"
#include "persistence/RecentFilesManager.hpp"
#include "application/AppContext.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDateTime>
#include <QScrollArea>
#include <QFrame>
#include <QSplitter>
#include <QToolButton>

namespace pdfinity::ui {

HomeDashboardWidget::HomeDashboardWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    SetupUi();
    RefreshRecentFiles();
}

void HomeDashboardWidget::SetupUi() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. Left Sidebar Navigation
    rootLayout->addWidget(CreateSidebar(), 0);

    // 2. Right Main Scroll Area
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* contentWidget = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(22);

    // Header Section
    contentLayout->addWidget(CreateHeaderSection());

    // 7 Hero Quick-Action Cards
    contentLayout->addWidget(CreateWorkflowCardsSection());

    // Middle Area: Recent Documents Table + Dropzone
    auto* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(20);
    middleLayout->addWidget(CreateRecentFilesSection(), 7);
    middleLayout->addWidget(CreateDropzoneSection(), 3);

    contentLayout->addLayout(middleLayout);
    contentLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);
    rootLayout->addWidget(scrollArea, 1);
}

QWidget* HomeDashboardWidget::CreateSidebar() {
    auto* sidebar = new QFrame(this);
    sidebar->setFixedWidth(200);
    sidebar->setStyleSheet(
        "QFrame { background-color: #1E2024; border-right: 1px solid #34373E; }"
    );

    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(12, 20, 12, 16);
    layout->setSpacing(8);

    auto* sectionTitle = new QLabel(tr("STUDIO HOME"), sidebar);
    sectionTitle->setStyleSheet(
        "color: #A1A1AA; font-size: 11px; font-weight: 700; letter-spacing: 0.5px; padding-left: 6px; border: none;"
    );
    layout->addWidget(sectionTitle);

    m_sidebarList = new QListWidget(sidebar);
    m_sidebarList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item {"
        "  padding: 10px 12px; border-radius: 6px; color: #F4F4F5; font-size: 13px; font-weight: 500; margin-bottom: 2px;"
        "}"
        "QListWidget::item:hover { background-color: rgba(220, 38, 38, 0.12); color: #F4F4F5; }"
        "QListWidget::item:selected { background-color: #DC2626; color: #FFFFFF; font-weight: 600; }"
    );

    auto* itemRecent = new QListWidgetItem(tr("⏱ Recent"), m_sidebarList);
    auto* itemStarred = new QListWidgetItem(tr("⭐ Starred / Pinned"), m_sidebarList);
    auto* itemLocal = new QListWidgetItem(tr("📁 Local Files"), m_sidebarList);
    auto* itemBatch = new QListWidgetItem(tr("⚡ Batch & Automate"), m_sidebarList);

    m_sidebarList->addItem(itemRecent);
    m_sidebarList->addItem(itemStarred);
    m_sidebarList->addItem(itemLocal);
    m_sidebarList->addItem(itemBatch);

    m_sidebarList->setCurrentRow(0);
    connect(m_sidebarList, &QListWidget::currentRowChanged, this, &HomeDashboardWidget::OnSidebarItemChanged);

    layout->addWidget(m_sidebarList);
    layout->addStretch();

    return sidebar;
}

void HomeDashboardWidget::OnSidebarItemChanged(int row) {
    if (row == 0) {
        m_currentSection = DashboardSection::Recent;
        RefreshRecentFiles();
    } else if (row == 1) {
        m_currentSection = DashboardSection::Starred;
        RefreshRecentFiles();
    } else if (row == 2) {
        emit OpenFileRequested();
        m_sidebarList->setCurrentRow(0);
    } else if (row == 3) {
        emit BatchRequested();
        m_sidebarList->setCurrentRow(0);
    }
}

QWidget* HomeDashboardWidget::CreateHeaderSection() {
    auto* header = new QFrame(this);
    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* textLayout = new QVBoxLayout();
    auto* titleLabel = new QLabel(tr("<h2>PDFinity Document Studio</h2>"), header);
    titleLabel->setStyleSheet("color: #F4F4F5; font-weight: 700; margin: 0;");

    auto* subtitleLabel = new QLabel(
        tr("Professional • Modular • Offline-First PDF Productivity Workstation"), header
    );
    subtitleLabel->setStyleSheet("color: #A1A1AA; font-size: 13px; font-weight: 500;");

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);
    layout->addLayout(textLayout);
    layout->addStretch();

    // Offline Security Guarantee Badge
    auto* shieldPill = new QFrame(header);
    shieldPill->setStyleSheet(
        "background-color: rgba(220, 38, 38, 0.12); border: 1px solid #DC2626; border-radius: 16px; padding: 6px 14px;"
    );
    auto* shieldLayout = new QHBoxLayout(shieldPill);
    shieldLayout->setContentsMargins(0, 0, 0, 0);
    shieldLayout->setSpacing(8);

    auto* shieldIcon = new QLabel(shieldPill);
    shieldIcon->setPixmap(VectorIcons::CreatePixmap(IconType::Shield, 18, QColor("#DC2626")));
    auto* shieldText = new QLabel(tr("<b>100% Offline • Zero Telemetry • Local Execution</b>"), shieldPill);
    shieldText->setStyleSheet("color: #DC2626; font-size: 12px; font-weight: 600;");

    shieldLayout->addWidget(shieldIcon);
    shieldLayout->addWidget(shieldText);
    layout->addWidget(shieldPill);

    return header;
}

QWidget* HomeDashboardWidget::CreateWorkflowCardsSection() {
    auto* container = new QWidget(this);
    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);

    struct HeroCardDef {
        QString title;
        QString desc;
        IconType icon;
        std::function<void()> onClick;
    };

    // 7 Hero Quick-Action Cards with Crimson Accent
    std::vector<HeroCardDef> cards = {
        {tr("Open PDF"), tr("Browse and open any document (Ctrl+O)"), IconType::Open, [this]() { emit OpenFileRequested(); }},
        {tr("Create from Images"), tr("Convert JPEG, PNG, TIFF, or BMP into PDF"), IconType::ContentEdit, [this]() { emit CreateFromImagesRequested(); }},
        {tr("Combine Files"), tr("Merge multiple documents into a single PDF"), IconType::Merge, [this]() { emit CombineFilesRequested(); }},
        {tr("Edit PDF"), tr("Direct page text and object manipulation"), IconType::ContentEdit, [this]() { emit EditPdfRequested(); }},
        {tr("Fill & Sign"), tr("Fill AcroForms and place digital signatures"), IconType::DigitalSign, [this]() { emit SignRequested(); }},
        {tr("Protect & Redact"), tr("Cryptographic encryption and sanitization"), IconType::Redaction, [this]() { emit RedactRequested(); }},
        {tr("OCR Text Recognition"), tr("Generate searchable text from scanned pages"), IconType::Ocr, [this]() { emit OcrRequested(); }}
    };

    int col = 0;
    int row = 0;
    const int maxCols = 4;

    for (const auto& card : cards) {
        auto* btn = new QPushButton(container);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #27292E; border: 1px solid #34373E; border-radius: 8px;"
            "  padding: 14px; text-align: left; min-height: 80px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #2D3036; border-color: #DC2626;"
            "}"
            "QPushButton:pressed {"
            "  background-color: rgba(220, 38, 38, 0.15);"
            "}"
        );

        auto* btnLayout = new QHBoxLayout(btn);
        btnLayout->setContentsMargins(6, 6, 6, 6);
        btnLayout->setSpacing(12);

        auto* iconLabel = new QLabel(btn);
        iconLabel->setPixmap(VectorIcons::CreatePixmap(card.icon, 28, QColor("#DC2626")));
        btnLayout->addWidget(iconLabel, 0, Qt::AlignTop);

        auto* textLayout = new QVBoxLayout();
        textLayout->setSpacing(3);

        auto* titleLbl = new QLabel(card.title, btn);
        titleLbl->setStyleSheet("color: #F4F4F5; font-size: 13px; font-weight: 700; border: none;");

        auto* descLbl = new QLabel(card.desc, btn);
        descLbl->setStyleSheet("color: #A1A1AA; font-size: 11px; font-weight: 400; border: none;");
        descLbl->setWordWrap(true);

        textLayout->addWidget(titleLbl);
        textLayout->addWidget(descLbl);
        btnLayout->addLayout(textLayout, 1);

        connect(btn, &QPushButton::clicked, this, card.onClick);
        grid->addWidget(btn, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    return container;
}

QWidget* HomeDashboardWidget::CreateRecentFilesSection() {
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // Section title & Filter Toolbar
    auto* barLayout = new QHBoxLayout();
    auto* sectionTitle = new QLabel(tr("Recent Documents"), container);
    sectionTitle->setStyleSheet("color: #F4F4F5; font-size: 16px; font-weight: 700;");
    barLayout->addWidget(sectionTitle);
    barLayout->addStretch();

    m_searchFilterEdit = new QLineEdit(container);
    m_searchFilterEdit->setPlaceholderText(tr("Search recent documents..."));
    m_searchFilterEdit->setFixedWidth(240);
    m_searchFilterEdit->setClearButtonEnabled(true);
    connect(m_searchFilterEdit, &QLineEdit::textChanged, this, &HomeDashboardWidget::OnFilterTextChanged);
    barLayout->addWidget(m_searchFilterEdit);

    auto* clearBtn = new QPushButton(tr("Clear History"), container);
    clearBtn->setStyleSheet("font-size: 11px; padding: 4px 10px;");
    connect(clearBtn, &QPushButton::clicked, this, &HomeDashboardWidget::OnClearRecentClicked);
    barLayout->addWidget(clearBtn);

    layout->addLayout(barLayout);

    // Recent Documents Table
    m_recentTable = new QTableWidget(container);
    m_recentTable->setColumnCount(6);
    m_recentTable->setHorizontalHeaderLabels({
        tr("Pin"), tr("Document Name"), tr("Location"), tr("Last Opened"), tr("Size"), tr("Action")
    });

    m_recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTable->verticalHeader()->setVisible(false);
    m_recentTable->setShowGrid(false);
    m_recentTable->setSortingEnabled(true);
    m_recentTable->setMinimumHeight(240);

    m_recentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_recentTable->setColumnWidth(0, 42);
    m_recentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_recentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_recentTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_recentTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_recentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_recentTable->setColumnWidth(5, 60);

    connect(m_recentTable, &QTableWidget::cellDoubleClicked, this, &HomeDashboardWidget::OnRecentCellDoubleClicked);
    layout->addWidget(m_recentTable);

    return container;
}

QWidget* HomeDashboardWidget::CreateDropzoneSection() {
    auto* frame = new QFrame(this);
    frame->setStyleSheet(
        "QFrame {"
        "  border: 2px dashed #34373E; border-radius: 8px; background-color: rgba(39, 41, 46, 0.5);"
        "}"
        "QFrame:hover {"
        "  border-color: #DC2626; background-color: rgba(220, 38, 38, 0.05);"
        "}"
    );

    auto* layout = new QVBoxLayout(frame);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    auto* iconLabel = new QLabel(frame);
    iconLabel->setPixmap(VectorIcons::CreatePixmap(IconType::Open, 36, QColor("#A1A1AA")));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("border: none; background: transparent;");

    auto* promptLabel = new QLabel(tr("<b>Drop PDF files here</b><br><span style='font-size: 11px; color: #A1A1AA;'>or click to browse local files</span>"), frame);
    promptLabel->setAlignment(Qt::AlignCenter);
    promptLabel->setStyleSheet("border: none; background: transparent; color: #F4F4F5; font-size: 13px;");

    auto* browseBtn = new QPushButton(tr("Browse Files..."), frame);
    browseBtn->setObjectName("primaryButton");
    connect(browseBtn, &QPushButton::clicked, this, &HomeDashboardWidget::OpenFileRequested);

    layout->addWidget(iconLabel);
    layout->addWidget(promptLabel);
    layout->addWidget(browseBtn);

    return frame;
}

namespace {
QString FormatBytes(uint64_t bytes) {
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
}
}

void HomeDashboardWidget::RefreshRecentFiles() {
    if (!m_recentTable) return;

    m_recentTable->setSortingEnabled(false);
    m_recentTable->setRowCount(0);

    const auto& recents = app::AppContext::Instance().GetRecentFiles().GetRecentFiles();
    QString filter = m_searchFilterEdit ? m_searchFilterEdit->text().trimmed().toLower() : "";

    int row = 0;
    for (const auto& item : recents) {
        QString pathStr = QString::fromStdString(item.canonicalPath);
        QString fileName = QString::fromStdString(item.fileName);

        bool isPinned = (m_pinnedPaths.find(item.canonicalPath) != m_pinnedPaths.end());

        // Apply sidebar section filtering
        if (m_currentSection == DashboardSection::Starred && !isPinned) {
            continue;
        }

        // Apply text filter
        if (!filter.isEmpty() && !fileName.toLower().contains(filter) && !pathStr.toLower().contains(filter)) {
            continue;
        }

        m_recentTable->insertRow(row);

        // Column 0: Star / Pin Button
        auto* pinBtn = new QToolButton(m_recentTable);
        pinBtn->setText(isPinned ? "⭐" : "☆");
        pinBtn->setToolTip(isPinned ? tr("Unpin document") : tr("Pin document to top"));
        pinBtn->setStyleSheet("QToolButton { border: none; font-size: 14px; background: transparent; }");
        connect(pinBtn, &QToolButton::clicked, this, [this, pathStr]() {
            OnTogglePin(pathStr);
        });
        m_recentTable->setCellWidget(row, 0, pinBtn);

        // Column 1: Document Name
        auto* nameItem = new QTableWidgetItem(fileName);
        nameItem->setIcon(VectorIcons::GetIcon(IconType::Open));
        nameItem->setData(Qt::UserRole, pathStr);
        m_recentTable->setItem(row, 1, nameItem);

        // Column 2: Folder Location
        QFileInfo fi(pathStr);
        auto* dirItem = new QTableWidgetItem(fi.absolutePath());
        dirItem->setData(Qt::UserRole, pathStr);
        m_recentTable->setItem(row, 2, dirItem);

        // Column 3: Last Opened
        QString dateStr = QString::fromStdString(item.lastOpenedIso);
        if (dateStr.length() >= 10) dateStr = dateStr.left(10);
        auto* dateItem = new QTableWidgetItem(dateStr);
        dateItem->setData(Qt::UserRole, pathStr);
        m_recentTable->setItem(row, 3, dateItem);

        // Column 4: Size
        auto* sizeItem = new QTableWidgetItem(FormatBytes(item.fileSizeBytes));
        sizeItem->setData(Qt::UserRole, pathStr);
        m_recentTable->setItem(row, 4, sizeItem);

        // Column 5: Remove Action Button
        auto* removeBtn = new QToolButton(m_recentTable);
        removeBtn->setText("✕");
        removeBtn->setToolTip(tr("Remove from recent list"));
        removeBtn->setStyleSheet("QToolButton { border: none; font-size: 11px; color: #A1A1AA; background: transparent; } QToolButton:hover { color: #DC2626; }");
        connect(removeBtn, &QToolButton::clicked, this, [this, pathStr]() {
            OnRemoveRecent(pathStr);
        });
        m_recentTable->setCellWidget(row, 5, removeBtn);

        row++;
    }

    m_recentTable->setSortingEnabled(true);
}

void HomeDashboardWidget::OnTogglePin(const QString& filePath) {
    std::string path = filePath.toStdString();
    if (m_pinnedPaths.find(path) != m_pinnedPaths.end()) {
        m_pinnedPaths.erase(path);
    } else {
        m_pinnedPaths.insert(path);
    }
    RefreshRecentFiles();
}

void HomeDashboardWidget::OnRemoveRecent(const QString& filePath) {
    app::AppContext::Instance().GetRecentFiles().RemoveFile(std::filesystem::path(filePath.toStdString()));
    m_pinnedPaths.erase(filePath.toStdString());
    RefreshRecentFiles();
}

void HomeDashboardWidget::OnFilterTextChanged(const QString& /*text*/) {
    RefreshRecentFiles();
}

void HomeDashboardWidget::OnRecentCellDoubleClicked(int row, int /*column*/) {
    auto* item = m_recentTable->item(row, 1);
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            emit FileSelected(path);
        }
    }
}

void HomeDashboardWidget::OnClearRecentClicked() {
    app::AppContext::Instance().GetRecentFiles().Clear();
    m_pinnedPaths.clear();
    RefreshRecentFiles();
}

void HomeDashboardWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void HomeDashboardWidget::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    for (const auto& url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            if (path.endsWith(".pdf", Qt::CaseInsensitive)) {
                emit FileSelected(path);
                break;
            }
        }
    }
}

} // namespace pdfinity::ui
