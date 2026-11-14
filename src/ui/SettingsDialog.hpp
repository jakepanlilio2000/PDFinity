// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "application/AppConfig.hpp"
#include <QDialog>

class QComboBox;
class QSpinBox;
class QCheckBox;

namespace pdfinity::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void OnSave();

private:
    void SetupUi();
    void LoadCurrentSettings();

    QComboBox* m_themeCombo{nullptr};
    QComboBox* m_logLevelCombo{nullptr};
    QCheckBox* m_autosaveCheck{nullptr};
    QSpinBox* m_autosaveIntervalSpin{nullptr};
    QSpinBox* m_maxRecentSpin{nullptr};
    QCheckBox* m_hardwareAccelCheck{nullptr};
};

} // namespace pdfinity::ui
