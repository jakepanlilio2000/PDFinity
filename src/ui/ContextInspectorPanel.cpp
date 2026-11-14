// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ContextInspectorPanel.hpp"
#include "VectorIcons.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QFileInfo>
#include <QFrame>
#include <cmath>

namespace pdfinity::ui {

ContextInspectorPanel::ContextInspectorPanel(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void ContextInspectorPanel::SetupUi() {
    setFixedWidth(280);
    setStyleSheet(
        "QWidget {"
        "  background-color: #f8fafc;"
        "  border-left: 1px solid #cbd5e1;"
        "}"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header bar
    auto* header = new QWidget(this);
    header->setStyleSheet("background-color: #f1f5f9; border-bottom: 1px solid #e2e8f0;");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 6, 8, 6);

    auto* headerTitle = new QLabel(tr("<b>Document & Tool Studio</b>"), header);
    headerTitle->setStyleSheet("color: #334155; font-size: 12px; border: none;");

    auto* closeBtn = new QPushButton("✕", header);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet("border: none; background: transparent; color: #64748b; font-weight: bold;");
    connect(closeBtn, &QPushButton::clicked, this, &ContextInspectorPanel::CloseRequested);

    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layout->addWidget(header);

    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: #ffffff; }"
        "QTabBar::tab { background: #f1f5f9; border: none; padding: 6px 14px; font-weight: 500; font-size: 11px; }"
        "QTabBar::tab:selected { background: #ffffff; border-bottom: 2px solid #2563eb; color: #2563eb; font-weight: 600; }"
    );

    m_tabWidget->addTab(CreatePropertiesTab(), tr("Properties"));
    m_tabWidget->addTab(CreateToolsPaletteTab(), tr("All Tools"));

    layout->addWidget(m_tabWidget);
}

QWidget* ContextInspectorPanel::CreatePropertiesTab() {
    auto* scroll = new QScrollArea(m_tabWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget(scroll);
    content->setStyleSheet("background-color: #ffffff;");
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

    // File Group
    auto* fileHeader = new QLabel(tr("<b>FILE METRICS</b>"), content);
    fileHeader->setStyleSheet("color: #64748b; font-size: 10px; font-weight: 700; border: none;");
    layout->addWidget(fileHeader);

    auto* fileForm = new QFormLayout();
    fileForm->setSpacing(6);
    m_propFileName = new QLabel("-", content);
    m_propFileName->setWordWrap(true);
    m_propFileSize = new QLabel("-", content);
    m_propPageCount = new QLabel("-", content);
    fileForm->addRow(tr("Name:"), m_propFileName);
    fileForm->addRow(tr("Size:"), m_propFileSize);
    fileForm->addRow(tr("Pages:"), m_propPageCount);
    layout->addLayout(fileForm);

    auto* sep1 = new QFrame(content);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #e2e8f0;");
    layout->addWidget(sep1);

    // Viewport Context Group
    auto* viewHeader = new QLabel(tr("<b>ACTIVE VIEWPORT</b>"), content);
    viewHeader->setStyleSheet("color: #64748b; font-size: 10px; font-weight: 700; border: none;");
    layout->addWidget(viewHeader);

    auto* viewForm = new QFormLayout();
    viewForm->setSpacing(6);
    m_propPageDimensions = new QLabel("-", content);
    m_propZoomLevel = new QLabel("100%", content);
    viewForm->addRow(tr("Dimensions:"), m_propPageDimensions);
    viewForm->addRow(tr("Zoom:"), m_propZoomLevel);
    layout->addLayout(viewForm);

    auto* sep2 = new QFrame(content);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #e2e8f0;");
    layout->addWidget(sep2);

    // Security & Compliance Group
    auto* secHeader = new QLabel(tr("<b>SECURITY & TRUST</b>"), content);
    secHeader->setStyleSheet("color: #64748b; font-size: 10px; font-weight: 700; border: none;");
    layout->addWidget(secHeader);

    auto* secForm = new QFormLayout();
    secForm->setSpacing(6);
    m_propEncryption = new QLabel(tr("Standard / None"), content);
    m_propPermissions = new QLabel(tr("Full Access (Print, Copy, Edit)"), content);
    m_propPermissions->setWordWrap(true);
    m_propSignatures = new QLabel(tr("None"), content);
    secForm->addRow(tr("Encryption:"), m_propEncryption);
    secForm->addRow(tr("Permissions:"), m_propPermissions);
    secForm->addRow(tr("Signatures:"), m_propSignatures);
    layout->addLayout(secForm);

    layout->addStretch();
    scroll->setWidget(content);
    return scroll;
}

QWidget* ContextInspectorPanel::CreateToolsPaletteTab() {
    auto* scroll = new QScrollArea(m_tabWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget(scroll);
    content->setStyleSheet("background-color: #ffffff;");
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto addCategoryHeader = [layout, content](const QString& title) {
        auto* label = new QLabel(QString("<b>%1</b>").arg(title), content);
        label->setStyleSheet("color: #64748b; font-size: 10px; font-weight: 700; margin-top: 6px; border: none;");
        layout->addWidget(label);
    };

    // Category 1: Assembly
    addCategoryHeader(tr("PAGE ASSEMBLY"));
    layout->addWidget(CreateToolButton(tr("Organize Pages"), tr("Reorder, rotate, crop, delete"), IconType::Organize, [this]() { emit OrganizePagesTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Merge Documents"), tr("Combine multiple files"), IconType::Merge, [this]() { emit MergeDocumentsTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Split Document"), tr("Split by range or size"), IconType::Split, [this]() { emit SplitDocumentTriggered(); }));

    // Category 2: Content & Forms
    addCategoryHeader(tr("FORMS & CONTENT"));
    layout->addWidget(CreateToolButton(tr("Form Inspector & Fill"), tr("AcroForm input & data exchange"), IconType::Forms, [this]() { emit FormInspectorTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Content Text Editor"), tr("Edit existing text objects"), IconType::ContentEdit, [this]() { emit ContentEditorTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Object Inspector"), tr("Vector paths, images, streams"), IconType::Properties, [this]() { emit ObjectInspectorTriggered(); }));

    // Category 3: Security & Signatures
    addCategoryHeader(tr("SECURITY & SIGNATURES"));
    layout->addWidget(CreateToolButton(tr("Digital Sign (PKCS#7)"), tr("Cryptographic certificate signing"), IconType::DigitalSign, [this]() { emit DigitalSignTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Visual Signature Stamp"), tr("Placement stamp overlay"), IconType::VisualSign, [this]() { emit VisualSignTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Validate Signatures"), tr("Verify byte ranges and integrity"), IconType::Shield, [this]() { emit ValidateSignaturesTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Redaction Studio"), tr("Permanent content removal"), IconType::Redaction, [this]() { emit RedactionStudioTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Sanitize Metadata"), tr("Scrub hidden sensitive data"), IconType::Sanitize, [this]() { emit SanitizeTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Certificate Manager"), tr("Manage digital ID keypairs"), IconType::Properties, [this]() { emit CertificateManagerTriggered(); }));

    // Category 4: Transformation & Automate
    addCategoryHeader(tr("PROCESS & AUTOMATE"));
    layout->addWidget(CreateToolButton(tr("OCR Studio"), tr("Generate searchable text layer"), IconType::Ocr, [this]() { emit OcrStudioTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Conversion Center"), tr("Images, text, and office interchange"), IconType::Compare, [this]() { emit ConversionCenterTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Optimizer / Compress"), tr("Stream compression presets"), IconType::Optimize, [this]() { emit OptimizerTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Document Intelligence"), tr("Deep structural & font audit"), IconType::Properties, [this]() { emit DocumentIntelligenceTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Compare Documents"), tr("Visual & semantic text difference"), IconType::Compare, [this]() { emit CompareDocumentsTriggered(); }));
    layout->addWidget(CreateToolButton(tr("Batch Center"), tr("Automate bulk jobs with profiles"), IconType::Batch, [this]() { emit BatchCenterTriggered(); }));

    layout->addStretch();
    scroll->setWidget(content);
    return scroll;
}

QPushButton* ContextInspectorPanel::CreateToolButton(
    const QString& title,
    const QString& desc,
    IconType icon,
    const std::function<void()>& onClick
) {
    auto* btn = new QPushButton(this);
    btn->setStyleSheet(
        "QPushButton {"
        "  background-color: #f8fafc;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  text-align: left;"
        "}"
        "QPushButton:hover {"
        "  background-color: #f1f5f9;"
        "  border-color: #3b82f6;"
        "}"
    );

    auto* layout = new QHBoxLayout(btn);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* iconLabel = new QLabel(btn);
    iconLabel->setPixmap(VectorIcons::CreatePixmap(icon, 20, QColor(37, 99, 235)));

    auto* textCol = new QVBoxLayout();
    textCol->setContentsMargins(0, 0, 0, 0);
    textCol->setSpacing(1);

    auto* titleLabel = new QLabel(QString("<b>%1</b>").arg(title), btn);
    titleLabel->setStyleSheet("color: #1e293b; font-size: 11px;");

    auto* descLabel = new QLabel(desc, btn);
    descLabel->setStyleSheet("color: #64748b; font-size: 9px;");

    textCol->addWidget(titleLabel);
    textCol->addWidget(descLabel);

    layout->addWidget(iconLabel);
    layout->addLayout(textCol, 1);

    connect(btn, &QPushButton::clicked, this, onClick);
    return btn;
}

void ContextInspectorPanel::SetDocument(std::shared_ptr<domain::DocumentModel> doc) {
    m_doc = std::move(doc);
    if (!m_doc || !m_doc->IsValid()) {
        m_propFileName->setText("-");
        m_propFileSize->setText("-");
        m_propPageCount->setText("-");
        m_propPageDimensions->setText("-");
        return;
    }

    QFileInfo fi(QString::fromStdString(m_doc->GetFilePath().string()));
    m_propFileName->setText(fi.fileName());
    m_propFileSize->setText(QString("%1 KB").arg(fi.size() / 1024));
    m_propPageCount->setText(QString("%1 pages").arg(m_doc->GetPageCount()));

    UpdatePageContext(0, 1.0);
}

void ContextInspectorPanel::UpdatePageContext(int currentPage, double zoomScale) {
    if (m_doc && m_doc->IsValid() && currentPage >= 0 && currentPage < m_doc->GetPageCount()) {
        auto sz = m_doc->GetPageSize(currentPage);
        double wPt = sz.width;
        double hPt = sz.height;
        double wIn = wPt / 72.0;
        double hIn = hPt / 72.0;
        m_propPageDimensions->setText(QString("%1 × %2 pt\n(%3 × %4 in)")
                                      .arg(static_cast<int>(wPt))
                                      .arg(static_cast<int>(hPt))
                                      .arg(wIn, 0, 'f', 1)
                                      .arg(hIn, 0, 'f', 1));
    }
    int percent = static_cast<int>(std::round(zoomScale * 100.0));
    m_propZoomLevel->setText(QString("%1%").arg(percent));
}

} // namespace pdfinity::ui
