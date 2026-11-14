// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SignatureValidationDialog.hpp"
#include "services/SignatureService.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <fstream>

namespace pdfinity::ui {

SignatureValidationDialog::SignatureValidationDialog(
    const std::filesystem::path& pdfPath,
    const std::vector<pdf_api::SignatureValidationResult>& results,
    QWidget* parent
) : QDialog(parent), m_pdfPath(pdfPath), m_results(results) {
    setWindowTitle(tr("Digital Signature Validation & Security Audit"));
    resize(760, 480);
    SetupUi();
    PopulateSignatures();
}

void SignatureValidationDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Global Status Header Banner
    bool allValid = !m_results.empty();
    for (const auto& sig : m_results) {
        if (!sig.hasValidDigest || sig.isTampered) {
            allValid = false;
            break;
        }
    }

    QLabel* statusBanner = nullptr;
    if (m_results.empty()) {
        statusBanner = new QLabel(
            tr("ℹ️ <b>No Cryptographic Signatures Detected:</b> This document contains no PKCS#7 digital signature fields. "
               "Any visual signatures or stamps are purely graphical and do not guarantee integrity."),
            this
        );
        statusBanner->setStyleSheet("background-color: #F5F5F5; color: #555; padding: 10px; border-radius: 4px;");
    } else if (allValid) {
        statusBanner = new QLabel(
            tr("✅ <b>VALID SIGNATURES:</b> All digital signatures in this document are mathematically valid. "
               "No tampering or unauthorized modifications were detected across the signed byte ranges."),
            this
        );
        statusBanner->setStyleSheet("background-color: #E8F5E9; color: #1B5E20; padding: 10px; border: 1px solid #A5D6A7; border-radius: 4px;");
    } else {
        statusBanner = new QLabel(
            tr("❌ <b>TAMPERING DETECTED:</b> One or more cryptographic signatures in this document are <b>INVALID</b>. "
               "The signed byte range has been modified or corrupted since the document was signed!"),
            this
        );
        statusBanner->setStyleSheet("background-color: #FFEBEE; color: #B71C1C; padding: 10px; border: 1px solid #EF9A9A; border-radius: 4px; font-weight: bold;");
    }
    statusBanner->setWordWrap(true);
    mainLayout->addWidget(statusBanner);

    // Splitter: List on Left, HTML Details on Right
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_sigList = new QListWidget(this);
    m_sigList->setMinimumWidth(220);
    connect(m_sigList, &QListWidget::currentRowChanged, this, &SignatureValidationDialog::OnSignatureSelected);
    splitter->addWidget(m_sigList);

    m_detailsBrowser = new QTextBrowser(this);
    m_detailsBrowser->setOpenExternalLinks(true);
    splitter->addWidget(m_detailsBrowser);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    // Bottom Action Bar
    auto* btnLayout = new QHBoxLayout();
    m_exportReportBtn = new QPushButton(tr("📄 Export Security Report..."), this);
    connect(m_exportReportBtn, &QPushButton::clicked, this, &SignatureValidationDialog::OnExportReport);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_exportReportBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

void SignatureValidationDialog::PopulateSignatures() {
    m_sigList->clear();
    for (size_t i = 0; i < m_results.size(); ++i) {
        const auto& sig = m_results[i];
        QString icon = (sig.hasValidDigest && !sig.isTampered) ? "✅ " : "❌ ";
        QString title = QString("Signature #%1 (%2)")
            .arg(i + 1)
            .arg(sig.certificate.subject.empty() ? "Digital Signer" : QString::fromStdString(sig.certificate.subject));
        m_sigList->addItem(icon + title);
    }

    if (!m_results.empty()) {
        m_sigList->setCurrentRow(0);
    } else {
        m_detailsBrowser->setHtml(
            tr("<div style='padding: 20px; text-align: center; color: #777;'>"
               "<h3>No Digital Signatures Present</h3>"
               "<p>Visual ink marks, handwritten stamps, or imported images on this document are graphic overlays "
               "and do not bind an X.509 certificate to the document byte stream.</p>"
               "</div>")
        );
    }
}

void SignatureValidationDialog::OnSignatureSelected(int index) {
    if (index < 0 || index >= static_cast<int>(m_results.size())) return;
    const auto& sig = m_results[index];

    QString statusColor = (sig.hasValidDigest && !sig.isTampered) ? "#2E7D32" : "#C62828";
    QString statusText = (sig.hasValidDigest && !sig.isTampered) ? tr("VALID (Digest Matches)") : tr("INVALID / TAMPERED");

    QString html;
    html += QString("<div style='font-family: sans-serif; padding: 10px;'>");
    html += QString("<h2>Signature Details #%1</h2>").arg(index + 1);

    html += QString("<table style='width: 100%; border-collapse: collapse; margin-bottom: 16px;'>");
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Integrity:</td>"
                    "<td style='padding: 4px; color: %1; font-weight: bold;'>%2</td></tr>")
                    .arg(statusColor, statusText);
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Signer (Subject):</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(QString::fromStdString(sig.certificate.subject));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Certificate Issuer:</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(QString::fromStdString(sig.certificate.issuer));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Serial Number:</td><td style='padding: 4px;'><code>%1</code></td></tr>")
        .arg(QString::fromStdString(sig.certificate.serialNumber));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Validity Dates:</td><td style='padding: 4px;'>%1 to %2</td></tr>")
        .arg(QString::fromStdString(sig.certificate.validFrom), QString::fromStdString(sig.certificate.validTo));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>SHA-256 Thumbprint:</td><td style='padding: 4px;'><code>%1</code></td></tr>")
        .arg(QString::fromStdString(sig.certificate.thumbprintSha256));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Signing Reason:</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(QString::fromStdString(sig.reason.empty() ? "(None)" : sig.reason));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Signing Time:</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(QString::fromStdString(sig.signingTime.empty() ? "(Unknown)" : sig.signingTime));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>SubFilter Format:</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(QString::fromStdString(sig.subFilter));
    html += QString("<tr><td style='padding: 4px; font-weight: bold;'>Post-Sign Changes:</td><td style='padding: 4px;'>%1</td></tr>")
        .arg(sig.hasModificationsAfterSigning ? tr("⚠️ Incremental additions detected after signed byte range") : tr("None (Signature covers file to EOF)"));
    html += "</table>";

    // Trust Limitation
    html += "<div style='background-color: #F8F9FA; border: 1px solid #E0E0E0; border-radius: 4px; padding: 10px; font-size: 11px;'>";
    html += "<b>Offline Validation Trust Limitation:</b><br/>";
    html += QString::fromStdString(sig.revocationStatus);
    html += "</div></div>";

    m_detailsBrowser->setHtml(html);
}

void SignatureValidationDialog::OnExportReport() {
    QString savePath = QFileDialog::getSaveFileName(
        this, tr("Export Security & Signature Audit Report"), "Signature_Audit_Report.md", tr("Markdown Files (*.md);;Text Files (*.txt)")
    );
    if (savePath.isEmpty()) return;

    std::string report = services::SignatureService::GenerateSecurityReport(m_pdfPath, m_results);
    std::ofstream out(savePath.toStdString());
    if (out.is_open()) {
        out << report;
        out.close();
        QMessageBox::information(this, tr("Report Exported"), tr("Security report successfully saved to:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write report to file."));
    }
}

} // namespace pdfinity::ui
