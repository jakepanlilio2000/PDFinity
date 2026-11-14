// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SettingsDialog.hpp"
#include "ThemeManager.hpp"
#include "application/AppContext.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>

namespace pdfinity::ui {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    SetupUi();
    LoadCurrentSettings();
}

void SettingsDialog::SetupUi() {
    setWindowTitle(tr("Preferences"));
    setFixedSize(460, 380);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 16);
    mainLayout->setSpacing(14);

    // General Group
    auto* generalGroup = new QGroupBox(tr("Appearance & Display"), this);
    auto* generalLayout = new QFormLayout(generalGroup);

    m_themeCombo = new QComboBox(generalGroup);
    m_themeCombo->addItem(tr("Follow Operating System"), "System");
    m_themeCombo->addItem(tr("Light Mode"), "Light");
    m_themeCombo->addItem(tr("Dark Mode"), "Dark");
    generalLayout->addRow(tr("Interface Theme:"), m_themeCombo);

    m_hardwareAccelCheck = new QCheckBox(tr("Enable Hardware-Accelerated Rendering"), generalGroup);
    generalLayout->addRow(tr("Graphics:"), m_hardwareAccelCheck);

    mainLayout->addWidget(generalGroup);

    // Storage & Privacy Group
    auto* storageGroup = new QGroupBox(tr("Storage, Privacy & Autosave"), this);
    auto* storageLayout = new QFormLayout(storageGroup);

    m_maxRecentSpin = new QSpinBox(storageGroup);
    m_maxRecentSpin->setRange(0, 30);
    storageLayout->addRow(tr("Recent Files History Limit:"), m_maxRecentSpin);

    m_autosaveCheck = new QCheckBox(tr("Automatically save recovery state"), storageGroup);
    storageLayout->addRow(tr("Autosave Recovery:"), m_autosaveCheck);

    m_autosaveIntervalSpin = new QSpinBox(storageGroup);
    m_autosaveIntervalSpin->setRange(30, 3600);
    m_autosaveIntervalSpin->setSuffix(tr(" seconds"));
    storageLayout->addRow(tr("Autosave Interval:"), m_autosaveIntervalSpin);

    mainLayout->addWidget(storageGroup);

    // Diagnostics Group
    auto* diagGroup = new QGroupBox(tr("Diagnostics & Redacted Logging"), this);
    auto* diagLayout = new QFormLayout(diagGroup);

    m_logLevelCombo = new QComboBox(diagGroup);
    m_logLevelCombo->addItem("Debug");
    m_logLevelCombo->addItem("Info");
    m_logLevelCombo->addItem("Warning");
    m_logLevelCombo->addItem("Error");
    diagLayout->addRow(tr("Log Verbosity Level:"), m_logLevelCombo);

    mainLayout->addWidget(diagGroup);

    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::OnSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::LoadCurrentSettings() {
    const auto& config = app::AppContext::Instance().GetConfig();

    std::string themeStr = app::AppConfig::ThemeModeToString(config.themeMode);
    int themeIdx = m_themeCombo->findData(QString::fromStdString(themeStr));
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    m_hardwareAccelCheck->setChecked(config.hardwareAcceleration);
    m_maxRecentSpin->setValue(config.maxRecentFiles);
    m_autosaveCheck->setChecked(config.autosaveEnabled);
    m_autosaveIntervalSpin->setValue(config.autosaveIntervalSeconds);

    int logIdx = m_logLevelCombo->findText(QString::fromStdString(config.logLevel));
    if (logIdx >= 0) m_logLevelCombo->setCurrentIndex(logIdx);
}

void SettingsDialog::OnSave() {
    auto& config = app::AppContext::Instance().GetConfig();

    std::string themeData = m_themeCombo->currentData().toString().toStdString();
    config.themeMode = app::AppConfig::StringToThemeMode(themeData);
    config.hardwareAcceleration = m_hardwareAccelCheck->isChecked();
    config.maxRecentFiles = m_maxRecentSpin->value();
    config.autosaveEnabled = m_autosaveCheck->isChecked();
    config.autosaveIntervalSeconds = m_autosaveIntervalSpin->value();
    config.logLevel = m_logLevelCombo->currentText().toStdString();

    app::AppContext::Instance().SaveCurrentState();

    // Reapply theme dynamically
    ThemeManager::ApplyTheme(config.themeMode);

    accept();
}

} // namespace pdfinity::ui
