// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DigitalSignatureDialog.hpp"
#include "CertificateManagerDialog.hpp"
#include "security/CryptoCertificateManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

namespace pdfinity::ui {

DigitalSignatureDialog::DigitalSignatureDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Cryptographic Digital PDF Signature"));
    setMinimumWidth(520);
    SetupUi();
    RefreshCertificates();
}

void DigitalSignatureDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Cryptographic Badge
    auto* banner = new QLabel(
        tr("🔐 <b>CRYPTOGRAPHIC DIGITAL SIGNATURE:</b> This operation signs the document byte range using your "
           "X.509 private key via an ISO 32000 incremental update. The resulting detached PKCS#7 signature "
           "guarantees non-repudiation and detects any post-signing file modification or tampering."),
        this
    );
    banner->setWordWrap(true);
    banner->setStyleSheet(
        "background-color: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7; "
        "border-radius: 4px; padding: 10px; font-size: 11px;"
    );
    mainLayout->addWidget(banner);

    auto* formLayout = new QFormLayout();

    // Certificate row with Manage button
    auto* certRow = new QHBoxLayout();
    m_certCombo = new QComboBox(this);
    auto* manageBtn = new QPushButton(tr("Manage..."), this);
    connect(manageBtn, &QPushButton::clicked, this, &DigitalSignatureDialog::OnManageCertificates);
    certRow->addWidget(m_certCombo, 1);
    certRow->addWidget(manageBtn);
    formLayout->addRow(tr("Signing Certificate:"), certRow);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Unlock password for private key (if required)"));
    formLayout->addRow(tr("Private Key Password:"), m_passwordEdit);

    m_reasonEdit = new QLineEdit(this);
    m_reasonEdit->setText(tr("I approve the content and authenticity of this document"));
    formLayout->addRow(tr("Signing Reason:"), m_reasonEdit);

    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText(tr("City, Country or Office (Optional)"));
    formLayout->addRow(tr("Location:"), m_locationEdit);

    m_contactEdit = new QLineEdit(this);
    m_contactEdit->setPlaceholderText(tr("signer@example.com (Optional)"));
    formLayout->addRow(tr("Contact Info:"), m_contactEdit);

    m_includeVisualCheck = new QCheckBox(tr("Embed visual signature badge on current page"), this);
    m_includeVisualCheck->setChecked(true);
    formLayout->addRow("", m_includeVisualCheck);

    mainLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    auto* signBtn = new QPushButton(tr("Sign Document"), this);
    signBtn->setDefault(true);
    connect(signBtn, &QPushButton::clicked, this, &DigitalSignatureDialog::OnAccept);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(signBtn);
    mainLayout->addLayout(btnLayout);
}

void DigitalSignatureDialog::RefreshCertificates() {
    m_certCombo->clear();
    auto certs = security::CryptoCertificateManager::Instance().GetInstalledCertificates();
    for (const auto& c : certs) {
        QString label = QString("%1 (%2) [Expires: %3]")
            .arg(QString::fromStdString(c.subject))
            .arg(QString::fromStdString(c.issuer))
            .arg(QString::fromStdString(c.validTo));
        m_certCombo->addItem(label, QString::fromStdString(c.thumbprintSha256));
    }

    if (m_certCombo->count() == 0) {
        m_certCombo->addItem(tr("-- No Certificates Installed (Click Manage to Import) --"), "");
    }
}

void DigitalSignatureDialog::OnManageCertificates() {
    CertificateManagerDialog dlg(this);
    dlg.exec();
    RefreshCertificates();
}

void DigitalSignatureDialog::OnAccept() {
    QString thumbprint = m_certCombo->currentData().toString();
    if (thumbprint.isEmpty()) {
        QMessageBox::warning(this, tr("Certificate Required"), tr("Please select or import a signing certificate."));
        return;
    }

    m_options.certThumbprint = thumbprint.toStdString();
    m_options.reason = m_reasonEdit->text().trimmed().toStdString();
    m_options.location = m_locationEdit->text().trimmed().toStdString();
    m_options.contactInfo = m_contactEdit->text().trimmed().toStdString();
    m_options.includeVisualAppearance = m_includeVisualCheck->isChecked();
    m_password = security::SecureString(m_passwordEdit->text().toStdString());

    accept();
}

pdf_api::CryptographicSignOptions DigitalSignatureDialog::GetSignOptions() const {
    return m_options;
}

security::SecureString DigitalSignatureDialog::GetPassword() const {
    return m_password;
}

} // namespace pdfinity::ui
