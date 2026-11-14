// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

namespace pdfinity::ui {

class DocumentSecurityDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocumentSecurityDialog(QWidget* parent = nullptr);
    ~DocumentSecurityDialog() override = default;

    [[nodiscard]] pdf_api::DocumentEncryptionConfig GetConfig() const;

private slots:
    void OnAccept();

private:
    void SetupUi();

    QCheckBox* m_requireOpenPassCheck{nullptr};
    QLineEdit* m_userPassEdit{nullptr};

    QCheckBox* m_restrictPermsCheck{nullptr};
    QLineEdit* m_ownerPassEdit{nullptr};

    QCheckBox* m_allowPrintCheck{nullptr};
    QCheckBox* m_allowCopyCheck{nullptr};
    QCheckBox* m_allowModifyCheck{nullptr};
    QCheckBox* m_allowAnnotCheck{nullptr};
    QCheckBox* m_allowFormsCheck{nullptr};

    QComboBox* m_algoCombo{nullptr};

    pdf_api::DocumentEncryptionConfig m_config;
};

} // namespace pdfinity::ui
