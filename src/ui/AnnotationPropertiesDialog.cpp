// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AnnotationPropertiesDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QColorDialog>
#include <QMessageBox>
#include <QLabel>

namespace pdfinity::ui {

AnnotationPropertiesDialog::AnnotationPropertiesDialog(
    const pdf_api::AnnotationData& data,
    QWidget* parent
)
    : QDialog(parent),
      m_data(data),
      m_currentColor(data.color.r, data.color.g, data.color.b, data.color.a),
      m_currentInteriorColor(data.interiorColor.r, data.interiorColor.g, data.interiorColor.b, data.interiorColor.a) {
    setWindowTitle(tr("Annotation Properties"));
    resize(420, 400);
    setMinimumSize(360, 320);

    SetupUi();
}

void AnnotationPropertiesDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    auto* formLayout = new QFormLayout();

    // Color Pickers
    auto* colorRow = new QHBoxLayout();
    m_colorBtn = new QPushButton(this);
    m_colorBtn->setFixedSize(60, 24);
    colorRow->addWidget(m_colorBtn);

    colorRow->addWidget(new QLabel(tr("Fill Color:"), this));
    m_interiorColorBtn = new QPushButton(this);
    m_interiorColorBtn->setFixedSize(60, 24);
    colorRow->addWidget(m_interiorColorBtn);
    colorRow->addStretch();
    formLayout->addRow(tr("Stroke / Color:"), colorRow);

    // Stroke width
    m_strokeWidthSpin = new QDoubleSpinBox(this);
    m_strokeWidthSpin->setRange(0.5, 20.0);
    m_strokeWidthSpin->setSingleStep(0.5);
    m_strokeWidthSpin->setValue(m_data.strokeWidth > 0.0f ? m_data.strokeWidth : 2.0);
    formLayout->addRow(tr("Line Width:"), m_strokeWidthSpin);

    // Author
    m_authorEdit = new QLineEdit(QString::fromStdString(m_data.author), this);
    formLayout->addRow(tr("Author:"), m_authorEdit);

    // Subject
    m_subjectEdit = new QLineEdit(QString::fromStdString(m_data.subject), this);
    formLayout->addRow(tr("Subject:"), m_subjectEdit);

    // Contents / Comment text
    m_contentsEdit = new QTextEdit(this);
    m_contentsEdit->setPlainText(QString::fromStdString(m_data.contents));
    m_contentsEdit->setPlaceholderText(tr("Enter notes or comment text here..."));
    formLayout->addRow(tr("Note Content:"), m_contentsEdit);

    mainLayout->addLayout(formLayout, 1);

    // Bottom Buttons
    auto* bottomLayout = new QHBoxLayout();
    auto* deleteBtn = new QPushButton(tr("Delete Annotation"), this);
    deleteBtn->setStyleSheet("QPushButton { color: #dc2626; }");
    bottomLayout->addWidget(deleteBtn);
    bottomLayout->addStretch();

    auto* okBtn = new QPushButton(tr("OK"), this);
    okBtn->setStyleSheet("QPushButton { font-weight: bold; background-color: #DC2626; color: white; padding: 5px 14px; border-radius: 4px; } QPushButton:hover { background-color: #B91C1C; }");
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);

    bottomLayout->addWidget(okBtn);
    bottomLayout->addWidget(cancelBtn);
    mainLayout->addLayout(bottomLayout);

    connect(m_colorBtn, &QPushButton::clicked, this, &AnnotationPropertiesDialog::OnPickColor);
    connect(m_interiorColorBtn, &QPushButton::clicked, this, &AnnotationPropertiesDialog::OnPickInteriorColor);
    connect(deleteBtn, &QPushButton::clicked, this, &AnnotationPropertiesDialog::OnDeleteClicked);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    UpdateColorButtons();
}

void AnnotationPropertiesDialog::UpdateColorButtons() {
    m_colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #94a3b8; border-radius: 3px;").arg(m_currentColor.name()));
    m_interiorColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #94a3b8; border-radius: 3px;").arg(m_currentInteriorColor.alpha() > 0 ? m_currentInteriorColor.name() : "transparent"));
}

void AnnotationPropertiesDialog::OnPickColor() {
    QColor c = QColorDialog::getColor(m_currentColor, this, tr("Select Stroke / Highlight Color"), QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_currentColor = c;
        UpdateColorButtons();
    }
}

void AnnotationPropertiesDialog::OnPickInteriorColor() {
    QColor c = QColorDialog::getColor(m_currentInteriorColor, this, tr("Select Fill / Interior Color"), QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_currentInteriorColor = c;
        UpdateColorButtons();
    }
}

void AnnotationPropertiesDialog::OnDeleteClicked() {
    auto ans = QMessageBox::question(
        this,
        tr("Delete Annotation"),
        tr("Are you sure you want to delete this annotation?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (ans == QMessageBox::Yes) {
        m_deleteRequested = true;
        accept();
    }
}

pdf_api::AnnotationData AnnotationPropertiesDialog::GetUpdatedData() const {
    pdf_api::AnnotationData updated = m_data;
    updated.color = pdf_api::PdfColor{
        static_cast<uint8_t>(m_currentColor.red()),
        static_cast<uint8_t>(m_currentColor.green()),
        static_cast<uint8_t>(m_currentColor.blue()),
        static_cast<uint8_t>(m_currentColor.alpha())
    };
    updated.interiorColor = pdf_api::PdfColor{
        static_cast<uint8_t>(m_currentInteriorColor.red()),
        static_cast<uint8_t>(m_currentInteriorColor.green()),
        static_cast<uint8_t>(m_currentInteriorColor.blue()),
        static_cast<uint8_t>(m_currentInteriorColor.alpha())
    };
    updated.strokeWidth = static_cast<float>(m_strokeWidthSpin->value());
    updated.author = m_authorEdit->text().trimmed().toStdString();
    updated.subject = m_subjectEdit->text().trimmed().toStdString();
    updated.contents = m_contentsEdit->toPlainText().toStdString();
    return updated;
}

} // namespace pdfinity::ui
