// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "WorkflowRibbon.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

namespace pdfinity::ui {

WorkflowRibbon::WorkflowRibbon(QWidget* parent) : QWidget(parent) {
    SetupUi();
}

void WorkflowRibbon::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_modeTabs = new QTabWidget(this);
    m_modeTabs->setDocumentMode(true);
    m_modeTabs->setStyleSheet(
        "QTabWidget::pane { border-top: 1px solid #34373E; background-color: #222428; }"
        "QTabBar::tab {"
        "  background: transparent; color: #A1A1AA; padding: 6px 14px; font-size: 11px; font-weight: 600;"
        "  border: none; border-bottom: 2px solid transparent;"
        "}"
        "QTabBar::tab:hover { color: #F4F4F5; background-color: rgba(255, 255, 255, 0.04); }"
        "QTabBar::tab:selected { color: #DC2626; border-bottom: 2px solid #DC2626; font-weight: 700; }"
    );

    // 8 Primary Mode Tabs
    m_modeTabs->addTab(CreateAllToolsStrip(), tr("All Tools"));
    m_modeTabs->addTab(CreateViewStrip(), tr("View"));
    m_modeTabs->addTab(CreateCommentStrip(), tr("Comment"));
    m_modeTabs->addTab(CreateEditStrip(), tr("Edit"));
    m_modeTabs->addTab(CreateOrganizeStrip(), tr("Organize"));
    m_modeTabs->addTab(CreateFillSignStrip(), tr("Fill & Sign"));
    m_modeTabs->addTab(CreateProtectRedactStrip(), tr("Protect & Redact"));
    m_modeTabs->addTab(CreateConvertOcrStrip(), tr("Convert & OCR"));

    // Persistent Right Quick-Access Strip
    m_modeTabs->setCornerWidget(CreateQuickAccessStrip(), Qt::TopRightCorner);

    mainLayout->addWidget(m_modeTabs);
}

void WorkflowRibbon::SetActiveDocumentAvailable(bool available) {
    for (auto* btn : m_docDependentButtons) {
        if (btn) btn->setEnabled(available);
    }
}

void WorkflowRibbon::SetActiveMode(int index) {
    if (m_modeTabs && index >= 0 && index < m_modeTabs->count()) {
        m_modeTabs->setCurrentIndex(index);
    }
}

QFrame* WorkflowRibbon::CreateVerticalDivider() {
    auto* line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: #34373E; margin: 4px 6px;");
    return line;
}

QToolButton* WorkflowRibbon::CreateRibbonButton(
    const QString& label,
    IconType icon,
    const std::function<void()>& onClick,
    const QString& tooltip,
    bool isLarge
) {
    auto* btn = new QToolButton();
    btn->setText(label);
    btn->setIcon(VectorIcons::GetIcon(icon));
    btn->setToolTip(tooltip.isEmpty() ? label : tooltip);
    btn->setCursor(Qt::PointingHandCursor);

    if (isLarge) {
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setIconSize(QSize(20, 20));
        btn->setStyleSheet(
            "QToolButton {"
            "  color: #F4F4F5; font-size: 11px; font-weight: 500; border: 1px solid transparent; border-radius: 4px;"
            "  padding: 4px 8px; min-width: 50px; background: transparent;"
            "}"
            "QToolButton:hover { background-color: #2D3036; border-color: #34373E; }"
            "QToolButton:pressed { background-color: rgba(220, 38, 38, 0.15); border-color: #DC2626; }"
            "QToolButton:disabled { color: #52525B; }"
        );
    } else {
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(16, 16));
        btn->setStyleSheet(
            "QToolButton {"
            "  color: #F4F4F5; font-size: 11px; font-weight: 500; border: 1px solid transparent; border-radius: 4px;"
            "  padding: 3px 6px; background: transparent;"
            "}"
            "QToolButton:hover { background-color: #2D3036; border-color: #34373E; }"
            "QToolButton:pressed { background-color: rgba(220, 38, 38, 0.15); border-color: #DC2626; }"
            "QToolButton:disabled { color: #52525B; }"
        );
    }

    if (onClick) {
        connect(btn, &QToolButton::clicked, this, onClick);
    }

    return btn;
}

QWidget* WorkflowRibbon::CreateQuickAccessStrip() {
    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(4, 2, 8, 2);
    layout->setSpacing(2);

    auto* homeBtn = CreateRibbonButton(tr("Home"), IconType::Home, [this]() { emit HomeClicked(); }, tr("Go to Home Dashboard"), false);
    auto* openBtn = CreateRibbonButton(tr("Open"), IconType::Open, [this]() { emit OpenClicked(); }, tr("Open PDF Document (Ctrl+O)"), false);
    layout->addWidget(homeBtn);
    layout->addWidget(openBtn);
    layout->addWidget(CreateVerticalDivider());

    auto* undoBtn = CreateRibbonButton("", IconType::Undo, [this]() { emit UndoClicked(); }, tr("Undo (Ctrl+Z)"), false);
    auto* redoBtn = CreateRibbonButton("", IconType::Redo, [this]() { emit RedoClicked(); }, tr("Redo (Ctrl+Y)"), false);
    auto* saveBtn = CreateRibbonButton("", IconType::Save, [this]() { emit SaveClicked(); }, tr("Save Document (Ctrl+S)"), false);
    auto* printBtn = CreateRibbonButton("", IconType::Print, [this]() { emit PrintClicked(); }, tr("Print Document (Ctrl+P)"), false);
    auto* shareBtn = CreateRibbonButton("", IconType::Share, [this]() { emit ShareClicked(); }, tr("Share / Export"), false);

    m_docDependentButtons.push_back(undoBtn);
    m_docDependentButtons.push_back(redoBtn);
    m_docDependentButtons.push_back(saveBtn);
    m_docDependentButtons.push_back(printBtn);
    m_docDependentButtons.push_back(shareBtn);

    layout->addWidget(undoBtn);
    layout->addWidget(redoBtn);
    layout->addWidget(saveBtn);
    layout->addWidget(printBtn);
    layout->addWidget(shareBtn);

    return container;
}

QWidget* WorkflowRibbon::CreateAllToolsStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    auto* organizeBtn = CreateRibbonButton(tr("Organize Pages"), IconType::Organize, [this]() { emit OrganizePagesClicked(); }, "", true);
    auto* editBtn = CreateRibbonButton(tr("Edit Content"), IconType::ContentEdit, [this]() { emit ToggleContentEditorClicked(); }, "", true);
    auto* mergeBtn = CreateRibbonButton(tr("Merge Files"), IconType::Merge, [this]() { emit MergeDocumentsClicked(); }, "", true);
    auto* splitBtn = CreateRibbonButton(tr("Split PDF"), IconType::Split, [this]() { emit SplitDocumentClicked(); }, "", true);
    auto* signBtn = CreateRibbonButton(tr("Digital Sign"), IconType::DigitalSign, [this]() { emit DigitalSignClicked(); }, "", true);
    auto* redactBtn = CreateRibbonButton(tr("Redact & Sanitize"), IconType::Redaction, [this]() { emit ToggleRedactionStudioClicked(); }, "", true);
    auto* ocrBtn = CreateRibbonButton(tr("OCR Studio"), IconType::Ocr, [this]() { emit OcrStudioClicked(); }, "", true);
    auto* convertBtn = CreateRibbonButton(tr("Convert Export"), IconType::Optimize, [this]() { emit ConversionCenterClicked(); }, "", true);
    auto* preflightBtn = CreateRibbonButton(tr("Preflight Standards"), IconType::Shield, [this]() { emit PreflightClicked(); }, "", true);

    m_docDependentButtons.push_back(organizeBtn);
    m_docDependentButtons.push_back(editBtn);
    m_docDependentButtons.push_back(signBtn);
    m_docDependentButtons.push_back(redactBtn);
    m_docDependentButtons.push_back(ocrBtn);
    m_docDependentButtons.push_back(convertBtn);
    m_docDependentButtons.push_back(preflightBtn);

    layout->addWidget(organizeBtn);
    layout->addWidget(editBtn);
    layout->addWidget(mergeBtn);
    layout->addWidget(splitBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(signBtn);
    layout->addWidget(redactBtn);
    layout->addWidget(ocrBtn);
    layout->addWidget(convertBtn);
    layout->addWidget(preflightBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateViewStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* panBtn = CreateRibbonButton(tr("Hand Pan"), IconType::Properties, [this]() { emit ViewPanClicked(); }, tr("Hand Pan tool"), true);
    auto* selectBtn = CreateRibbonButton(tr("Text Select"), IconType::Search, [this]() { emit ViewSelectClicked(); }, tr("Select Text tool"), true);
    layout->addWidget(panBtn);
    layout->addWidget(selectBtn);
    layout->addWidget(CreateVerticalDivider());

    auto* zoomIn = CreateRibbonButton(tr("Zoom In"), IconType::ZoomIn, [this]() { emit ViewZoomInClicked(); }, tr("Zoom In (+)"), false);
    auto* zoomOut = CreateRibbonButton(tr("Zoom Out"), IconType::ZoomOut, [this]() { emit ViewZoomOutClicked(); }, tr("Zoom Out (-)"), false);
    auto* zoom100 = CreateRibbonButton(tr("100%"), IconType::Properties, [this]() { emit ViewZoom100Clicked(); }, tr("Actual Size"), false);
    auto* fitWidth = CreateRibbonButton(tr("Fit Width"), IconType::FitWidth, [this]() { emit ViewFitWidthClicked(); }, tr("Fit to Width"), false);
    auto* fitPage = CreateRibbonButton(tr("Fit Page"), IconType::FitPage, [this]() { emit ViewFitPageClicked(); }, tr("Fit to Page"), false);
    auto* rotate = CreateRibbonButton(tr("Rotate 90°"), IconType::Rotate, [this]() { emit ViewRotateClicked(); }, tr("Rotate Clockwise"), false);
    auto* fullScreen = CreateRibbonButton(tr("Focus Mode"), IconType::Shield, [this]() { emit ViewFullscreenClicked(); }, tr("Full Screen (F11)"), false);

    m_docDependentButtons.push_back(zoomIn);
    m_docDependentButtons.push_back(zoomOut);
    m_docDependentButtons.push_back(zoom100);
    m_docDependentButtons.push_back(fitWidth);
    m_docDependentButtons.push_back(fitPage);
    m_docDependentButtons.push_back(rotate);
    m_docDependentButtons.push_back(fullScreen);

    layout->addWidget(zoomIn);
    layout->addWidget(zoomOut);
    layout->addWidget(zoom100);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(fitWidth);
    layout->addWidget(fitPage);
    layout->addWidget(rotate);
    layout->addWidget(fullScreen);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateCommentStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* annotStudioBtn = CreateRibbonButton(tr("Markup Studio"), IconType::Annotations, [this]() { emit ToggleAnnotationStudioClicked(); }, tr("Toggle Annotation Toolbar"), true);
    m_docDependentButtons.push_back(annotStudioBtn);
    layout->addWidget(annotStudioBtn);
    layout->addWidget(CreateVerticalDivider());

    auto* highlightBtn = CreateRibbonButton(tr("Highlight"), IconType::Annotations, [this]() { emit CommentHighlightClicked(); }, tr("Text Highlight"), false);
    auto* underlineBtn = CreateRibbonButton(tr("Underline"), IconType::Annotations, [this]() { emit CommentUnderlineClicked(); }, tr("Text Underline"), false);
    auto* strikeoutBtn = CreateRibbonButton(tr("Strikeout"), IconType::Annotations, [this]() { emit CommentStrikeoutClicked(); }, tr("Text Strikeout"), false);
    auto* noteBtn = CreateRibbonButton(tr("Sticky Note"), IconType::Annotations, [this]() { emit CommentNoteClicked(); }, tr("Add Sticky Note"), false);
    auto* drawBtn = CreateRibbonButton(tr("Freehand Draw"), IconType::Annotations, [this]() { emit CommentDrawClicked(); }, tr("Freehand Ink Drawing"), false);
    auto* shapesBtn = CreateRibbonButton(tr("Shapes"), IconType::Annotations, [this]() { emit CommentShapesClicked(); }, tr("Geometric Shapes"), false);
    auto* calloutBtn = CreateRibbonButton(tr("Callout"), IconType::Annotations, [this]() { emit CommentCalloutClicked(); }, tr("Text Callout Box"), false);
    auto* stampBtn = CreateRibbonButton(tr("Stamp"), IconType::VisualSign, [this]() { emit CommentStampClicked(); }, tr("Approval / Review Stamp"), false);

    m_docDependentButtons.push_back(highlightBtn);
    m_docDependentButtons.push_back(underlineBtn);
    m_docDependentButtons.push_back(strikeoutBtn);
    m_docDependentButtons.push_back(noteBtn);
    m_docDependentButtons.push_back(drawBtn);
    m_docDependentButtons.push_back(shapesBtn);
    m_docDependentButtons.push_back(calloutBtn);
    m_docDependentButtons.push_back(stampBtn);

    layout->addWidget(highlightBtn);
    layout->addWidget(underlineBtn);
    layout->addWidget(strikeoutBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(noteBtn);
    layout->addWidget(drawBtn);
    layout->addWidget(shapesBtn);
    layout->addWidget(calloutBtn);
    layout->addWidget(stampBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateEditStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* editorBtn = CreateRibbonButton(tr("Content Editor"), IconType::ContentEdit, [this]() { emit ToggleContentEditorClicked(); }, tr("Interactive Text & Image Editor"), true);
    auto* objectInspBtn = CreateRibbonButton(tr("Object Streams"), IconType::Properties, [this]() { emit ObjectInspectorClicked(); }, tr("Low-Level Object Inspector"), true);
    auto* formInspBtn = CreateRibbonButton(tr("Form Fields"), IconType::Forms, [this]() { emit FormInspectorClicked(); }, tr("AcroForm Field Inspector"), true);

    m_docDependentButtons.push_back(editorBtn);
    m_docDependentButtons.push_back(objectInspBtn);
    m_docDependentButtons.push_back(formInspBtn);

    layout->addWidget(editorBtn);
    layout->addWidget(objectInspBtn);
    layout->addWidget(formInspBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateOrganizeStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* orgPagesBtn = CreateRibbonButton(tr("Organize Pages"), IconType::Organize, [this]() { emit OrganizePagesClicked(); }, tr("Reorder, Rotate, and Extract Pages"), true);
    auto* mergeBtn = CreateRibbonButton(tr("Merge PDFs"), IconType::Merge, [this]() { emit MergeDocumentsClicked(); }, tr("Combine Multiple Documents"), true);
    auto* splitBtn = CreateRibbonButton(tr("Split PDF"), IconType::Split, [this]() { emit SplitDocumentClicked(); }, tr("Divide into Multiple Documents"), true);
    auto* rotateBtn = CreateRibbonButton(tr("Rotate CW"), IconType::Rotate, [this]() { emit RotateClicked(); }, tr("Rotate Active Page 90°"), true);

    m_docDependentButtons.push_back(orgPagesBtn);
    m_docDependentButtons.push_back(rotateBtn);

    layout->addWidget(orgPagesBtn);
    layout->addWidget(mergeBtn);
    layout->addWidget(splitBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(rotateBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateFillSignStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* formBtn = CreateRibbonButton(tr("Fill Forms"), IconType::Forms, [this]() { emit FormInspectorClicked(); }, tr("Fill Form Fields"), true);
    auto* digiSignBtn = CreateRibbonButton(tr("Digital Sign"), IconType::DigitalSign, [this]() { emit DigitalSignClicked(); }, tr("PKCS#7 Detached Cryptographic Signature"), true);
    auto* visualSignBtn = CreateRibbonButton(tr("Visual Stamp"), IconType::VisualSign, [this]() { emit VisualSignClicked(); }, tr("Place Handwritten Signature Stamp"), true);
    auto* validateBtn = CreateRibbonButton(tr("Validate Signatures"), IconType::Shield, [this]() { emit ValidateSignaturesClicked(); }, tr("Validate Integrity & Certificates"), true);
    auto* certVaultBtn = CreateRibbonButton(tr("Certificates Vault"), IconType::Properties, [this]() { emit CertificateManagerClicked(); }, tr("Manage Cryptographic Certificates"), true);

    m_docDependentButtons.push_back(formBtn);
    m_docDependentButtons.push_back(digiSignBtn);
    m_docDependentButtons.push_back(visualSignBtn);
    m_docDependentButtons.push_back(validateBtn);

    layout->addWidget(formBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(digiSignBtn);
    layout->addWidget(visualSignBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(validateBtn);
    layout->addWidget(certVaultBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateProtectRedactStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* redactStudioBtn = CreateRibbonButton(tr("Redaction Studio"), IconType::Redaction, [this]() { emit ToggleRedactionStudioClicked(); }, tr("Interactive Redaction Toolbar"), true);
    auto* applyRedactBtn = CreateRibbonButton(tr("Apply Redactions"), IconType::Redaction, [this]() { emit ApplyRedactionsClicked(); }, tr("Burn In Redactions Irreversibly"), true);
    auto* sanitizeBtn = CreateRibbonButton(tr("Sanitize Document"), IconType::Sanitize, [this]() { emit SanitizeClicked(); }, tr("Scrub Metadata, Bookmarks, and Attachments"), true);
    auto* encryptBtn = CreateRibbonButton(tr("Encrypt & Protect"), IconType::Shield, [this]() { emit EncryptClicked(); }, tr("Set Passwords and Permissions"), true);

    m_docDependentButtons.push_back(redactStudioBtn);
    m_docDependentButtons.push_back(applyRedactBtn);
    m_docDependentButtons.push_back(sanitizeBtn);
    m_docDependentButtons.push_back(encryptBtn);

    layout->addWidget(redactStudioBtn);
    layout->addWidget(applyRedactBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(sanitizeBtn);
    layout->addWidget(encryptBtn);
    layout->addStretch();

    return pane;
}

QWidget* WorkflowRibbon::CreateConvertOcrStrip() {
    auto* pane = new QWidget();
    auto* layout = new QHBoxLayout(pane);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);

    auto* ocrBtn = CreateRibbonButton(tr("OCR Studio"), IconType::Ocr, [this]() { emit OcrStudioClicked(); }, tr("Optical Character Recognition"), true);
    auto* convertBtn = CreateRibbonButton(tr("Conversion Center"), IconType::Optimize, [this]() { emit ConversionCenterClicked(); }, tr("Convert PDF to Images, Text, Office"), true);
    auto* optBtn = CreateRibbonButton(tr("Optimizer"), IconType::Optimize, [this]() { emit OptimizerClicked(); }, tr("Compress and Optimize PDF Size"), true);
    auto* intelBtn = CreateRibbonButton(tr("Intelligence"), IconType::Properties, [this]() { emit DocumentIntelligenceClicked(); }, tr("Font, XMP, and Content Audit"), true);
    auto* compareBtn = CreateRibbonButton(tr("Compare PDFs"), IconType::Compare, [this]() { emit CompareDocumentsClicked(); }, tr("Visual Diff Between Revisions"), true);
    auto* preflightBtn = CreateRibbonButton(tr("PDF/A Preflight"), IconType::Shield, [this]() { emit PreflightClicked(); }, tr("ISO 19005 Compliance Verification"), true);
    auto* batchBtn = CreateRibbonButton(tr("Batch Center"), IconType::Batch, [this]() { emit BatchCenterClicked(); }, tr("Multi-File Automation Queue"), true);

    m_docDependentButtons.push_back(ocrBtn);
    m_docDependentButtons.push_back(convertBtn);
    m_docDependentButtons.push_back(optBtn);
    m_docDependentButtons.push_back(intelBtn);
    m_docDependentButtons.push_back(preflightBtn);

    layout->addWidget(ocrBtn);
    layout->addWidget(convertBtn);
    layout->addWidget(optBtn);
    layout->addWidget(CreateVerticalDivider());
    layout->addWidget(intelBtn);
    layout->addWidget(compareBtn);
    layout->addWidget(preflightBtn);
    layout->addWidget(batchBtn);
    layout->addStretch();

    return pane;
}

} // namespace pdfinity::ui
