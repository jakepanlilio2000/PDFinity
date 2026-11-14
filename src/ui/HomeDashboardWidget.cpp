// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "HomeDashboardWidget.hpp"
#include "VectorIcons.hpp"
#include "persistence/RecentFilesManager.hpp"
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

namespace pdfinity::ui {

HomeDashboardWidget::HomeDashboardWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    SetupUi();
    RefreshRecentFiles();
}

void HomeDashboardWidget::SetupUi() {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* contentWidget = new QWidget(scrollArea);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(32, 28, 32, 32);
    contentLayout->setSpacing(24);

    // 1. Header Section
    contentLayout->addWidget(CreateHeaderSection());

    // 2. Workflow Quick Action Cards
    contentLayout->addWidget(CreateWorkflowCardsSection());

    // 3. Middle Area: Recent Files & Dropzone in a 2-column layout
    auto* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(24);
    middleLayout->addWidget(CreateRecentFilesSection(), 6);
    middleLayout->addWidget(CreateDropzoneSection(), 4);

    contentLayout->addLayout(middleLayout);
    contentLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);
}

QWidget* HomeDashboardWidget::CreateHeaderSection() {
    auto* header = new QFrame(this);
    auto* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* textLayout = new QVBoxLayout();
    auto* titleLabel = new QLabel(tr("<h2>PDFinity Document Studio</h2>"), header);
    titleLabel->setStyleSheet("color: #1e293b; font-weight: 700; margin: 0;");

    auto* subtitleLabel = new QLabel(
        tr("Professional • Modular • Offline-First PDF Productivity Suite"), header
    );
    subtitleLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 500;");

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(subtitleLabel);
    layout->addLayout(textLayout);
    layout->addStretch();

    // Security Guarantee Pill
    auto* shieldPill = new QFrame(header);
    shieldPill->setStyleSheet(
        "background-color: #ecfdf5; border: 1px solid #10b981; border-radius: 16px; padding: 6px 14px;"
    );
    auto* shieldLayout = new QHBoxLayout(shieldPill);
    shieldLayout->setContentsMargins(0, 0, 0, 0);
    shieldLayout->setSpacing(8);

    auto* shieldIcon = new QLabel(shieldPill);
    shieldIcon->setPixmap(VectorIcons::CreatePixmap(IconType::Shield, 18, QColor(16, 185, 129)));
    auto* shieldText = new QLabel(tr("<b>100% Offline • Zero Telemetry • Local Execution</b>"), shieldPill);
    shieldText->setStyleSheet("color: #065f46; font-size: 12px;");

    shieldLayout->addWidget(shieldIcon);
    shieldLayout->addWidget(shieldText);
    layout->addWidget(shieldPill);

    return header;
}

QWidget* HomeDashboardWidget::CreateWorkflowCardsSection() {
    auto* container = new QWidget(this);
    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(14);

    struct CardDef {
        QString title;
        QString desc;
        IconType icon;
        std::function<void()> onClick;
    };

    std::vector<CardDef> cards = {
        {tr("Open Document"), tr("Open any standard PDF document (Ctrl+O)"), IconType::Open, [this]() { emit OpenFileRequested(); }},
        {tr("Organize & Assemble"), tr("Merge, split, extract, rotate, and reorder pages"), IconType::Organize, [this]() { emit OrganizeRequested(); }},
        {tr("Review & Annotate"), tr("Highlight text, add sticky notes, and vector callouts"), IconType::Annotations, [this]() { emit AnnotateRequested(); }},
        {tr("Fill Forms & Sign"), tr("Fill interactive AcroForms and apply digital signatures"), IconType::DigitalSign, [this]() { emit SignRequested(); }},
        {tr("Protect & Redact"), tr("Irreversible content-stream redaction and metadata scrub"), IconType::Redaction, [this]() { emit RedactRequested(); }},
        {tr("OCR Studio"), tr("Recognize scanned text and create searchable layers"), IconType::Ocr, [this]() { emit OcrRequested(); }},
        {tr("Document Intelligence"), tr("Inspect fonts, images, layers, and preflight rules"), IconType::Properties, [this]() { emit IntelligenceRequested(); }},
        {tr("Batch Automation"), tr("Automate multi-file profiles and batch workflows"), IconType::Batch, [this]() { emit BatchRequested(); }}
    };

    for (size_t i = 0; i < cards.size(); ++i) {
        int r = static_cast<int>(i / 4);
        int c = static_cast<int>(i % 4);

        auto* cardBtn = new QPushButton(container);
        cardBtn->setStyleSheet(
            "QPushButton {"
            "  background: #ffffff;"
            "  border: 1px solid #e2e8f0;"
            "  border-radius: 10px;"
            "  padding: 14px;"
            "  text-align: left;"
            "}"
            "QPushButton:hover {"
            "  border-color: #3b82f6;"
            "  background-color: #f8fafc;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #eff6ff;"
            "}"
        );

        auto* btnLayout = new QVBoxLayout(cardBtn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(6);

        auto* iconLabel = new QLabel(cardBtn);
        iconLabel->setPixmap(VectorIcons::CreatePixmap(cards[i].icon, 28, QColor(37, 99, 235)));

        auto* titleLabel = new QLabel(QString("<b>%1</b>").arg(cards[i].title), cardBtn);
        titleLabel->setStyleSheet("color: #1e293b; font-size: 13px;");

        auto* descLabel = new QLabel(cards[i].desc, cardBtn);
        descLabel->setStyleSheet("color: #64748b; font-size: 11px;");
        descLabel->setWordWrap(true);

        btnLayout->addWidget(iconLabel);
        btnLayout->addWidget(titleLabel);
        btnLayout->addWidget(descLabel);

        connect(cardBtn, &QPushButton::clicked, this, cards[i].onClick);
        grid->addWidget(cardBtn, r, c);
    }

    return container;
}

QWidget* HomeDashboardWidget::CreateRecentFilesSection() {
    auto* box = new QFrame(this);
    box->setStyleSheet(
        "QFrame {"
        "  background: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "  padding: 12px;"
        "}"
    );

    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    auto* sectionTitle = new QLabel(tr("<b>Recent Documents</b>"), box);
    sectionTitle->setStyleSheet("color: #1e293b; font-size: 14px; border: none;");

    m_searchFilterEdit = new QLineEdit(box);
    m_searchFilterEdit->setPlaceholderText(tr("Search recent files..."));
    m_searchFilterEdit->setStyleSheet(
        "QLineEdit {"
        "  background: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 8px; font-size: 12px;"
        "}"
    );
    connect(m_searchFilterEdit, &QLineEdit::textChanged, this, &HomeDashboardWidget::OnFilterTextChanged);

    auto* clearBtn = new QPushButton(tr("Clear"), box);
    clearBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; border: none; color: #94a3b8; font-size: 11px;"
        "}"
        "QPushButton:hover { color: #ef4444; }"
    );
    connect(clearBtn, &QPushButton::clicked, this, &HomeDashboardWidget::OnClearRecentClicked);

    headerLayout->addWidget(sectionTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_searchFilterEdit);
    headerLayout->addWidget(clearBtn);
    layout->addLayout(headerLayout);

    // Recent Files Table
    m_recentTable = new QTableWidget(box);
    m_recentTable->setColumnCount(3);
    m_recentTable->setHorizontalHeaderLabels({tr("Document Name"), tr("Location"), tr("Action")});
    m_recentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_recentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_recentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_recentTable->verticalHeader()->setVisible(false);
    m_recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTable->setAlternatingRowColors(true);
    m_recentTable->setShowGrid(false);
    m_recentTable->setMinimumHeight(220);
    m_recentTable->setStyleSheet(
        "QTableWidget {"
        "  border: none; background: #ffffff;"
        "}"
        "QHeaderView::section {"
        "  background: #f8fafc; border: none; border-bottom: 1px solid #e2e8f0; color: #64748b; font-weight: 600; padding: 6px;"
        "}"
    );
    connect(m_recentTable, &QTableWidget::cellDoubleClicked, this, &HomeDashboardWidget::OnRecentCellDoubleClicked);

    layout->addWidget(m_recentTable);
    return box;
}

QWidget* HomeDashboardWidget::CreateDropzoneSection() {
    auto* dropBox = new QFrame(this);
    dropBox->setStyleSheet(
        "QFrame {"
        "  background: #f8fafc;"
        "  border: 2px dashed #94a3b8;"
        "  border-radius: 12px;"
        "  padding: 24px;"
        "}"
    );

    auto* layout = new QVBoxLayout(dropBox);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    auto* iconLabel = new QLabel(dropBox);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(VectorIcons::CreatePixmap(IconType::Open, 48, QColor(148, 163, 184)));

    auto* promptLabel = new QLabel(tr("<b>Drop PDF Files Here</b><br>or click Browse to open"), dropBox);
    promptLabel->setAlignment(Qt::AlignCenter);
    promptLabel->setStyleSheet("color: #475569; font-size: 13px; border: none;");

    auto* browseBtn = new QPushButton(tr("Browse Files..."), dropBox);
    browseBtn->setStyleSheet(
        "QPushButton {"
        "  background: #2563eb; color: #ffffff; border: none; border-radius: 6px; padding: 8px 18px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #1d4ed8; }"
    );
    connect(browseBtn, &QPushButton::clicked, this, &HomeDashboardWidget::OpenFileRequested);

    layout->addStretch();
    layout->addWidget(iconLabel);
    layout->addWidget(promptLabel);
    layout->addWidget(browseBtn);
    layout->addStretch();

    return dropBox;
}

void HomeDashboardWidget::RefreshRecentFiles() {
    auto recentFiles = persistence::RecentFilesManager::Instance().GetRecentFiles();
    m_recentTable->setRowCount(0);

    for (const auto& path : recentFiles) {
        QFileInfo fi(QString::fromStdString(path.string()));
        if (!fi.exists()) continue;

        int row = m_recentTable->rowCount();
        m_recentTable->insertRow(row);

        auto* nameItem = new QTableWidgetItem(fi.fileName());
        nameItem->setIcon(VectorIcons::GetIcon(IconType::Properties));
        nameItem->setToolTip(fi.filePath());

        auto* pathItem = new QTableWidgetItem(fi.absolutePath());
        pathItem->setToolTip(fi.filePath());
        pathItem->setForeground(QColor(100, 116, 139));

        m_recentTable->setItem(row, 0, nameItem);
        m_recentTable->setItem(row, 1, pathItem);

        auto* openBtn = new QPushButton(tr("Open"), m_recentTable);
        openBtn->setStyleSheet(
            "QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 4px; padding: 2px 8px; font-size: 11px; }"
            "QPushButton:hover { background: #e2e8f0; }"
        );
        connect(openBtn, &QPushButton::clicked, this, [this, fp = fi.filePath()]() {
            emit FileSelected(fp);
        });
        m_recentTable->setCellWidget(row, 2, openBtn);
    }
}

void HomeDashboardWidget::OnFilterTextChanged(const QString& text) {
    for (int r = 0; r < m_recentTable->rowCount(); ++r) {
        auto* nameItem = m_recentTable->item(r, 0);
        auto* pathItem = m_recentTable->item(r, 1);
        bool match = false;
        if (nameItem && nameItem->text().contains(text, Qt::CaseInsensitive)) match = true;
        if (pathItem && pathItem->text().contains(text, Qt::CaseInsensitive)) match = true;
        m_recentTable->setRowHidden(r, !match);
    }
}

void HomeDashboardWidget::OnRecentCellDoubleClicked(int row, int /*column*/) {
    auto* nameItem = m_recentTable->item(row, 0);
    if (nameItem) {
        QString fp = nameItem->toolTip();
        emit FileSelected(fp);
    }
}

void HomeDashboardWidget::OnClearRecentClicked() {
    persistence::RecentFilesManager::Instance().ClearRecentFiles();
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
