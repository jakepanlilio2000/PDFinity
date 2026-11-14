// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpConfigDialog.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "mcp/McpServer.hpp"
#include "application/AppContext.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>

namespace pdfinity::ui {

McpConfigDialog::McpConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Model Context Protocol (MCP) Manager"));
    resize(860, 640);
    setModal(true);

    setupUi();
    loadFromPolicy();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &McpConfigDialog::onRefreshTimer);
    m_refreshTimer->start(1500); // 1.5s refresh for active session and logs
}

void McpConfigDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // 1. Security Advisory Banner
    auto* banner = new QFrame(this);
    banner->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px; padding: 12px;");
    auto* bannerLayout = new QVBoxLayout(banner);
    auto* bannerTitle = new QLabel(tr("<b>Local Model Context Protocol (MCP) Integration</b>"), this);
    bannerTitle->setStyleSheet("color: #0F172A; font-size: 14px;");
    auto* bannerDesc = new QLabel(
        tr("Provides local AI assistants (Claude Code, Cursor, etc.) with strictly sandboxed, read/write PDF tools over standard I/O (stdio).\n"
           "• <b>Security Controls:</b> Off by default. Network access is disabled. File access is strictly constrained to approved roots.\n"
           "• <b>Source Immutability:</b> Source files cannot be overwritten. Destructive operations require explicit operator consent."),
        this
    );
    bannerDesc->setStyleSheet("color: #475569; font-size: 12px;");
    bannerDesc->setWordWrap(true);
    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(bannerDesc);
    mainLayout->addWidget(banner);

    // 2. Core Settings Group
    auto* cfgGroup = new QGroupBox(tr("General MCP Policy & Consent"), this);
    auto* cfgLayout = new QGridLayout(cfgGroup);

    m_enableCheck = new QCheckBox(tr("Enable Model Context Protocol (MCP) Integration"), this);
    m_enableCheck->setStyleSheet("font-weight: bold; color: #1E293B;");
    connect(m_enableCheck, &QCheckBox::toggled, this, &McpConfigDialog::onEnableToggled);
    cfgLayout->addWidget(m_enableCheck, 0, 0, 1, 2);

    cfgLayout->addWidget(new QLabel(tr("Default Consent Policy:"), this), 1, 0);
    m_consentCombo = new QComboBox(this);
    m_consentCombo->addItem(tr("Ask For Destructive (Permit read/write in approved roots, request confirmation for destructive)"), static_cast<int>(mcp::McpConsentPolicy::AskForDestructive));
    m_consentCombo->addItem(tr("Trust Approved Roots Only (Permit read/write in approved roots, disallow destructive)"), static_cast<int>(mcp::McpConsentPolicy::TrustApprovedRootsOnly));
    m_consentCombo->addItem(tr("Always Ask (Require interactive confirmation for every tool execution)"), static_cast<int>(mcp::McpConsentPolicy::AlwaysAsk));
    m_consentCombo->addItem(tr("Deny All (Block all tool execution requests unconditionally)"), static_cast<int>(mcp::McpConsentPolicy::DenyAll));
    connect(m_consentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &McpConfigDialog::onConsentPolicyChanged);
    cfgLayout->addWidget(m_consentCombo, 1, 1);

    m_allowDestructiveCheck = new QCheckBox(tr("Permit Destructive Operations (Form flattening, metadata and content sanitization)"), this);
    connect(m_allowDestructiveCheck, &QCheckBox::toggled, this, &McpConfigDialog::onAllowDestructiveToggled);
    cfgLayout->addWidget(m_allowDestructiveCheck, 2, 0, 1, 2);

    mainLayout->addWidget(cfgGroup);

    // 3. Approved Directory Roots Group
    auto* rootsGroup = new QGroupBox(tr("Approved Directory Roots (Sandbox Boundary)"), this);
    auto* rootsLayout = new QHBoxLayout(rootsGroup);

    m_rootsList = new QListWidget(this);
    rootsLayout->addWidget(m_rootsList, 1);

    auto* rootsBtnLayout = new QVBoxLayout();
    m_addRootBtn = new QPushButton(tr("Add Folder..."), this);
    connect(m_addRootBtn, &QPushButton::clicked, this, &McpConfigDialog::onAddApprovedRoot);
    rootsBtnLayout->addWidget(m_addRootBtn);

    m_removeRootBtn = new QPushButton(tr("Remove Selected"), this);
    connect(m_removeRootBtn, &QPushButton::clicked, this, &McpConfigDialog::onRemoveApprovedRoot);
    rootsBtnLayout->addWidget(m_removeRootBtn);

    m_clearRootsBtn = new QPushButton(tr("Clear All"), this);
    connect(m_clearRootsBtn, &QPushButton::clicked, this, &McpConfigDialog::onClearApprovedRoots);
    rootsBtnLayout->addWidget(m_clearRootsBtn);

    rootsBtnLayout->addStretch();
    rootsLayout->addLayout(rootsBtnLayout);
    mainLayout->addWidget(rootsGroup);

    // 4. Active Session & Revoke Bar
    auto* sessionBar = new QFrame(this);
    sessionBar->setStyleSheet("background-color: #F1F5F9; border-radius: 4px; padding: 6px 12px;");
    auto* sessionLayout = new QHBoxLayout(sessionBar);

    m_sessionStatusLabel = new QLabel(tr("Session Status: Disconnected"), this);
    m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #475569;");
    sessionLayout->addWidget(m_sessionStatusLabel, 1);

    m_disconnectBtn = new QPushButton(tr("Revoke / Disconnect Session"), this);
    m_disconnectBtn->setStyleSheet("background-color: #FEE2E2; color: #991B1B; font-weight: bold;");
    connect(m_disconnectBtn, &QPushButton::clicked, this, &McpConfigDialog::onDisconnectSession);
    sessionLayout->addWidget(m_disconnectBtn);
    mainLayout->addWidget(sessionBar);

    // 5. Audit Log Table
    auto* logsGroup = new QGroupBox(tr("Recent MCP Audit Logs"), this);
    auto* logsLayout = new QVBoxLayout(logsGroup);

    m_auditTable = new QTableWidget(this);
    m_auditTable->setColumnCount(5);
    m_auditTable->setHorizontalHeaderLabels({tr("Timestamp"), tr("Client"), tr("Method / Tool"), tr("Status"), tr("Details")});
    m_auditTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_auditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_auditTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logsLayout->addWidget(m_auditTable);

    auto* logBtnLayout = new QHBoxLayout();
    m_clearLogsBtn = new QPushButton(tr("Clear Logs"), this);
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &McpConfigDialog::onClearAuditLogs);
    logBtnLayout->addWidget(m_clearLogsBtn);
    logBtnLayout->addStretch();
    logsLayout->addLayout(logBtnLayout);

    mainLayout->addWidget(logsGroup);

    // 6. Bottom Buttons
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* saveCloseBtn = new QPushButton(tr("Save & Close"), this);
    saveCloseBtn->setStyleSheet("background-color: #2563EB; color: #FFFFFF; font-weight: bold; padding: 6px 16px;");
    connect(saveCloseBtn, &QPushButton::clicked, this, &McpConfigDialog::onSaveAndClose);
    bottomLayout->addWidget(saveCloseBtn);

    mainLayout->addLayout(bottomLayout);
}

void McpConfigDialog::loadFromPolicy() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    m_enableCheck->setChecked(sec.IsEnabled());

    int policyIdx = m_consentCombo->findData(static_cast<int>(sec.GetConsentPolicy()));
    if (policyIdx >= 0) {
        m_consentCombo->setCurrentIndex(policyIdx);
    }

    m_allowDestructiveCheck->setChecked(sec.IsAllowDestructive());

    m_rootsList->clear();
    for (const auto& r : sec.GetApprovedRoots()) {
        m_rootsList->addItem(QString::fromStdString(r.string()));
    }

    refreshSessionState();
    refreshAuditLogsTable();
}

void McpConfigDialog::refreshSessionState() {
    auto state = mcp::McpServer::Instance().GetSessionState();
    if (state.isConnected) {
        m_sessionStatusLabel->setText(tr("Session: Connected (%1 v%2) | Protocol: %3 | Handled: %4 | Tools: %5 | Violations: %6")
            .arg(QString::fromStdString(state.clientName))
            .arg(QString::fromStdString(state.clientVersion))
            .arg(QString::fromStdString(state.negotiatedProtocolVersion))
            .arg(state.requestsHandled)
            .arg(state.toolsExecuted)
            .arg(state.securityViolations));
        m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #166534;");
        m_disconnectBtn->setEnabled(true);
    } else {
        m_sessionStatusLabel->setText(tr("Session Status: Disconnected (No active stdio or local client)"));
        m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #64748B;");
        m_disconnectBtn->setEnabled(false);
    }
}

void McpConfigDialog::refreshAuditLogsTable() {
    auto logs = mcp::McpSecurityPolicy::Instance().GetAuditLogs();
    m_auditTable->setRowCount(static_cast<int>(logs.size()));

    for (int i = 0; i < static_cast<int>(logs.size()); ++i) {
        // Show newest on top
        int srcIdx = static_cast<int>(logs.size()) - 1 - i;
        const auto& l = logs[srcIdx];

        m_auditTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(l.timestamp)));
        m_auditTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(l.client)));
        m_auditTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(l.method + (l.toolName != "-" ? (" / " + l.toolName) : ""))));

        auto* statusItem = new QTableWidgetItem(QString::fromStdString(l.status));
        if (l.status == "SUCCESS") {
            statusItem->setBackground(QColor(220, 252, 231));
            statusItem->setForeground(QColor(22, 101, 52));
        } else if (l.status == "DENIED" || l.status == "BLOCKED") {
            statusItem->setBackground(QColor(254, 226, 226));
            statusItem->setForeground(QColor(153, 27, 27));
        } else {
            statusItem->setBackground(QColor(254, 243, 199));
            statusItem->setForeground(QColor(146, 64, 14));
        }
        m_auditTable->setItem(i, 3, statusItem);
        m_auditTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(l.details)));
    }
}

void McpConfigDialog::onEnableToggled(bool checked) {
    mcp::McpSecurityPolicy::Instance().SetEnabled(checked);
}

void McpConfigDialog::onConsentPolicyChanged(int index) {
    auto policy = static_cast<mcp::McpConsentPolicy>(m_consentCombo->itemData(index).toInt());
    mcp::McpSecurityPolicy::Instance().SetConsentPolicy(policy);
}

void McpConfigDialog::onAllowDestructiveToggled(bool checked) {
    mcp::McpSecurityPolicy::Instance().SetAllowDestructive(checked);
}

void McpConfigDialog::onAddApprovedRoot() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Approved Root Directory for MCP"));
    if (dir.isEmpty()) return;

    mcp::McpSecurityPolicy::Instance().AddApprovedRoot(dir.toStdString());
    loadFromPolicy();
}

void McpConfigDialog::onRemoveApprovedRoot() {
    int row = m_rootsList->currentRow();
    if (row < 0) return;

    QString pathStr = m_rootsList->item(row)->text();
    mcp::McpSecurityPolicy::Instance().RemoveApprovedRoot(pathStr.toStdString());
    loadFromPolicy();
}

void McpConfigDialog::onClearApprovedRoots() {
    int ans = QMessageBox::question(
        this, tr("Clear Approved Roots"),
        tr("Clear all approved roots? Local AI tools will have zero filesystem access until new roots are approved."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (ans != QMessageBox::Yes) return;

    mcp::McpSecurityPolicy::Instance().ClearApprovedRoots();
    loadFromPolicy();
}

void McpConfigDialog::onDisconnectSession() {
    mcp::McpServer::Instance().Disconnect();
    refreshSessionState();
    refreshAuditLogsTable();
}

void McpConfigDialog::onClearAuditLogs() {
    mcp::McpSecurityPolicy::Instance().ClearAuditLogs();
    refreshAuditLogsTable();
}

void McpConfigDialog::onRefreshTimer() {
    refreshSessionState();
    refreshAuditLogsTable();
}

void McpConfigDialog::onSaveAndClose() {
    // Persist to AppConfig in AppContext
    auto& ctx = app::AppContext::Instance();
    auto& cfg = ctx.GetConfig();
    auto sec = mcp::McpSecurityPolicy::Instance().GetConfig();

    cfg.mcpEnabled = sec.enabled;
    cfg.mcpConsentPolicy = mcp::ConsentPolicyToString(sec.consentPolicy);
    cfg.mcpAllowDestructiveOperations = sec.allowDestructive;
    cfg.mcpApprovedRoots.clear();
    for (const auto& r : sec.approvedRoots) {
        cfg.mcpApprovedRoots.push_back(r.string());
    }

    ctx.SaveCurrentState();
    accept();
}

} // namespace pdfinity::ui
