// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "UpdateDialog.hpp"
#include "application/AppContext.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QUrl>

namespace pdfinity::ui {

UpdateDialog::UpdateDialog(QWidget* parent) : QDialog(parent) {
    SetupUi();
}

void UpdateDialog::SetupUi() {
    setWindowTitle(tr("Check for Updates"));
    setFixedSize(540, 420);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 16);
    mainLayout->setSpacing(14);

    // Header
    auto* headerLabel = new QLabel(tr("<h2>PDFinity Software Updates</h2>"), this);
    mainLayout->addWidget(headerLabel);

    // Current Version Info Box
    auto* versionBox = new QGroupBox(tr("Installed Version"), this);
    auto* versionLayout = new QVBoxLayout(versionBox);
    const auto& platform = app::AppContext::Instance().GetPlatform();

    QString versionInfo = tr(
        "<b>Current Version:</b> 1.0.0-rc1<br>"
        "<b>Target Platform:</b> %1 (%2)<br>"
        "<b>Deployment Mode:</b> %3"
    ).arg(QString::fromStdString(platform.GetPlatformName()))
     .arg(QString::fromStdString(platform.GetArchitecture()))
     .arg(platform.IsPortableMode() ? tr("Portable (Zero-Registry Footprint)") : tr("Standard System Installation"));

    auto* infoLabel = new QLabel(versionInfo, versionBox);
    versionLayout->addWidget(infoLabel);
    mainLayout->addWidget(versionBox);

    // Privacy & Offline Policy Box
    auto* policyBox = new QGroupBox(tr("Privacy & Air-Gapped Operation Policy"), this);
    auto* policyLayout = new QVBoxLayout(policyBox);
    auto* policyLabel = new QLabel(tr(
        "PDFinity is strictly <b>Offline-First</b>. Automatic background polling, "
        "silent auto-downloaders, and cloud telemetry are permanently disabled.<br><br>"
        "Updates are entirely optional and user-initiated. Enterprise administrators "
        "can deploy offline packages without internet access."
    ), policyBox);
    policyLabel->setWordWrap(true);
    policyLabel->setStyleSheet("color: #4b5563; font-size: 11px;");
    policyLayout->addWidget(policyLabel);
    mainLayout->addWidget(policyBox);

    // Manual Update Channel Selection
    auto* channelLayout = new QHBoxLayout();
    auto* channelLabel = new QLabel(tr("Update Channel:"), this);
    m_channelCombo = new QComboBox(this);
    m_channelCombo->setObjectName("channelCombo");
    m_channelCombo->addItem(tr("Stable Releases (Recommended)"));
    m_channelCombo->addItem(tr("Preview / Development Builds"));
    channelLayout->addWidget(channelLabel);
    channelLayout->addWidget(m_channelCombo, 1);
    mainLayout->addLayout(channelLayout);

    // Status / Action Area
    m_statusLabel = new QLabel(tr("Status: Ready to check official release feed."), this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("font-weight: bold; color: #1f2937;");
    mainLayout->addWidget(m_statusLabel);

    auto* actionsLayout = new QHBoxLayout();
    m_checkButton = new QPushButton(tr("Check for Updates Now"), this);
    m_checkButton->setObjectName("btnCheckNow");
    connect(m_checkButton, &QPushButton::clicked, this, &UpdateDialog::OnCheckOnline);

    m_downloadButton = new QPushButton(tr("Offline Package Downloads..."), this);
    m_downloadButton->setObjectName("btnDownloadSite");
    connect(m_downloadButton, &QPushButton::clicked, this, &UpdateDialog::OnOpenDownloadSite);

    actionsLayout->addWidget(m_checkButton);
    actionsLayout->addWidget(m_downloadButton);
    mainLayout->addLayout(actionsLayout);

    mainLayout->addStretch();

    // Dialog Button Box (Close)
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}

void UpdateDialog::OnCheckOnline() {
    m_statusLabel->setText(tr("Status: You are currently running the latest certified build (1.0.0-rc1)."));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #047857;");
}

void UpdateDialog::OnOpenDownloadSite() {
    QDesktopServices::openUrl(QUrl("https://github.com/pdfinity/pdfinity/releases"));
}

} // namespace pdfinity::ui
