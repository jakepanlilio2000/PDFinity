// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "WorkflowRibbon.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

namespace pdfinity::ui {

WorkflowRibbon::WorkflowRibbon(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void WorkflowRibbon::SetupUi() {
    setFixedHeight(84);
    setStyleSheet(
        "WorkflowRibbon {"
        "  background-color: #ffffff;"
        "  border-bottom: 1px solid #cbd5e1;"
        "}"
        "QTabWidget::pane {"
        "  border: none;"
        "  background-color: #ffffff;"
        "}"
        "QTabBar::tab {"
        "  background: #f8fafc;"
        "  border: none;"
        "  border-right: 1px solid #e2e8f0;"
        "  padding: 5px 16px;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "  color: #475569;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #ffffff;"
        "  border-bottom: 2px solid #DC2626;"
        "  color: #DC2626;"
        "  font-weight: 600;"
        "}"
        "QToolButton {"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  border-radius: 6px;"
        "  padding: 2px 8px;"
        "  font-size: 11px;"
        "  color: #1e293b;"
        "}"
        "QToolButton:hover {"
        "  background-color: #f1f5f9;"
        "  border-color: #cbd5e1;"
        "}"
        "QToolButton:pressed {"
        "  background-color: #e2e8f0;"
        "}"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_ribbonTabs = new QTabWidget(this);
    m_ribbonTabs->addTab(CreateHomeTab(), tr("Home & File"));
    m_ribbonTabs->addTab(CreateAssemblyTab(), tr("Assemble Pages"));
    m_ribbonTabs->addTab(CreateAnnotateTab(), tr("Review & Annotate"));
    m_ribbonTabs->addTab(CreateFormsEditTab(), tr("Forms & Edit"));
    m_ribbonTabs->addTab(CreateSecurityTab(), tr("Sign & Protect"));
    m_ribbonTabs->addTab(CreateToolsTab(), tr("Tools & Automate"));

    layout->addWidget(m_ribbonTabs);
}

QToolButton* WorkflowRibbon::CreateRibbonButton(
    const QString& label,
    IconType icon,
    const std::function<void()>& onClick,
    const QString& tooltip,
    bool isLarge
) {
    auto* btn = new QToolButton(this);
    btn->setIcon(VectorIcons::GetIcon(icon));
    btn->setText(label);
    if (!tooltip.isEmpty()) btn->setToolTip(tooltip);

    if (isLarge) {
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIconSize(QSize(22, 22));
    } else {
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(16, 16));
    }

    connect(btn, &QToolButton::clicked, this, onClick);
    return btn;
}

QWidget* WorkflowRibbon::CreateHomeTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(4);

    layout->addWidget(CreateRibbonButton(tr("Home"), IconType::Home, [this]() { emit HomeClicked(); }, tr("Open Home Dashboard")));
    layout->addWidget(CreateRibbonButton(tr("Open"), IconType::Open, [this]() { emit OpenClicked(); }, tr("Open Document (Ctrl+O)")));

    auto* saveBtn = CreateRibbonButton(tr("Save Copy"), IconType::Save, [this]() { emit SaveClicked(); }, tr("Save As Copy (Ctrl+S)"));
    m_docDependentButtons.push_back(saveBtn);
    layout->addWidget(saveBtn);

    auto* printBtn = CreateRibbonButton(tr("Print"), IconType::Print, [this]() { emit PrintClicked(); }, tr("Print Document (Ctrl+P)"));
    m_docDependentButtons.push_back(printBtn);
    layout->addWidget(printBtn);

    auto* shareBtn = CreateRibbonButton(tr("Share"), IconType::Share, [this]() { emit ShareClicked(); }, tr("Share Document"));
    m_docDependentButtons.push_back(shareBtn);
    layout->addWidget(shareBtn);

    auto* sep = new QFrame(pane);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color: #e2e8f0;");
    layout->addWidget(sep);

    auto* findBtn = CreateRibbonButton(tr("Find"), IconType::Search, [this]() { emit FindClicked(); }, tr("Find Text (Ctrl+F)"), false);
    m_docDependentButtons.push_back(findBtn);
    layout->addWidget(findBtn);

    auto* undoBtn = CreateRibbonButton(tr("Undo"), IconType::Undo, [this]() { emit UndoClicked(); }, tr("Undo (Ctrl+Z)"), false);
    m_docDependentButtons.push_back(undoBtn);
    layout->addWidget(undoBtn);

    auto* redoBtn = CreateRibbonButton(tr("Redo"), IconType::Redo, [this]() { emit RedoClicked(); }, tr("Redo (Ctrl+Y)"), false);
    m_docDependentButtons.push_back(redoBtn);
    layout->addWidget(redoBtn);

    layout->addStretch();
    return pane;
}

QWidget* WorkflowRibbon::CreateAssemblyTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    auto* orgBtn = CreateRibbonButton(tr("Organize"), IconType::Organize, [this]() { emit OrganizePagesClicked(); }, tr("Organize, reorder, rotate, and extract pages"));
    m_docDependentButtons.push_back(orgBtn);
    layout->addWidget(orgBtn);

    layout->addWidget(CreateRibbonButton(tr("Merge"), IconType::Merge, [this]() { emit MergeDocumentsClicked(); }, tr("Merge multiple PDF documents")));

    auto* splitBtn = CreateRibbonButton(tr("Split"), IconType::Split, [this]() { emit SplitDocumentClicked(); }, tr("Split document into multiple parts"));
    m_docDependentButtons.push_back(splitBtn);
    layout->addWidget(splitBtn);

    auto* rotBtn = CreateRibbonButton(tr("Rotate"), IconType::Rotate, [this]() { emit RotateClicked(); }, tr("Rotate current page 90° clockwise"));
    m_docDependentButtons.push_back(rotBtn);
    layout->addWidget(rotBtn);

    layout->addStretch();
    return pane;
}

QWidget* WorkflowRibbon::CreateAnnotateTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    auto* annotBtn = CreateRibbonButton(tr("Markup Studio"), IconType::Annotations, [this]() { emit ToggleAnnotationStudioClicked(); }, tr("Toggle annotation markup studio toolbar"));
    m_docDependentButtons.push_back(annotBtn);
    layout->addWidget(annotBtn);

    layout->addStretch();
    return pane;
}

QWidget* WorkflowRibbon::CreateFormsEditTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    auto* formBtn = CreateRibbonButton(tr("Forms Inspector"), IconType::Forms, [this]() { emit FormInspectorClicked(); }, tr("Inspect, fill, and exchange AcroForm field data"));
    m_docDependentButtons.push_back(formBtn);
    layout->addWidget(formBtn);

    auto* editBtn = CreateRibbonButton(tr("Edit Text"), IconType::ContentEdit, [this]() { emit ToggleContentEditorClicked(); }, tr("Edit native text runs and vector objects"));
    m_docDependentButtons.push_back(editBtn);
    layout->addWidget(editBtn);

    auto* objBtn = CreateRibbonButton(tr("Object Inspector"), IconType::Properties, [this]() { emit ObjectInspectorClicked(); }, tr("Inspect raw PDF stream objects and image assets"));
    m_docDependentButtons.push_back(objBtn);
    layout->addWidget(objBtn);

    layout->addStretch();
    return pane;
}

QWidget* WorkflowRibbon::CreateSecurityTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    auto* dSignBtn = CreateRibbonButton(tr("Digital Sign"), IconType::DigitalSign, [this]() { emit DigitalSignClicked(); }, tr("Apply cryptographic PKCS#7 digital signature"));
    m_docDependentButtons.push_back(dSignBtn);
    layout->addWidget(dSignBtn);

    auto* vSignBtn = CreateRibbonButton(tr("Visual Stamp"), IconType::VisualSign, [this]() { emit VisualSignClicked(); }, tr("Place visual signature or approval stamp"));
    m_docDependentButtons.push_back(vSignBtn);
    layout->addWidget(vSignBtn);

    auto* valBtn = CreateRibbonButton(tr("Validate"), IconType::Shield, [this]() { emit ValidateSignaturesClicked(); }, tr("Validate signatures and verify byte range integrity"));
    m_docDependentButtons.push_back(valBtn);
    layout->addWidget(valBtn);

    auto* redBtn = CreateRibbonButton(tr("Redaction Studio"), IconType::Redaction, [this]() { emit ToggleRedactionStudioClicked(); }, tr("Mark and burn-in permanent stream redaction"));
    m_docDependentButtons.push_back(redBtn);
    layout->addWidget(redBtn);

    auto* sanBtn = CreateRibbonButton(tr("Sanitize"), IconType::Sanitize, [this]() { emit SanitizeClicked(); }, tr("Scrub metadata, attachments, and hidden data"));
    m_docDependentButtons.push_back(sanBtn);
    layout->addWidget(sanBtn);

    layout->addWidget(CreateRibbonButton(tr("Certificates"), IconType::Properties, [this]() { emit CertificateManagerClicked(); }, tr("Manage digital identity certificates and keypairs")));

    layout->addStretch();
    return pane;
}

QWidget* WorkflowRibbon::CreateToolsTab() {
    auto* pane = new QWidget(m_ribbonTabs);
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    auto* ocrBtn = CreateRibbonButton(tr("OCR Studio"), IconType::Ocr, [this]() { emit OcrStudioClicked(); }, tr("Recognize text and generate searchable layers"));
    m_docDependentButtons.push_back(ocrBtn);
    layout->addWidget(ocrBtn);

    auto* convBtn = CreateRibbonButton(tr("Conversion"), IconType::Compare, [this]() { emit ConversionCenterClicked(); }, tr("Export to images, text, and HTML"));
    m_docDependentButtons.push_back(convBtn);
    layout->addWidget(convBtn);

    auto* optBtn = CreateRibbonButton(tr("Optimizer"), IconType::Optimize, [this]() { emit OptimizerClicked(); }, tr("Compress streams with optimization presets"));
    m_docDependentButtons.push_back(optBtn);
    layout->addWidget(optBtn);

    auto* intelBtn = CreateRibbonButton(tr("Intelligence"), IconType::Properties, [this]() { emit DocumentIntelligenceClicked(); }, tr("Deep document analysis and preflight inspection"));
    m_docDependentButtons.push_back(intelBtn);
    layout->addWidget(intelBtn);

    layout->addWidget(CreateRibbonButton(tr("Compare"), IconType::Compare, [this]() { emit CompareDocumentsClicked(); }, tr("Visual & semantic text comparison workspace")));
    layout->addWidget(CreateRibbonButton(tr("Batch Center"), IconType::Batch, [this]() { emit BatchCenterClicked(); }, tr("Automate bulk document processing with profiles")));
    layout->addWidget(CreateRibbonButton(tr("Settings"), IconType::Settings, [this]() { emit SettingsClicked(); }, tr("Application settings and configuration")));

    layout->addStretch();
    return pane;
}

void WorkflowRibbon::SetActiveDocumentAvailable(bool available) {
    for (auto* btn : m_docDependentButtons) {
        if (btn) btn->setEnabled(available);
    }
}

} // namespace pdfinity::ui
