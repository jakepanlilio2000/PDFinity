// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "CertificateManagerDialog.hpp"
#include "security/CryptoCertificateManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>

namespace pdfinity::ui {

CertificateManagerDialog::CertificateManagerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Digital Certificate Manager & PKCS#12 Vault"));
    resize(720, 380);
    SetupUi();
    OnRefreshList();
}

void CertificateManagerDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* header = new QLabel(
        tr("<b>Digital Certificate Vault:</b> Certificates stored here are protected with Windows DPAPI encryption. "
           "Private keys are never exposed in plaintext and are used exclusively for detached cryptographic PDF signing."),
        this
    );
    header->setWordWrap(true);
    header->setStyleSheet("color: #444; margin-bottom: 6px;");
    mainLayout->addWidget(header);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        tr("Subject (Friendly Name)"),
        tr("Issuer"),
        tr("Valid Until"),
        tr("Status"),
        tr("SHA-256 Thumbprint")
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout();
    m_importBtn = new QPushButton(tr("📥 Import PKCS#12 (.p12 / .pfx)..."), this);
    connect(m_importBtn, &QPushButton::clicked, this, &CertificateManagerDialog::OnImportCertificate);

    m_deleteBtn = new QPushButton(tr("🗑️ Remove Certificate"), this);
    connect(m_deleteBtn, &QPushButton::clicked, this, &CertificateManagerDialog::OnDeleteCertificate);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_importBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

void CertificateManagerDialog::OnRefreshList() {
    m_table->setRowCount(0);
    auto certs = security::CryptoCertificateManager::Instance().GetInstalledCertificates();

    for (const auto& c : certs) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.subject)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c.issuer)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c.validTo)));

        auto* statusItem = new QTableWidgetItem(c.isExpired ? tr("Expired") : tr("Active"));
        statusItem->setForeground(c.isExpired ? Qt::red : QColor(0, 140, 40));
        m_table->setItem(row, 3, statusItem);

        m_table->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(c.thumbprintSha256)));
    }
}

void CertificateManagerDialog::OnImportCertificate() {
    QString pfxPath = QFileDialog::getOpenFileName(
        this, tr("Select PKCS#12 Certificate Archive"), "", tr("PKCS#12 Archives (*.pfx *.p12)")
    );
    if (pfxPath.isEmpty()) return;

    bool ok = false;
    QString pwd = QInputDialog::getText(
        this, tr("Certificate Password"),
        tr("Enter the password to unlock this PKCS#12 archive:"),
        QLineEdit::Password, "", &ok
    );
    if (!ok) return;

    security::SecureString pass(pwd.toStdString());
    auto res = security::CryptoCertificateManager::Instance().ImportPkcs12(
        std::filesystem::path(pfxPath.toStdString()), pass
    );

    if (res.has_value()) {
        QMessageBox::information(
            this, tr("Import Successful"),
            tr("Certificate successfully imported to secure vault:\n\nSubject: %1\nThumbprint: %2")
                .arg(QString::fromStdString(res.value.subject))
                .arg(QString::fromStdString(res.value.thumbprintSha256))
        );
        OnRefreshList();
    } else {
        QMessageBox::critical(
            this, tr("Import Failed"),
            QString::fromStdString(res.errorMessage)
        );
    }
}

void CertificateManagerDialog::OnDeleteCertificate() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("Select Certificate"), tr("Please select a certificate to remove."));
        return;
    }

    QString thumbprint = m_table->item(row, 4)->text();
    auto confirm = QMessageBox::question(
        this, tr("Confirm Removal"),
        tr("Are you sure you want to remove this certificate from your secure vault?\n\nThumbprint: %1").arg(thumbprint),
        QMessageBox::Yes | QMessageBox::No
    );

    if (confirm == QMessageBox::Yes) {
        security::CryptoCertificateManager::Instance().DeleteCertificate(thumbprint.toStdString());
        OnRefreshList();
    }
}

} // namespace pdfinity::ui
