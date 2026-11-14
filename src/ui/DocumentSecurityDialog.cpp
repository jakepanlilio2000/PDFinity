// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentSecurityDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QLabel>

namespace pdfinity::ui {

DocumentSecurityDialog::DocumentSecurityDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Document Security, Encryption & Permissions"));
    setMinimumWidth(480);
    SetupUi();
}

void DocumentSecurityDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Group 1: Document Open Password
    auto* openGroup = new QGroupBox(tr("Document Open Password"), this);
    auto* openLayout = new QFormLayout(openGroup);
    m_requireOpenPassCheck = new QCheckBox(tr("Require password to open and view the document"), this);
    m_userPassEdit = new QLineEdit(this);
    m_userPassEdit->setEchoMode(QLineEdit::Password);
    m_userPassEdit->setEnabled(false);

    connect(m_requireOpenPassCheck, &QCheckBox::toggled, m_userPassEdit, &QLineEdit::setEnabled);
    openLayout->addRow(m_requireOpenPassCheck);
    openLayout->addRow(tr("Document Open Password:"), m_userPassEdit);
    mainLayout->addWidget(openGroup);

    // Group 2: Permissions Password & Restrictions
    auto* permsGroup = new QGroupBox(tr("Permissions & Modification Restrictions"), this);
    auto* permsLayout = new QVBoxLayout(permsGroup);

    m_restrictPermsCheck = new QCheckBox(tr("Restrict editing and printing of the document"), this);
    m_ownerPassEdit = new QLineEdit(this);
    m_ownerPassEdit->setEchoMode(QLineEdit::Password);
    m_ownerPassEdit->setEnabled(false);

    auto* passRow = new QHBoxLayout();
    passRow->addWidget(new QLabel(tr("Permissions Password:")));
    passRow->addWidget(m_ownerPassEdit);

    connect(m_restrictPermsCheck, &QCheckBox::toggled, [this](bool checked) {
        m_ownerPassEdit->setEnabled(checked);
        m_allowPrintCheck->setEnabled(checked);
        m_allowCopyCheck->setEnabled(checked);
        m_allowModifyCheck->setEnabled(checked);
        m_allowAnnotCheck->setEnabled(checked);
        m_allowFormsCheck->setEnabled(checked);
    });

    permsLayout->addWidget(m_restrictPermsCheck);
    permsLayout->addLayout(passRow);

    m_allowPrintCheck = new QCheckBox(tr("Allow high-resolution printing"), this);
    m_allowPrintCheck->setChecked(true);
    m_allowPrintCheck->setEnabled(false);
    permsLayout->addWidget(m_allowPrintCheck);

    m_allowCopyCheck = new QCheckBox(tr("Allow copying text, images, and content"), this);
    m_allowCopyCheck->setChecked(true);
    m_allowCopyCheck->setEnabled(false);
    permsLayout->addWidget(m_allowCopyCheck);

    m_allowModifyCheck = new QCheckBox(tr("Allow modifying document pages and structure"), this);
    m_allowModifyCheck->setChecked(false);
    m_allowModifyCheck->setEnabled(false);
    permsLayout->addWidget(m_allowModifyCheck);

    m_allowAnnotCheck = new QCheckBox(tr("Allow adding comments and annotations"), this);
    m_allowAnnotCheck->setChecked(true);
    m_allowAnnotCheck->setEnabled(false);
    permsLayout->addWidget(m_allowAnnotCheck);

    m_allowFormsCheck = new QCheckBox(tr("Allow filling interactive form fields"), this);
    m_allowFormsCheck->setChecked(true);
    m_allowFormsCheck->setEnabled(false);
    permsLayout->addWidget(m_allowFormsCheck);

    mainLayout->addWidget(permsGroup);

    // Group 3: Encryption Algorithm
    auto* algoGroup = new QGroupBox(tr("Encryption Strength"), this);
    auto* algoLayout = new QHBoxLayout(algoGroup);
    m_algoCombo = new QComboBox(this);
    m_algoCombo->addItem(tr("AES-256 (High Security, Modern ISO 32000-2 / PDF 2.0)"), "AES-256");
    m_algoCombo->addItem(tr("AES-128 (Standard Compatibility, ISO 32000-1 / PDF 1.6+)"), "AES-128");
    algoLayout->addWidget(new QLabel(tr("Algorithm:")));
    algoLayout->addWidget(m_algoCombo, 1);
    mainLayout->addWidget(algoGroup);

    // Dialog Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    auto* applyBtn = new QPushButton(tr("Apply Encryption"), this);
    applyBtn->setDefault(true);
    connect(applyBtn, &QPushButton::clicked, this, &DocumentSecurityDialog::OnAccept);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(applyBtn);
    mainLayout->addLayout(btnLayout);
}

void DocumentSecurityDialog::OnAccept() {
    if (!m_requireOpenPassCheck->isChecked() && !m_restrictPermsCheck->isChecked()) {
        QMessageBox::warning(this, tr("No Security Selected"), tr("Please enable at least an Open Password or Permissions Restrictions."));
        return;
    }

    if (m_requireOpenPassCheck->isChecked() && m_userPassEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Password Required"), tr("Please enter an Open Password."));
        return;
    }

    if (m_restrictPermsCheck->isChecked() && m_ownerPassEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Password Required"), tr("Please enter a Permissions Password."));
        return;
    }

    m_config.userPassword = m_requireOpenPassCheck->isChecked() ? m_userPassEdit->text().toStdString() : "";
    m_config.ownerPassword = m_restrictPermsCheck->isChecked() ? m_ownerPassEdit->text().toStdString() : "";
    m_config.algorithm = m_algoCombo->currentData().toString().toStdString();
    m_config.keyLengthBits = (m_config.algorithm == "AES-256") ? 256 : 128;

    m_config.permissions.canPrint = m_allowPrintCheck->isChecked();
    m_config.permissions.canPrintHighQuality = m_allowPrintCheck->isChecked();
    m_config.permissions.canCopyText = m_allowCopyCheck->isChecked();
    m_config.permissions.canModifyContents = m_allowModifyCheck->isChecked();
    m_config.permissions.canAnnotate = m_allowAnnotCheck->isChecked();
    m_config.permissions.canFillForms = m_allowFormsCheck->isChecked();
    m_config.permissions.canAssemble = m_allowModifyCheck->isChecked();
    m_config.permissions.isEncrypted = true;

    accept();
}

pdf_api::DocumentEncryptionConfig DocumentSecurityDialog::GetConfig() const {
    return m_config;
}

} // namespace pdfinity::ui
