// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AboutDialog.hpp"
#include "application/AppContext.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QDialogButtonBox>

namespace pdfinity::ui {

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    SetupUi();
}

void AboutDialog::SetupUi() {
    setWindowTitle(tr("About PDFinity"));
    setFixedSize(540, 420);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 16);
    mainLayout->setSpacing(14);

    // Header with App Title and Version
    auto* headerLayout = new QVBoxLayout();
    auto* titleLabel = new QLabel(tr("<h2>PDFinity 0.1.7-Beta</h2>"), this);
    auto* subtitleLabel = new QLabel(tr("<b>Professional Offline-First PDF Productivity Suite</b><br>"
                                        "Published by <a href='https://www.stacksyncsolutions.com/'>StackSync Solutions</a><br>"
                                        "Lead Programmer: <a href='https://github.com/jakepanlilio2000'>Jake Ashley Panlilio</a><br>"
                                        "License: <a href='https://mozilla.org/MPL/2.0/'>Mozilla Public License Version 2.0 (MPL-2.0)</a>"), this);
    subtitleLabel->setOpenExternalLinks(true);
    subtitleLabel->setStyleSheet("color: #4b5563;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    mainLayout->addLayout(headerLayout);

    // Tab widget for Information vs Licenses
    auto* tabWidget = new QTabWidget(this);

    // Tab 1: System and Security Information
    auto* infoWidget = new QWidget(tabWidget);
    auto* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(12, 12, 12, 12);
    infoLayout->setSpacing(8);

    const auto& platform = app::AppContext::Instance().GetPlatform();
    QString sysInfo = tr(
        "<b>Operating System:</b> %1 (%2)<br>"
        "<b>OS Version:</b> %3<br>"
        "<b>Architecture:</b> %4<br>"
        "<b>High-DPI Awareness:</b> %5<br>"
        "<b>Primary Display DPI:</b> %6<br>"
        "<b>UI Framework:</b> Qt 6.8.2 LTS (QtWidgets)<br>"
        "<b>C++ Standard:</b> ISO C++20<br>"
        "<b>Security Guarantee:</b> 100% Offline-First, Zero Telemetry, No Cloud Tracking<br>"
        "<b>Document Protection:</b> Atomic file replacement, memory scrubbing"
    ).arg(QString::fromStdString(platform.GetPlatformName()))
     .arg(QString::fromStdString(platform.GetArchitecture()))
     .arg(QString::fromStdString(platform.GetPlatformVersion()))
     .arg(QString::fromStdString(platform.GetArchitecture()))
     .arg(platform.HasCapability(platform::PlatformCapability::PerMonitorDpiV2) ? "Per-Monitor DPI v2" : "System DPI")
     .arg(platform.GetPrimaryScreenDpi());

    auto* sysInfoLabel = new QLabel(sysInfo, infoWidget);
    sysInfoLabel->setWordWrap(true);
    infoLayout->addWidget(sysInfoLabel);
    infoLayout->addStretch();
    tabWidget->addTab(infoWidget, tr("System & Security"));

    // Tab 2: Open Source Licenses & Compliance
    auto* licenseWidget = new QWidget(tabWidget);
    auto* licenseLayout = new QVBoxLayout(licenseWidget);
    licenseLayout->setContentsMargins(12, 12, 12, 12);

    auto* licenseText = new QTextEdit(licenseWidget);
    licenseText->setReadOnly(true);
    licenseText->setHtml(tr(
        "<h3>Open Source Licensing Governance</h3>"
        "<p><b>PDFinity Core & Application:</b> Mozilla Public License Version 2.0 (MPL-2.0). Copyright (c) 2026 StackSync Solutions & Jake Ashley Panlilio.</p>"
        "<p>PDFinity utilizes third-party components under permissive and weak copyleft open-source licenses:</p>"
        "<ul>"
        "<li><b>Qt 6 Framework:</b> GNU Lesser General Public License version 3 (LGPLv3). Dynamically linked. User is free to replace Qt dynamic libraries. Source code available at <a href='https://code.qt.io'>https://code.qt.io</a>.</li>"
        "<li><b>Google PDFium:</b> Apache 2.0 / BSD 3-Clause.</li>"
        "<li><b>QPDF:</b> Apache 2.0.</li>"
        "<li><b>OpenSSL:</b> Apache 2.0.</li>"
        "<li><b>SQLite:</b> Public Domain.</li>"
        "<li><b>nlohmann/json:</b> MIT License.</li>"
        "</ul>"
        "<p><i>Note: Viral copyleft libraries (Poppler under GPLv3, MuPDF under AGPLv3) are strictly prohibited and absent from this application.</i></p>"
    ));
    licenseLayout->addWidget(licenseText);
    tabWidget->addTab(licenseWidget, tr("Licenses"));

    mainLayout->addWidget(tabWidget);

    // Close button
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}

} // namespace pdfinity::ui
