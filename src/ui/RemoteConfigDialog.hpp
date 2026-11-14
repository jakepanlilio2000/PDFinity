// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QTimer>

namespace pdfinity::ui {

class RemoteConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit RemoteConfigDialog(QWidget* parent = nullptr);
    ~RemoteConfigDialog() override = default;

private slots:
    void onEnableToggled(bool checked);
    void onBindAddressChanged(int index);
    void onPortChanged(int port);
    void onRequireConfirmationToggled(bool checked);
    void onStartPairingClicked();
    void onCancelPairingClicked();
    void onRevokeSelectedDevice();
    void onClearAllDevices();
    void onDisconnectSession();
    void onEmergencyDisable();
    void onClearAuditLogs();
    void onRefreshTimer();
    void onSaveAndClose();

private:
    void setupUi();
    void loadFromPolicy();
    void refreshSessionState();
    void refreshPairedDevicesTable();
    void refreshAuditLogsTable();
    void populateNetworkInterfaces();

    // Controls
    QCheckBox* m_enableCheck{nullptr};
    QComboBox* m_bindAddressCombo{nullptr};
    QLabel* m_lanWarningLabel{nullptr};
    QSpinBox* m_portSpin{nullptr};
    QCheckBox* m_confirmCheck{nullptr};

    // Pairing Box
    QLabel* m_pinDisplayLabel{nullptr};
    QLabel* m_pinTimerLabel{nullptr};
    QProgressBar* m_pinProgressBar{nullptr};
    QPushButton* m_startPairBtn{nullptr};
    QPushButton* m_cancelPairBtn{nullptr};

    // Paired Devices Table
    QTableWidget* m_devicesTable{nullptr};
    QPushButton* m_revokeDeviceBtn{nullptr};
    QPushButton* m_clearDevicesBtn{nullptr};

    // Active Session & Emergency Bar
    QLabel* m_sessionStatusLabel{nullptr};
    QPushButton* m_disconnectBtn{nullptr};
    QPushButton* m_emergencyBtn{nullptr};

    // Audit Log Table
    QTableWidget* m_auditTable{nullptr};
    QPushButton* m_clearLogsBtn{nullptr};

    QTimer* m_refreshTimer{nullptr};
};

} // namespace pdfinity::ui
