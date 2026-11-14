// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SanitizationDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

namespace pdfinity::ui {

SanitizationDialog::SanitizationDialog(
    const std::filesystem::path& sourcePath,
    QWidget* parent
)
    : QDialog(parent),
      m_sourcePath(sourcePath)
{
    setWindowTitle("Sanitize Document - High-Assurance Privacy & Metadata Scrubbing");
    resize(560, 440);
    setModal(true);

    setupUi(sourcePath);
}

void SanitizationDialog::setupUi(const std::filesystem::path& sourcePath) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    auto* banner = new QLabel(
        "<b>🛡 Document Sanitization:</b> Remove hidden data, metadata, attachments, and executable scripts "
        "before publishing or distributing the document. Output is written to a clean new file.", this
    );
    banner->setWordWrap(true);
    banner->setStyleSheet("background-color: #EFF6FF; color: #1E40AF; border: 1px solid #93C5FD; "
                          "border-radius: 6px; padding: 10px; font-size: 13px;");
    mainLayout->addWidget(banner);

    // Profile selector
    auto* profLayout = new QHBoxLayout();
    profLayout->addWidget(new QLabel("Sanitization Profile:", this));
    m_profileCombo = new QComboBox(this);
    m_profileCombo->addItem("Standard Sanitization (Recommended)", static_cast<int>(pdf_api::SanitizationProfile::StandardSanitize));
    m_profileCombo->addItem("High-Assurance Clean (Strict Clean Rebuild)", static_cast<int>(pdf_api::SanitizationProfile::HighAssuranceClean));
    m_profileCombo->addItem("Redaction Only (Keep Metadata)", static_cast<int>(pdf_api::SanitizationProfile::RedactionOnly));
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SanitizationDialog::onProfileChanged);
    profLayout->addWidget(m_profileCombo, 1);
    mainLayout->addLayout(profLayout);

    // Options group
    auto* group = new QGroupBox("Sanitization Targets", this);
    auto* grpLayout = new QVBoxLayout(group);

    m_infoMetaCheck = new QCheckBox("Scrub Document Information (Title, Author, Subject, Keywords, Creator, Dates)", group);
    m_infoMetaCheck->setChecked(true);
    grpLayout->addWidget(m_infoMetaCheck);

    m_xmpMetaCheck = new QCheckBox("Strip XMP Metadata XML Stream from Catalog (/Metadata)", group);
    m_xmpMetaCheck->setChecked(true);
    grpLayout->addWidget(m_xmpMetaCheck);

    m_attachmentsCheck = new QCheckBox("Remove Embedded File Attachments (/EmbeddedFiles)", group);
    m_attachmentsCheck->setChecked(true);
    grpLayout->addWidget(m_attachmentsCheck);

    m_jsCheck = new QCheckBox("Remove Document JavaScript Actions and Scripts (/JavaScript, /OpenAction)", group);
    m_jsCheck->setChecked(true);
    grpLayout->addWidget(m_jsCheck);

    m_hiddenLayersCheck = new QCheckBox("Remove Hidden Layers & Optional Content Groups (/OCProperties, /OCGs)", group);
    m_hiddenLayersCheck->setChecked(false);
    grpLayout->addWidget(m_hiddenLayersCheck);

    m_annotationsCheck = new QCheckBox("Remove All Comments, Markups, and Annotations (/Annots)", group);
    m_annotationsCheck->setChecked(false);
    grpLayout->addWidget(m_annotationsCheck);

    mainLayout->addWidget(group);

    // Destination path
    auto* destLayout = new QHBoxLayout();
    destLayout->addWidget(new QLabel("Save Sanitized Copy To:", this));

    std::filesystem::path defDst = sourcePath;
    if (!sourcePath.empty()) {
        std::string stem = sourcePath.stem().string();
        defDst = sourcePath.parent_path() / (stem + "_sanitized.pdf");
    } else {
        defDst = std::filesystem::current_path() / "document_sanitized.pdf";
    }
    m_destinationPath = defDst;

    m_destPathEdit = new QLineEdit(QString::fromStdString(defDst.string()), this);
    destLayout->addWidget(m_destPathEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &SanitizationDialog::onBrowseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_sanitizeBtn = new QPushButton("Sanitize & Save Copy...", this);
    m_sanitizeBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px;");
    connect(m_sanitizeBtn, &QPushButton::clicked, this, &SanitizationDialog::onSanitizeClicked);
    btnLayout->addWidget(m_sanitizeBtn);

    mainLayout->addLayout(btnLayout);
}

void SanitizationDialog::onProfileChanged(int index) {
    if (index == 0) { // Standard
        m_infoMetaCheck->setChecked(true);
        m_xmpMetaCheck->setChecked(true);
        m_attachmentsCheck->setChecked(true);
        m_jsCheck->setChecked(true);
        m_hiddenLayersCheck->setChecked(false);
        m_annotationsCheck->setChecked(false);
    } else if (index == 1) { // High-Assurance
        m_infoMetaCheck->setChecked(true);
        m_xmpMetaCheck->setChecked(true);
        m_attachmentsCheck->setChecked(true);
        m_jsCheck->setChecked(true);
        m_hiddenLayersCheck->setChecked(true);
        m_annotationsCheck->setChecked(true);
    } else if (index == 2) { // Redaction only
        m_infoMetaCheck->setChecked(false);
        m_xmpMetaCheck->setChecked(false);
        m_attachmentsCheck->setChecked(false);
        m_jsCheck->setChecked(false);
        m_hiddenLayersCheck->setChecked(false);
        m_annotationsCheck->setChecked(false);
    }
}

void SanitizationDialog::onBrowseDestination() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Sanitized PDF Copy",
        m_destPathEdit ? m_destPathEdit->text() : "",
        "PDF Documents (*.pdf)"
    );
    if (!path.isEmpty()) {
        m_destPathEdit->setText(path);
        m_destinationPath = std::filesystem::path(path.toStdString());
    }
}

void SanitizationDialog::onSanitizeClicked() {
    if (!m_destPathEdit) return;
    std::filesystem::path chosenDst(m_destPathEdit->text().toStdString());

    std::error_code ec;
    if (std::filesystem::equivalent(m_sourcePath, chosenDst, ec)) {
        QMessageBox::critical(
            this, "Error: In-Place Overwrite Prohibited",
            "Sanitization requires writing to a separate destination file to guarantee source preservation. Please choose a different filename."
        );
        return;
    }

    m_destinationPath = chosenDst;
    accept();
}

pdf_api::SanitizationOptions SanitizationDialog::GetOptions() const {
    pdf_api::SanitizationOptions opt;
    opt.scrubInfoMetadata = m_infoMetaCheck ? m_infoMetaCheck->isChecked() : true;
    opt.scrubXmpMetadata = m_xmpMetaCheck ? m_xmpMetaCheck->isChecked() : true;
    opt.removeAttachments = m_attachmentsCheck ? m_attachmentsCheck->isChecked() : true;
    opt.removeJavaScript = m_jsCheck ? m_jsCheck->isChecked() : true;
    opt.removeHiddenLayers = m_hiddenLayersCheck ? m_hiddenLayersCheck->isChecked() : false;
    opt.removeAllAnnotations = m_annotationsCheck ? m_annotationsCheck->isChecked() : false;
    return opt;
}

std::filesystem::path SanitizationDialog::GetDestinationPath() const {
    return m_destinationPath;
}

} // namespace pdfinity::ui
