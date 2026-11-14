// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

namespace pdfinity::ui {

class McpConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit McpConfigDialog(QWidget* parent = nullptr);
    ~McpConfigDialog() override = default;

private slots:
    void onEnableToggled(bool checked);
    void onConsentPolicyChanged(int index);
    void onAllowDestructiveToggled(bool checked);
    void onAddApprovedRoot();
    void onRemoveApprovedRoot();
    void onClearApprovedRoots();
    void onDisconnectSession();
    void onClearAuditLogs();
    void onRefreshTimer();
    void onSaveAndClose();

private:
    void setupUi();
    void loadFromPolicy();
    void refreshSessionState();
    void refreshAuditLogsTable();

    // Controls
    QCheckBox* m_enableCheck{nullptr};
    QComboBox* m_consentCombo{nullptr};
    QCheckBox* m_allowDestructiveCheck{nullptr};

    QListWidget* m_rootsList{nullptr};
    QPushButton* m_addRootBtn{nullptr};
    QPushButton* m_removeRootBtn{nullptr};
    QPushButton* m_clearRootsBtn{nullptr};

    QLabel* m_sessionStatusLabel{nullptr};
    QPushButton* m_disconnectBtn{nullptr};

    QTableWidget* m_auditTable{nullptr};
    QPushButton* m_clearLogsBtn{nullptr};

    QTimer* m_refreshTimer{nullptr};
};

} // namespace pdfinity::ui
