// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QPushButton;

namespace pdfinity::ui {

class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget* parent = nullptr);
    ~UpdateDialog() override = default;

private slots:
    void OnCheckOnline();
    void OnOpenDownloadSite();

private:
    void SetupUi();

    QComboBox* m_channelCombo{nullptr};
    QLabel* m_statusLabel{nullptr};
    QPushButton* m_checkButton{nullptr};
    QPushButton* m_downloadButton{nullptr};
};

} // namespace pdfinity::ui
