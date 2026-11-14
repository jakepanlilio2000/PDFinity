// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteConfigDialog.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemotePairingManager.hpp"
#include "remote/RemoteControllerServer.hpp"
#include "application/AppContext.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QFont>

namespace pdfinity::ui {

RemoteConfigDialog::RemoteConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Remote Controller & Pairing Manager"));
    resize(880, 680);
    setModal(true);

    setupUi();
    loadFromPolicy();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &RemoteConfigDialog::onRefreshTimer);
    m_refreshTimer->start(1000); // 1s refresh for pairing countdown & session
}

void RemoteConfigDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // 1. Security Advisory Banner
    auto* banner = new QFrame(this);
    banner->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px; padding: 12px;");
    auto* bannerLayout = new QVBoxLayout(banner);
    auto* bannerTitle = new QLabel(tr("<b>Secure PDF Remote Controller</b>"), this);
    bannerTitle->setStyleSheet("color: #0F172A; font-size: 14px;");
    auto* bannerDesc = new QLabel(
        tr("Provides trusted companion devices (tablets, presentation clickers, workflow runners) with authenticated navigation and status monitoring.\n"
           "• <b>Security Guarantees:</b> Disabled by default. Zero public internet exposure. Bounded to local interface.\n"
           "• <b>Pairing Protection:</b> Short-lived 6-digit PIN with rate-limiting and replay defense. Host confirmation required for sensitive actions."),
        this
    );
    bannerDesc->setStyleSheet("color: #475569; font-size: 12px;");
    bannerDesc->setWordWrap(true);
    bannerLayout->addWidget(bannerTitle);
    bannerLayout->addWidget(bannerDesc);
    mainLayout->addWidget(banner);

    // 2. Core Service Configuration
    auto* cfgGroup = new QGroupBox(tr("Remote Service Configuration"), this);
    auto* cfgLayout = new QGridLayout(cfgGroup);

    m_enableCheck = new QCheckBox(tr("Enable Remote Controller Companion Service"), this);
    m_enableCheck->setObjectName("enableCheck");
    m_enableCheck->setStyleSheet("font-weight: bold; color: #1E293B;");
    connect(m_enableCheck, &QCheckBox::toggled, this, &RemoteConfigDialog::onEnableToggled);
    cfgLayout->addWidget(m_enableCheck, 0, 0, 1, 3);

    cfgLayout->addWidget(new QLabel(tr("Listen Interface:"), this), 1, 0);
    m_bindAddressCombo = new QComboBox(this);
    populateNetworkInterfaces();
    connect(m_bindAddressCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RemoteConfigDialog::onBindAddressChanged);
    cfgLayout->addWidget(m_bindAddressCombo, 1, 1);

    cfgLayout->addWidget(new QLabel(tr("Port:"), this), 1, 2);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(remote::DEFAULT_REMOTE_PORT);
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RemoteConfigDialog::onPortChanged);
    cfgLayout->addWidget(m_portSpin, 1, 3);

    m_lanWarningLabel = new QLabel(tr("<b>Notice:</b> Binding to a LAN interface permits local network devices to connect. Ensure your Wi-Fi/LAN is trusted."), this);
    m_lanWarningLabel->setStyleSheet("color: #B45309; font-size: 11px;");
    m_lanWarningLabel->setVisible(false);
    cfgLayout->addWidget(m_lanWarningLabel, 2, 0, 1, 4);

    m_confirmCheck = new QCheckBox(tr("Require Host Desktop Confirmation for Sensitive Actions (Document close, batch starts)"), this);
    m_confirmCheck->setChecked(true);
    connect(m_confirmCheck, &QCheckBox::toggled, this, &RemoteConfigDialog::onRequireConfirmationToggled);
    cfgLayout->addWidget(m_confirmCheck, 3, 0, 1, 4);

    mainLayout->addWidget(cfgGroup);

    // 3. One-Time PIN Pairing Card
    auto* pairGroup = new QGroupBox(tr("Device Pairing (Mutual Confirmation)"), this);
    auto* pairLayout = new QHBoxLayout(pairGroup);

    auto* pinBox = new QFrame(this);
    pinBox->setStyleSheet("background-color: #FFFFFF; border: 2px dashed #CBD5E1; border-radius: 8px; padding: 12px;");
    auto* pinBoxLayout = new QVBoxLayout(pinBox);
    pinBoxLayout->setAlignment(Qt::AlignCenter);

    m_pinDisplayLabel = new QLabel(tr("------"), this);
    QFont pinFont("Courier New", 28, QFont::Bold);
    m_pinDisplayLabel->setFont(pinFont);
    m_pinDisplayLabel->setStyleSheet("color: #1E293B; letter-spacing: 4px;");
    m_pinDisplayLabel->setAlignment(Qt::AlignCenter);
    pinBoxLayout->addWidget(m_pinDisplayLabel);

    m_pinTimerLabel = new QLabel(tr("No active pairing session"), this);
    m_pinTimerLabel->setStyleSheet("color: #64748B; font-size: 12px;");
    m_pinTimerLabel->setAlignment(Qt::AlignCenter);
    pinBoxLayout->addWidget(m_pinTimerLabel);

    m_pinProgressBar = new QProgressBar(this);
    m_pinProgressBar->setRange(0, 60);
    m_pinProgressBar->setValue(0);
    m_pinProgressBar->setTextVisible(false);
    m_pinProgressBar->setFixedHeight(4);
    pinBoxLayout->addWidget(m_pinProgressBar);

    pairLayout->addWidget(pinBox, 1);

    auto* pairBtnLayout = new QVBoxLayout();
    m_startPairBtn = new QPushButton(tr("Generate Pairing PIN (60s)..."), this);
    m_startPairBtn->setObjectName("startPairBtn");
    m_startPairBtn->setStyleSheet("background-color: #0EA5E9; color: #FFFFFF; font-weight: bold; padding: 8px 16px;");
    connect(m_startPairBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onStartPairingClicked);
    pairBtnLayout->addWidget(m_startPairBtn);

    m_cancelPairBtn = new QPushButton(tr("Cancel Pairing"), this);
    m_cancelPairBtn->setObjectName("cancelPairBtn");
    connect(m_cancelPairBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onCancelPairingClicked);
    m_cancelPairBtn->setEnabled(false);
    pairBtnLayout->addWidget(m_cancelPairBtn);

    pairBtnLayout->addStretch();
    pairLayout->addLayout(pairBtnLayout);
    mainLayout->addWidget(pairGroup);

    // 4. Paired Devices Table
    auto* devGroup = new QGroupBox(tr("Authorized / Paired Devices"), this);
    auto* devLayout = new QHBoxLayout(devGroup);

    m_devicesTable = new QTableWidget(this);
    m_devicesTable->setColumnCount(5);
    m_devicesTable->setHorizontalHeaderLabels({tr("Device ID"), tr("Friendly Name"), tr("Paired At"), tr("Last Seen IP"), tr("Status")});
    m_devicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_devicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_devicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_devicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_devicesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_devicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_devicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    devLayout->addWidget(m_devicesTable, 1);

    auto* devBtnLayout = new QVBoxLayout();
    m_revokeDeviceBtn = new QPushButton(tr("Revoke Selected"), this);
    m_revokeDeviceBtn->setStyleSheet("background-color: #FEE2E2; color: #991B1B; font-weight: bold;");
    connect(m_revokeDeviceBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onRevokeSelectedDevice);
    devBtnLayout->addWidget(m_revokeDeviceBtn);

    m_clearDevicesBtn = new QPushButton(tr("Revoke All"), this);
    connect(m_clearDevicesBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onClearAllDevices);
    devBtnLayout->addWidget(m_clearDevicesBtn);

    devBtnLayout->addStretch();
    devLayout->addLayout(devBtnLayout);
    mainLayout->addWidget(devGroup);

    // 5. Active Session & Emergency Disable Bar
    auto* sessionBar = new QFrame(this);
    sessionBar->setStyleSheet("background-color: #F1F5F9; border-radius: 4px; padding: 6px 12px;");
    auto* sessionLayout = new QHBoxLayout(sessionBar);

    m_sessionStatusLabel = new QLabel(tr("Session: Disconnected (No active remote clients)"), this);
    m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #475569;");
    sessionLayout->addWidget(m_sessionStatusLabel, 1);

    m_disconnectBtn = new QPushButton(tr("Disconnect Active Session"), this);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onDisconnectSession);
    m_disconnectBtn->setEnabled(false);
    sessionLayout->addWidget(m_disconnectBtn);

    m_emergencyBtn = new QPushButton(tr("Emergency Disable Service"), this);
    m_emergencyBtn->setStyleSheet("background-color: #DC2626; color: #FFFFFF; font-weight: bold;");
    connect(m_emergencyBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onEmergencyDisable);
    sessionLayout->addWidget(m_emergencyBtn);

    mainLayout->addWidget(sessionBar);

    // 6. Audit Log Table
    auto* logGroup = new QGroupBox(tr("Remote Operations Audit Log"), this);
    auto* logLayout = new QVBoxLayout(logGroup);

    m_auditTable = new QTableWidget(this);
    m_auditTable->setColumnCount(5);
    m_auditTable->setHorizontalHeaderLabels({tr("Timestamp"), tr("Client / IP"), tr("Command"), tr("Status"), tr("Details")});
    m_auditTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_auditTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_auditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_auditTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    logLayout->addWidget(m_auditTable);

    auto* logBtnLayout = new QHBoxLayout();
    m_clearLogsBtn = new QPushButton(tr("Clear Logs"), this);
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onClearAuditLogs);
    logBtnLayout->addWidget(m_clearLogsBtn);
    logBtnLayout->addStretch();
    logLayout->addLayout(logBtnLayout);

    mainLayout->addWidget(logGroup);

    // 7. Bottom Action Bar
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* saveCloseBtn = new QPushButton(tr("Save & Close"), this);
    saveCloseBtn->setStyleSheet("background-color: #2563EB; color: #FFFFFF; font-weight: bold; padding: 6px 18px;");
    connect(saveCloseBtn, &QPushButton::clicked, this, &RemoteConfigDialog::onSaveAndClose);
    bottomLayout->addWidget(saveCloseBtn);

    mainLayout->addLayout(bottomLayout);
}

void RemoteConfigDialog::populateNetworkInterfaces() {
    m_bindAddressCombo->clear();
    m_bindAddressCombo->addItem(tr("127.0.0.1 (Loopback only - Highly Recommended)"), "127.0.0.1");

    const auto allAddresses = QNetworkInterface::allAddresses();
    for (const auto& addr : allAddresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            QString ipStr = addr.toString();
            m_bindAddressCombo->addItem(tr("%1 (Local LAN)").arg(ipStr), ipStr);
        }
    }
}

void RemoteConfigDialog::loadFromPolicy() {
    auto& sec = remote::RemoteSecurityPolicy::Instance();
    m_enableCheck->setChecked(sec.IsEnabled());

    int addrIdx = m_bindAddressCombo->findData(QString::fromStdString(sec.GetBindAddress()));
    if (addrIdx >= 0) {
        m_bindAddressCombo->setCurrentIndex(addrIdx);
    }

    m_portSpin->setValue(sec.GetServerPort());
    m_confirmCheck->setChecked(sec.GetConfig().requireHostConfirmation);

    refreshSessionState();
    refreshPairedDevicesTable();
    refreshAuditLogsTable();
}

void RemoteConfigDialog::refreshSessionState() {
    auto state = remote::RemoteControllerServer::Instance().GetSessionState();
    if (state.isConnected) {
        m_sessionStatusLabel->setText(tr("Session: Connected to %1 (%2) | Handled: %3 | Commands: %4 | Violations: %5")
            .arg(QString::fromStdString(state.activeClientName))
            .arg(QString::fromStdString(state.activeIp))
            .arg(state.requestsHandled)
            .arg(state.commandsExecuted)
            .arg(state.violationsCount));
        m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #166534;");
        m_disconnectBtn->setEnabled(true);
    } else {
        bool listening = remote::RemoteControllerServer::Instance().IsListening();
        m_sessionStatusLabel->setText(listening
            ? tr("Service: Listening on %1:%2 (No client connected)")
                .arg(QString::fromStdString(remote::RemoteControllerServer::Instance().GetBoundAddress()))
                .arg(remote::RemoteControllerServer::Instance().GetBoundPort())
            : tr("Service: Inactive / Stopped (Off by default)"));
        m_sessionStatusLabel->setStyleSheet("font-weight: 600; color: #64748B;");
        m_disconnectBtn->setEnabled(false);
    }
}

void RemoteConfigDialog::refreshPairedDevicesTable() {
    auto devices = remote::RemotePairingManager::Instance().GetPairedDevices();
    m_devicesTable->setRowCount(static_cast<int>(devices.size()));

    for (int i = 0; i < static_cast<int>(devices.size()); ++i) {
        const auto& d = devices[i];
        m_devicesTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(d.deviceId)));
        m_devicesTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(d.clientName)));
        m_devicesTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(d.pairedAtIso)));
        m_devicesTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(d.lastSeenIp)));

        auto* statusItem = new QTableWidgetItem(d.isRevoked ? tr("Revoked") : tr("Active"));
        if (d.isRevoked) {
            statusItem->setBackground(QColor(254, 226, 226));
            statusItem->setForeground(QColor(153, 27, 27));
        } else {
            statusItem->setBackground(QColor(220, 252, 231));
            statusItem->setForeground(QColor(22, 101, 52));
        }
        m_devicesTable->setItem(i, 4, statusItem);
    }
}

void RemoteConfigDialog::refreshAuditLogsTable() {
    auto logs = remote::RemoteSecurityPolicy::Instance().GetAuditLogs();
    m_auditTable->setRowCount(static_cast<int>(logs.size()));

    for (int i = 0; i < static_cast<int>(logs.size()); ++i) {
        int srcIdx = static_cast<int>(logs.size()) - 1 - i;
        const auto& l = logs[srcIdx];

        m_auditTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(l.timestamp)));
        m_auditTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(l.clientName + " (" + l.ip + ")")));
        m_auditTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(l.command)));

        auto* statusItem = new QTableWidgetItem(QString::fromStdString(l.status));
        if (l.status == "ALLOWED" || l.status == "CONFIRMED") {
            statusItem->setBackground(QColor(220, 252, 231));
            statusItem->setForeground(QColor(22, 101, 52));
        } else if (l.status == "DENIED" || l.status == "BLOCKED" || l.status == "RATE_LIMITED") {
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

void RemoteConfigDialog::onEnableToggled(bool checked) {
    remote::RemoteSecurityPolicy::Instance().SetEnabled(checked);
    if (checked) {
        remote::RemoteControllerServer::Instance().Start();
    } else {
        remote::RemoteControllerServer::Instance().Stop();
    }
    refreshSessionState();
}

void RemoteConfigDialog::onBindAddressChanged(int index) {
    QString addr = m_bindAddressCombo->itemData(index).toString();
    remote::RemoteSecurityPolicy::Instance().SetBindAddress(addr.toStdString());
    m_lanWarningLabel->setVisible(addr != "127.0.0.1");

    if (remote::RemoteSecurityPolicy::Instance().IsEnabled()) {
        remote::RemoteControllerServer::Instance().Start();
    }
    refreshSessionState();
}

void RemoteConfigDialog::onPortChanged(int port) {
    remote::RemoteSecurityPolicy::Instance().SetServerPort(static_cast<uint16_t>(port));
    if (remote::RemoteSecurityPolicy::Instance().IsEnabled()) {
        remote::RemoteControllerServer::Instance().Start();
    }
    refreshSessionState();
}

void RemoteConfigDialog::onRequireConfirmationToggled(bool checked) {
    auto cfg = remote::RemoteSecurityPolicy::Instance().GetConfig();
    cfg.requireHostConfirmation = checked;
    remote::RemoteSecurityPolicy::Instance().SetConfig(cfg);
}

void RemoteConfigDialog::onStartPairingClicked() {
    std::string pin = remote::RemotePairingManager::Instance().GenerateNewPin(60);
    // Format pin as XXX XXX
    QString pinFormatted = QString::fromStdString(pin);
    if (pinFormatted.length() == 6) {
        pinFormatted.insert(3, " ");
    }

    m_pinDisplayLabel->setText(pinFormatted);
    m_pinTimerLabel->setText(tr("PIN active: 60s remaining. Enter this PIN in your remote controller app."));
    m_pinProgressBar->setValue(60);
    m_startPairBtn->setEnabled(false);
    m_cancelPairBtn->setEnabled(true);
}

void RemoteConfigDialog::onCancelPairingClicked() {
    remote::RemotePairingManager::Instance().CancelPairing();
    m_pinDisplayLabel->setText(tr("------"));
    m_pinTimerLabel->setText(tr("Pairing cancelled"));
    m_pinProgressBar->setValue(0);
    m_startPairBtn->setEnabled(true);
    m_cancelPairBtn->setEnabled(false);
}

void RemoteConfigDialog::onRevokeSelectedDevice() {
    int row = m_devicesTable->currentRow();
    if (row < 0) return;

    QString devId = m_devicesTable->item(row, 0)->text();
    remote::RemoteControllerServer::Instance().RevokeDevice(devId.toStdString());
    refreshPairedDevicesTable();
    refreshSessionState();
}

void RemoteConfigDialog::onClearAllDevices() {
    int ans = QMessageBox::question(
        this, tr("Revoke All Devices"),
        tr("Revoke all paired remote devices? All existing paired devices will be barred from communicating with this host."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (ans != QMessageBox::Yes) return;

    remote::RemotePairingManager::Instance().ClearAllDevices();
    remote::RemoteControllerServer::Instance().DisconnectAll();
    refreshPairedDevicesTable();
    refreshSessionState();
}

void RemoteConfigDialog::onDisconnectSession() {
    remote::RemoteControllerServer::Instance().DisconnectAll();
    refreshSessionState();
}

void RemoteConfigDialog::onEmergencyDisable() {
    int ans = QMessageBox::critical(
        this, tr("Emergency Remote Disable"),
        tr("Immediately unbind remote listener, disconnect all active sockets, and disable the Remote Controller service?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
    );
    if (ans != QMessageBox::Yes) return;

    remote::RemoteControllerServer::Instance().EmergencyDisable();
    m_enableCheck->setChecked(false);
    onCancelPairingClicked();
    refreshSessionState();
    refreshAuditLogsTable();
}

void RemoteConfigDialog::onClearAuditLogs() {
    remote::RemoteSecurityPolicy::Instance().ClearAuditLogs();
    refreshAuditLogsTable();
}

void RemoteConfigDialog::onRefreshTimer() {
    if (remote::RemotePairingManager::Instance().IsPairingActive()) {
        int rem = remote::RemotePairingManager::Instance().GetSecondsRemaining();
        m_pinTimerLabel->setText(tr("PIN active: %1s remaining. Enter this PIN in your remote controller app.").arg(rem));
        m_pinProgressBar->setValue(rem);
    } else if (!m_startPairBtn->isEnabled()) {
        m_pinDisplayLabel->setText(tr("------"));
        m_pinTimerLabel->setText(tr("Pairing PIN expired"));
        m_pinProgressBar->setValue(0);
        m_startPairBtn->setEnabled(true);
        m_cancelPairBtn->setEnabled(false);
    }

    refreshSessionState();
    refreshPairedDevicesTable();
    refreshAuditLogsTable();
}

void RemoteConfigDialog::onSaveAndClose() {
    auto& ctx = app::AppContext::Instance();
    auto& cfg = ctx.GetConfig();
    auto sec = remote::RemoteSecurityPolicy::Instance().GetConfig();

    cfg.remoteEnabled = sec.enabled;
    cfg.remoteBindAddress = sec.bindAddress;
    cfg.remotePort = sec.port;
    cfg.remoteRequireHostConfirmation = sec.requireHostConfirmation;
    cfg.remotePairedDevicesJson = remote::RemotePairingManager::Instance().ExportDevicesJson();

    ctx.SaveCurrentState();
    accept();
}

} // namespace pdfinity::ui
