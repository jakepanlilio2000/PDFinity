// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AnnotationToolbar.hpp"
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QPainter>

namespace pdfinity::ui {

AnnotationToolbar::AnnotationToolbar(QWidget* parent)
    : QToolBar(tr("Annotation Studio"), parent) {
    setObjectName("AnnotationToolbar");
    setMovable(false);
    SetupTools();
}

void AnnotationToolbar::SetupTools() {
    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    m_selectAction = addAction(tr("👆 Select"));
    m_selectAction->setCheckable(true);
    m_selectAction->setChecked(true);
    m_selectAction->setToolTip(tr("Select & Edit Annotation"));
    m_toolGroup->addAction(m_selectAction);

    addSeparator();

    m_highlightAction = addAction(tr("🖍️ Highlight"));
    m_highlightAction->setCheckable(true);
    m_highlightAction->setToolTip(tr("Highlight Text (Text Markup)"));
    m_toolGroup->addAction(m_highlightAction);

    m_underlineAction = addAction(tr("̲ Underline"));
    m_underlineAction->setCheckable(true);
    m_underlineAction->setToolTip(tr("Underline Text (Text Markup)"));
    m_toolGroup->addAction(m_underlineAction);

    m_strikeoutAction = addAction(tr("̶ Strikeout"));
    m_strikeoutAction->setCheckable(true);
    m_strikeoutAction->setToolTip(tr("Strikeout Text (Text Markup)"));
    m_toolGroup->addAction(m_strikeoutAction);

    m_noteAction = addAction(tr("💬 Sticky Note"));
    m_noteAction->setCheckable(true);
    m_noteAction->setToolTip(tr("Insert Sticky Note Comment"));
    m_toolGroup->addAction(m_noteAction);

    m_inkAction = addAction(tr("✏️ Draw"));
    m_inkAction->setCheckable(true);
    m_inkAction->setToolTip(tr("Freehand Ink Drawing"));
    m_toolGroup->addAction(m_inkAction);

    addSeparator();

    m_rectAction = addAction(tr("⬜ Rectangle"));
    m_rectAction->setCheckable(true);
    m_rectAction->setToolTip(tr("Draw Rectangle Shape"));
    m_toolGroup->addAction(m_rectAction);

    m_circleAction = addAction(tr("⚪ Circle"));
    m_circleAction->setCheckable(true);
    m_circleAction->setToolTip(tr("Draw Circle / Ellipse Shape"));
    m_toolGroup->addAction(m_circleAction);

    m_lineAction = addAction(tr("📏 Line"));
    m_lineAction->setCheckable(true);
    m_lineAction->setToolTip(tr("Draw Line"));
    m_toolGroup->addAction(m_lineAction);

    m_arrowAction = addAction(tr("➡️ Arrow"));
    m_arrowAction->setCheckable(true);
    m_arrowAction->setToolTip(tr("Draw Arrow"));
    m_toolGroup->addAction(m_arrowAction);

    m_stampAction = addAction(tr("🏷️ Stamp"));
    m_stampAction->setCheckable(true);
    m_stampAction->setToolTip(tr("Place Stamp Annotation"));
    m_toolGroup->addAction(m_stampAction);

    m_textAction = addAction(tr("🔤 Text Box"));
    m_textAction->setCheckable(true);
    m_textAction->setToolTip(tr("Insert FreeText Callout Box"));
    m_toolGroup->addAction(m_textAction);

    addSeparator();

    // Stamp selection dropdown
    m_stampCombo = new QComboBox(this);
    m_stampCombo->addItems({"APPROVED", "REJECTED", "DRAFT", "CONFIDENTIAL", "FINAL", "VOID"});
    m_stampCombo->setToolTip(tr("Select Stamp Template"));
    addWidget(m_stampCombo);

    addSeparator();

    // Color picker button
    m_colorBtn = new QPushButton(this);
    m_colorBtn->setFixedSize(26, 26);
    m_colorBtn->setToolTip(tr("Annotation Color & Opacity"));
    addWidget(m_colorBtn);

    // Stroke width spinbox
    addWidget(new QLabel(tr(" Width:"), this));
    m_widthSpin = new QDoubleSpinBox(this);
    m_widthSpin->setRange(0.5, 20.0);
    m_widthSpin->setSingleStep(0.5);
    m_widthSpin->setValue(2.0);
    m_widthSpin->setFixedWidth(60);
    m_widthSpin->setToolTip(tr("Stroke Width (Points)"));
    addWidget(m_widthSpin);

    addSeparator();

    // Undo / Redo / Delete
    m_undoAction = addAction(tr("↩️ Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);

    m_redoAction = addAction(tr("↪️ Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false);

    m_deleteAction = addAction(tr("🗑️ Delete"));
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setEnabled(false);

    // Connections
    connect(m_toolGroup, &QActionGroup::triggered, this, &AnnotationToolbar::OnToolTriggered);
    connect(m_colorBtn, &QPushButton::clicked, this, &AnnotationToolbar::OnPickColor);
    connect(m_widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnnotationToolbar::OnWidthChanged);
    connect(m_stampCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AnnotationToolbar::OnStampSelected);

    connect(m_undoAction, &QAction::triggered, this, &AnnotationToolbar::UndoRequested);
    connect(m_redoAction, &QAction::triggered, this, &AnnotationToolbar::RedoRequested);
    connect(m_deleteAction, &QAction::triggered, this, &AnnotationToolbar::DeleteRequested);

    UpdateColorIcon();
}

void AnnotationToolbar::UpdateColorIcon() {
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(m_currentColor);
    p.setPen(QPen(Qt::darkGray, 1));
    p.drawRoundedRect(1, 1, 18, 18, 4, 4);
    p.end();

    m_colorBtn->setIcon(QIcon(pix));
}

void AnnotationToolbar::OnToolTriggered(QAction* action) {
    if (action == m_selectAction) {
        m_activeTool = AnnotationToolMode::Select;
    } else if (action == m_highlightAction) {
        m_activeTool = AnnotationToolMode::Highlight;
        m_currentColor = QColor(255, 235, 59, 180); // Yellow
    } else if (action == m_underlineAction) {
        m_activeTool = AnnotationToolMode::Underline;
        m_currentColor = QColor(37, 99, 235, 255); // Blue
    } else if (action == m_strikeoutAction) {
        m_activeTool = AnnotationToolMode::Strikeout;
        m_currentColor = QColor(220, 38, 38, 255); // Red
    } else if (action == m_noteAction) {
        m_activeTool = AnnotationToolMode::StickyNote;
        m_currentColor = QColor(234, 179, 8, 255); // Amber
    } else if (action == m_inkAction) {
        m_activeTool = AnnotationToolMode::FreehandInk;
        m_currentColor = QColor(37, 99, 235, 255); // Blue
    } else if (action == m_rectAction) {
        m_activeTool = AnnotationToolMode::Rectangle;
        m_currentColor = QColor(220, 38, 38, 255); // Red
    } else if (action == m_circleAction) {
        m_activeTool = AnnotationToolMode::Circle;
        m_currentColor = QColor(22, 163, 74, 255); // Green
    } else if (action == m_lineAction) {
        m_activeTool = AnnotationToolMode::Line;
        m_currentColor = QColor(15, 23, 42, 255); // Slate
    } else if (action == m_arrowAction) {
        m_activeTool = AnnotationToolMode::Arrow;
        m_currentColor = QColor(220, 38, 38, 255); // Red
    } else if (action == m_stampAction) {
        m_activeTool = AnnotationToolMode::Stamp;
        m_currentColor = QColor(185, 28, 28, 255); // Red stamp
    } else if (action == m_textAction) {
        m_activeTool = AnnotationToolMode::FreeText;
        m_currentColor = QColor(15, 23, 42, 255); // Dark text
    }

    UpdateColorIcon();
    emit ToolChanged(m_activeTool);
    emit ColorChanged(m_currentColor);
}

void AnnotationToolbar::OnPickColor() {
    QColor c = QColorDialog::getColor(m_currentColor, this, tr("Select Annotation Color"), QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_currentColor = c;
        UpdateColorIcon();
        emit ColorChanged(m_currentColor);
    }
}

void AnnotationToolbar::OnWidthChanged(double val) {
    m_strokeWidth = static_cast<float>(val);
    emit StrokeWidthChanged(m_strokeWidth);
}

void AnnotationToolbar::OnStampSelected(int /*index*/) {
    emit StampNameChanged(GetCurrentStampName());
}

QString AnnotationToolbar::GetCurrentStampName() const {
    return m_stampCombo ? m_stampCombo->currentText() : "APPROVED";
}

void AnnotationToolbar::SetUndoEnabled(bool enabled) {
    m_undoAction->setEnabled(enabled);
}

void AnnotationToolbar::SetRedoEnabled(bool enabled) {
    m_redoAction->setEnabled(enabled);
}

void AnnotationToolbar::SetDeleteEnabled(bool enabled) {
    m_deleteAction->setEnabled(enabled);
}

} // namespace pdfinity::ui
