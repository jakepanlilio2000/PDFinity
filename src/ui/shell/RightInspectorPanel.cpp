// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RightInspectorPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QColorDialog>
#include <QGroupBox>

namespace pdfinity::ui {

RightInspectorPanel::RightInspectorPanel(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(260);
    setMaximumWidth(360);
    resize(300, height());
    SetupUi();
}

void RightInspectorPanel::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setStyleSheet(
        "QWidget {"
        "  background-color: #27292E;"
        "  border-left: 1px solid #34373E;"
        "  color: #F4F4F5;"
        "}"
    );

    // 1. Header with title and close button
    auto* header = new QWidget(this);
    header->setStyleSheet("background-color: #1E2024; border-bottom: 1px solid #34373E;");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 8, 8, 8);

    m_headerTitle = new QLabel(tr("<b>All Tools Palette</b>"), header);
    m_headerTitle->setStyleSheet("color: #F4F4F5; font-size: 12px; border: none;");

    auto* closeBtn = new QPushButton("✕", header);
    closeBtn->setFixedSize(20, 20);
    closeBtn->setStyleSheet(
        "QPushButton { border: none; background: transparent; color: #A1A1AA; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { color: #DC2626; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &RightInspectorPanel::CloseRequested);

    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    mainLayout->addWidget(header);

    // 2. Stacked Modes: 0 = All Tools, 1 = Selection Properties
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(CreateAllToolsWidget());
    m_stack->addWidget(CreateSelectionPropertiesWidget());
    mainLayout->addWidget(m_stack);
}

QWidget* RightInspectorPanel::CreateAllToolsWidget() {
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    layout->addWidget(CreateToolCardButton(
        tr("Organize Pages"),
        tr("Rearrange, extract, rotate, and delete pages"),
        IconType::Organize,
        [this]() { emit OrganizePagesTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Edit Content"),
        tr("Modify text blocks and vector image elements"),
        IconType::ContentEdit,
        [this]() { emit ContentEditorTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Merge Documents"),
        tr("Combine multiple PDF files into one"),
        IconType::Merge,
        [this]() { emit MergeDocumentsTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Split Document"),
        tr("Separate document by page ranges"),
        IconType::Split,
        [this]() { emit SplitDocumentTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Fill & Sign"),
        tr("Apply cryptographic and visual signatures"),
        IconType::DigitalSign,
        [this]() { emit DigitalSignTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Protect & Redact"),
        tr("Permanently redact text, sanitize, and encrypt"),
        IconType::Redaction,
        [this]() { emit RedactionStudioTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("OCR Studio"),
        tr("Recognize text from scanned pages"),
        IconType::Ocr,
        [this]() { emit OcrStudioTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Preflight & Compliance"),
        tr("Verify ISO 32000 and PDF/A standards"),
        IconType::Properties,
        [this]() { emit PreflightTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Convert Document"),
        tr("Export to DOCX, XLSX, TXT, and images"),
        IconType::Optimize,
        [this]() { emit ConversionCenterTriggered(); }
    ));

    layout->addWidget(CreateToolCardButton(
        tr("Batch Center"),
        tr("Automated multi-file processing queue"),
        IconType::Batch,
        [this]() { emit BatchCenterTriggered(); }
    ));

    layout->addStretch();
    scrollArea->setWidget(container);
    return scrollArea;
}

QWidget* RightInspectorPanel::CreateSelectionPropertiesWidget() {
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    // Selection Object Type Badge
    m_selectionTypeLabel = new QLabel(tr("Annotation Properties"), container);
    m_selectionTypeLabel->setStyleSheet(
        "background: rgba(220, 38, 38, 0.15); color: #DC2626; border: 1px solid #DC2626; "
        "border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px;"
    );
    layout->addWidget(m_selectionTypeLabel);

    // Geometry Group
    auto* geomGroup = new QGroupBox(tr("Boundary Geometry (pt)"), container);
    geomGroup->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; color: #A1A1AA; border: 1px solid #34373E; border-radius: 4px; margin-top: 6px; padding-top: 10px; }");
    auto* geomGrid = new QGridLayout(geomGroup);
    geomGrid->setContentsMargins(8, 8, 8, 8);
    geomGrid->setSpacing(6);

    const QString spinStyle = "QDoubleSpinBox { background: #1E2024; border: 1px solid #34373E; border-radius: 3px; color: #F4F4F5; padding: 2px; }";

    m_spinX = new QDoubleSpinBox(geomGroup);
    m_spinX->setRange(0.0, 5000.0);
    m_spinX->setStyleSheet(spinStyle);
    m_spinY = new QDoubleSpinBox(geomGroup);
    m_spinY->setRange(0.0, 5000.0);
    m_spinY->setStyleSheet(spinStyle);
    m_spinW = new QDoubleSpinBox(geomGroup);
    m_spinW->setRange(0.0, 5000.0);
    m_spinW->setStyleSheet(spinStyle);
    m_spinH = new QDoubleSpinBox(geomGroup);
    m_spinH->setRange(0.0, 5000.0);
    m_spinH->setStyleSheet(spinStyle);

    geomGrid->addWidget(new QLabel("X:"), 0, 0);
    geomGrid->addWidget(m_spinX, 0, 1);
    geomGrid->addWidget(new QLabel("Y:"), 0, 2);
    geomGrid->addWidget(m_spinY, 0, 3);
    geomGrid->addWidget(new QLabel("W:"), 1, 0);
    geomGrid->addWidget(m_spinW, 1, 1);
    geomGrid->addWidget(new QLabel("H:"), 1, 2);
    geomGrid->addWidget(m_spinH, 1, 3);
    layout->addWidget(geomGroup);

    // Appearance Group
    auto* appGroup = new QGroupBox(tr("Appearance & Stroke"), container);
    appGroup->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; color: #A1A1AA; border: 1px solid #34373E; border-radius: 4px; margin-top: 6px; padding-top: 10px; }");
    auto* appLayout = new QVBoxLayout(appGroup);
    appLayout->setContentsMargins(8, 8, 8, 8);
    appLayout->setSpacing(8);

    // Stroke Color Picker
    auto* strokeRow = new QHBoxLayout();
    strokeRow->addWidget(new QLabel(tr("Stroke Color:")));
    m_strokeColorBtn = new QPushButton(tr("Select Color..."), appGroup);
    m_strokeColorBtn->setStyleSheet(QString("background-color: %1; color: #FFFFFF; border-radius: 3px; padding: 4px;").arg(m_currentStrokeColor.name()));
    connect(m_strokeColorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_currentStrokeColor, this, tr("Select Stroke Color"));
        if (c.isValid()) {
            m_currentStrokeColor = c;
            m_strokeColorBtn->setStyleSheet(QString("background-color: %1; color: #FFFFFF; border-radius: 3px; padding: 4px;").arg(c.name()));
            emit StrokeColorChanged(c);
        }
    });
    strokeRow->addWidget(m_strokeColorBtn);
    appLayout->addLayout(strokeRow);

    // Fill Color Picker
    auto* fillRow = new QHBoxLayout();
    fillRow->addWidget(new QLabel(tr("Fill Color:")));
    m_fillColorBtn = new QPushButton(tr("Select Fill..."), appGroup);
    m_fillColorBtn->setStyleSheet("background-color: #1E2024; color: #A1A1AA; border-radius: 3px; padding: 4px;");
    connect(m_fillColorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_currentFillColor, this, tr("Select Fill Color"));
        if (c.isValid()) {
            m_currentFillColor = c;
            m_fillColorBtn->setStyleSheet(QString("background-color: %1; color: #FFFFFF; border-radius: 3px; padding: 4px;").arg(c.name()));
            emit FillColorChanged(c);
        }
    });
    fillRow->addWidget(m_fillColorBtn);
    appLayout->addLayout(fillRow);

    // Stroke Width
    auto* widthRow = new QHBoxLayout();
    widthRow->addWidget(new QLabel(tr("Stroke Width:")));
    m_spinStrokeWidth = new QDoubleSpinBox(appGroup);
    m_spinStrokeWidth->setRange(0.5, 30.0);
    m_spinStrokeWidth->setValue(2.0);
    m_spinStrokeWidth->setSuffix(" pt");
    m_spinStrokeWidth->setStyleSheet(spinStyle);
    connect(m_spinStrokeWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RightInspectorPanel::StrokeWidthChanged);
    widthRow->addWidget(m_spinStrokeWidth);
    appLayout->addLayout(widthRow);

    // Opacity
    auto* opacityRow = new QHBoxLayout();
    opacityRow->addWidget(new QLabel(tr("Opacity:")));
    m_opacitySlider = new QSlider(Qt::Horizontal, appGroup);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(100);
    m_opacityLabel = new QLabel("100%", appGroup);
    m_opacityLabel->setFixedWidth(36);
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int val) {
        m_opacityLabel->setText(QString::number(val) + "%");
        emit OpacityChanged(val / 100.0);
    });
    opacityRow->addWidget(m_opacitySlider);
    opacityRow->addWidget(m_opacityLabel);
    appLayout->addLayout(opacityRow);

    layout->addWidget(appGroup);

    // Metadata Group
    auto* metaGroup = new QGroupBox(tr("Author & Annotation Content"), container);
    metaGroup->setStyleSheet("QGroupBox { font-size: 11px; font-weight: bold; color: #A1A1AA; border: 1px solid #34373E; border-radius: 4px; margin-top: 6px; padding-top: 10px; }");
    auto* metaLayout = new QVBoxLayout(metaGroup);
    metaLayout->setContentsMargins(8, 8, 8, 8);
    metaLayout->setSpacing(6);

    const QString editStyle = "QLineEdit { background: #1E2024; border: 1px solid #34373E; border-radius: 3px; color: #F4F4F5; padding: 4px; }";

    metaLayout->addWidget(new QLabel(tr("Author:"), metaGroup));
    m_authorEdit = new QLineEdit("PDFinity User", metaGroup);
    m_authorEdit->setStyleSheet(editStyle);
    metaLayout->addWidget(m_authorEdit);

    metaLayout->addWidget(new QLabel(tr("Note Contents:"), metaGroup));
    m_contentsEdit = new QLineEdit(metaGroup);
    m_contentsEdit->setStyleSheet(editStyle);
    metaLayout->addWidget(m_contentsEdit);

    layout->addWidget(metaGroup);

    // Actions
    auto* deleteBtn = new QPushButton(tr("Delete Selection"), container);
    deleteBtn->setStyleSheet(
        "QPushButton { background: #DC2626; color: #FFFFFF; font-weight: bold; border-radius: 4px; padding: 8px; border: none; }"
        "QPushButton:hover { background: #B91C1C; }"
    );
    connect(deleteBtn, &QPushButton::clicked, this, &RightInspectorPanel::DeleteSelectionTriggered);
    layout->addWidget(deleteBtn);

    layout->addStretch();
    scrollArea->setWidget(container);
    return scrollArea;
}

QPushButton* RightInspectorPanel::CreateToolCardButton(
    const QString& title,
    const QString& desc,
    IconType icon,
    const std::function<void()>& onClick
) {
    auto* btn = new QPushButton();
    btn->setIcon(VectorIcons::GetIcon(icon));
    btn->setIconSize(QSize(22, 22));
    btn->setText(QString(" %1\n %2").arg(title, desc));
    btn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1E2024;"
        "  border: 1px solid #34373E;"
        "  border-radius: 6px;"
        "  padding: 8px 10px;"
        "  text-align: left;"
        "  color: #F4F4F5;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  border-color: #DC2626;"
        "  background-color: rgba(220, 38, 38, 0.08);"
        "}"
    );
    connect(btn, &QPushButton::clicked, this, onClick);
    return btn;
}

void RightInspectorPanel::SetDocument(std::shared_ptr<domain::DocumentModel> doc) {
    m_doc = std::move(doc);
}

void RightInspectorPanel::UpdatePageContext(int /*currentPage*/, double /*zoomScale*/) {
    // Dynamically update context labels if present
}

void RightInspectorPanel::SetSelectionProperties(const SelectionPropertiesInfo& info) {
    m_selectionTypeLabel->setText(info.objectType);
    m_spinX->setValue(info.x);
    m_spinY->setValue(info.y);
    m_spinW->setValue(info.width);
    m_spinH->setValue(info.height);
    m_currentStrokeColor = info.strokeColor;
    m_currentFillColor = info.fillColor;
    m_strokeColorBtn->setStyleSheet(QString("background-color: %1; color: #FFFFFF; border-radius: 3px; padding: 4px;").arg(info.strokeColor.name()));
    m_spinStrokeWidth->setValue(info.strokeWidth);
    m_opacitySlider->setValue(static_cast<int>(info.opacity * 100));
    m_opacityLabel->setText(QString::number(static_cast<int>(info.opacity * 100)) + "%");
    m_authorEdit->setText(info.author);
    m_contentsEdit->setText(info.contents);

    SetSelectionMode();
}

void RightInspectorPanel::ClearSelection() {
    SetAllToolsMode();
}

void RightInspectorPanel::SetAllToolsMode() {
    m_headerTitle->setText(tr("<b>All Tools Palette</b>"));
    m_stack->setCurrentIndex(0);
}

void RightInspectorPanel::SetSelectionMode() {
    m_headerTitle->setText(tr("<b>Selection Properties</b>"));
    m_stack->setCurrentIndex(1);
}

} // namespace pdfinity::ui