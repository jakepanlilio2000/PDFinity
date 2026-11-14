// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "FormInspectorDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <fstream>

namespace pdfinity::ui {

FormInspectorDialog::FormInspectorDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
) : QDialog(parent),
    m_doc(std::move(doc)),
    m_formService(std::make_unique<services::FormService>(this)) {

    setWindowTitle(tr("Form Inspector & Field Properties"));
    resize(860, 560);
    SetupUi();
    PopulateTable();
}

void FormInspectorDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Header Summary
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #1f2937; padding: 4px;");
    mainLayout->addWidget(m_summaryLabel);

    // Split Content: Left Table, Right Properties Panel
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // Left: Fields Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("Page"), tr("Field Name"), tr("Type"), tr("Value"), tr("Default"), tr("State")
    });
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #d1d5db; border-radius: 6px; gridline-color: #f3f4f6; }"
        "QHeaderView::section { background: #f9fafb; padding: 6px; font-weight: 600; border: none; border-bottom: 1px solid #e5e7eb; }"
    );
    connect(m_table, &QTableWidget::cellClicked, this, &FormInspectorDialog::OnTableRowSelected);
    contentLayout->addWidget(m_table, 3);

    // Right: Field Properties & Editor GroupBox
    auto* editorBox = new QGroupBox(tr("Field Properties & Editor"), this);
    editorBox->setStyleSheet(
        "QGroupBox { font-weight: 600; border: 1px solid #d1d5db; border-radius: 6px; margin-top: 6px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    auto* editorLayout = new QVBoxLayout(editorBox);
    editorLayout->setSpacing(10);

    auto* metaGrid = new QGridLayout();
    metaGrid->setSpacing(8);

    metaGrid->addWidget(new QLabel(tr("<b>Name:</b>"), editorBox), 0, 0);
    m_nameLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_nameLabel, 0, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Type:</b>"), editorBox), 1, 0);
    m_typeLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_typeLabel, 1, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Tooltip:</b>"), editorBox), 2, 0);
    m_altNameLabel = new QLabel(tr("-"), editorBox);
    m_altNameLabel->setWordWrap(true);
    metaGrid->addWidget(m_altNameLabel, 2, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Coordinates:</b>"), editorBox), 3, 0);
    m_rectLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_rectLabel, 3, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Flags:</b>"), editorBox), 4, 0);
    m_flagsLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_flagsLabel, 4, 1);

    editorLayout->addLayout(metaGrid);

    editorLayout->addSpacing(8);
    auto* valHeader = new QLabel(tr("<b>Modify Value:</b>"), editorBox);
    editorLayout->addWidget(valHeader);

    m_textEdit = new QLineEdit(editorBox);
    m_textEdit->setPlaceholderText(tr("Enter text value..."));
    editorLayout->addWidget(m_textEdit);

    m_checkEdit = new QCheckBox(tr("Checked"), editorBox);
    m_checkEdit->setVisible(false);
    editorLayout->addWidget(m_checkEdit);

    m_comboEdit = new QComboBox(editorBox);
    m_comboEdit->setVisible(false);
    editorLayout->addWidget(m_comboEdit);

    m_applyButton = new QPushButton(tr("Apply Value"), editorBox);
    m_applyButton->setStyleSheet(
        "QPushButton { background: #2563eb; color: white; font-weight: 600; padding: 6px 12px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background: #1d4ed8; }"
        "QPushButton:disabled { background: #9ca3af; }"
    );
    m_applyButton->setEnabled(false);
    connect(m_applyButton, &QPushButton::clicked, this, &FormInspectorDialog::OnApplyFieldValue);
    editorLayout->addWidget(m_applyButton);

    editorLayout->addStretch(1);
    contentLayout->addWidget(editorBox, 2);

    mainLayout->addLayout(contentLayout, 1);

    // Bottom Action Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    auto* exportBtn = new QPushButton(tr("Export JSON..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &FormInspectorDialog::OnExportJson);
    buttonLayout->addWidget(exportBtn);

    auto* importBtn = new QPushButton(tr("Import JSON..."), this);
    connect(importBtn, &QPushButton::clicked, this, &FormInspectorDialog::OnImportJson);
    buttonLayout->addWidget(importBtn);

    buttonLayout->addStretch(1);

    auto* resetBtn = new QPushButton(tr("Reset Form"), this);
    resetBtn->setToolTip(tr("Restore all form fields to their original default values"));
    connect(resetBtn, &QPushButton::clicked, this, &FormInspectorDialog::OnResetForm);
    buttonLayout->addWidget(resetBtn);

    auto* flattenBtn = new QPushButton(tr("Flatten Form"), this);
    flattenBtn->setToolTip(tr("Permanently convert all interactive fields into static visual page content"));
    flattenBtn->setStyleSheet("QPushButton { color: #b91c1c; font-weight: 600; }");
    connect(flattenBtn, &QPushButton::clicked, this, &FormInspectorDialog::OnFlattenForm);
    buttonLayout->addWidget(flattenBtn);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);
}

static const char* GetFieldTypeLabel(pdf_api::FormFieldType type) {
    switch (type) {
        case pdf_api::FormFieldType::TextField: return "Text Field";
        case pdf_api::FormFieldType::CheckBox: return "Check Box";
        case pdf_api::FormFieldType::RadioButton: return "Radio Button";
        case pdf_api::FormFieldType::ComboBox: return "Drop-down (Combo)";
        case pdf_api::FormFieldType::ListBox: return "List Box";
        case pdf_api::FormFieldType::PushButton: return "Push Button";
        case pdf_api::FormFieldType::Signature: return "Digital Signature";
        case pdf_api::FormFieldType::Xfa: return "XFA Form Field";
        default: return "Unknown";
    }
}

void FormInspectorDialog::PopulateTable() {
    m_table->setRowCount(0);
    m_fields.clear();
    m_selectedFieldIdx = -1;

    if (!m_doc) return;

    auto summary = m_formService->GetFormSummary(*m_doc);
    QString typeStr = "None";
    if (summary.formType == pdf_api::FormType::AcroForm) typeStr = "Standard AcroForm (ISO 32000-1)";
    else if (summary.formType == pdf_api::FormType::XfaFull) typeStr = "Dynamic XFA (Unsupported)";
    else if (summary.formType == pdf_api::FormType::XfaForeground) typeStr = "Static XFA (Partial)";

    m_summaryLabel->setText(
        tr("<b>Form Format:</b> %1 | <b>Total Fields:</b> %2 | <b>Filled:</b> %3 (%4%) | <b>Read-Only:</b> %5")
            .arg(typeStr)
            .arg(summary.totalFields)
            .arg(summary.filledFields)
            .arg(QString::number(summary.completionPercentage, 'f', 1))
            .arg(summary.readOnlyFields)
    );

    auto fieldsRes = m_doc->GetAllFormFields();
    if (!fieldsRes.has_value()) return;

    m_fields = fieldsRes.value;
    m_table->setRowCount(static_cast<int>(m_fields.size()));

    for (int i = 0; i < static_cast<int>(m_fields.size()); ++i) {
        const auto& f = m_fields[i];

        auto* pageItem = new QTableWidgetItem(QString::number(f.pageIndex + 1));
        pageItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, pageItem);

        auto* nameItem = new QTableWidgetItem(QString::fromStdString(f.name.empty() ? "(Unnamed)" : f.name));
        m_table->setItem(i, 1, nameItem);

        auto* typeItem = new QTableWidgetItem(tr(GetFieldTypeLabel(f.type)));
        m_table->setItem(i, 2, typeItem);

        QString valStr;
        if (f.type == pdf_api::FormFieldType::CheckBox || f.type == pdf_api::FormFieldType::RadioButton) {
            valStr = f.isChecked ? tr("Checked [x]") : tr("Unchecked [ ]");
        } else {
            valStr = QString::fromStdString(f.value);
        }
        auto* valItem = new QTableWidgetItem(valStr);
        m_table->setItem(i, 3, valItem);

        auto* defItem = new QTableWidgetItem(QString::fromStdString(f.defaultValue));
        m_table->setItem(i, 4, defItem);

        QString flagsStr;
        if (f.isReadOnly) flagsStr += tr("[Read-Only] ");
        if (f.isRequired) flagsStr += tr("[Required] ");
        if (flagsStr.isEmpty()) flagsStr = tr("Normal");
        auto* flagsItem = new QTableWidgetItem(flagsStr);
        m_table->setItem(i, 5, flagsItem);
    }

    if (!m_fields.empty()) {
        m_table->selectRow(0);
        UpdateEditorPanel(0);
    } else {
        UpdateEditorPanel(-1);
    }
}

void FormInspectorDialog::OnTableRowSelected(int row) {
    if (row >= 0 && row < static_cast<int>(m_fields.size())) {
        UpdateEditorPanel(row);
        emit FieldSelected(m_fields[row].pageIndex, m_fields[row].annotIndex);
    }
}

void FormInspectorDialog::UpdateEditorPanel(int fieldIdx) {
    m_selectedFieldIdx = fieldIdx;

    if (fieldIdx < 0 || fieldIdx >= static_cast<int>(m_fields.size())) {
        m_nameLabel->setText("-");
        m_typeLabel->setText("-");
        m_altNameLabel->setText("-");
        m_rectLabel->setText("-");
        m_flagsLabel->setText("-");
        m_textEdit->clear();
        m_textEdit->setEnabled(false);
        m_checkEdit->setVisible(false);
        m_comboEdit->setVisible(false);
        m_applyButton->setEnabled(false);
        return;
    }

    const auto& f = m_fields[fieldIdx];
    m_nameLabel->setText(QString::fromStdString(f.name.empty() ? "(Unnamed Field)" : f.name));
    m_typeLabel->setText(tr(GetFieldTypeLabel(f.type)));
    m_altNameLabel->setText(QString::fromStdString(f.altName.empty() ? "(None)" : f.altName));
    m_rectLabel->setText(QString("X: %1, Y: %2, W: %3, H: %4")
        .arg(f.rect.x, 0, 'f', 1)
        .arg(f.rect.y, 0, 'f', 1)
        .arg(f.rect.width, 0, 'f', 1)
        .arg(f.rect.height, 0, 'f', 1));

    QString flags;
    if (f.isReadOnly) flags += "Read-Only ";
    if (f.isRequired) flags += "Required ";
    m_flagsLabel->setText(flags.isEmpty() ? tr("Editable") : flags);

    // Show appropriate editor
    if (f.type == pdf_api::FormFieldType::CheckBox || f.type == pdf_api::FormFieldType::RadioButton) {
        m_textEdit->setVisible(false);
        m_comboEdit->setVisible(false);
        m_checkEdit->setVisible(true);
        m_checkEdit->setChecked(f.isChecked);
        m_checkEdit->setEnabled(!f.isReadOnly);
    } else if (f.type == pdf_api::FormFieldType::ComboBox || f.type == pdf_api::FormFieldType::ListBox) {
        m_textEdit->setVisible(false);
        m_checkEdit->setVisible(false);
        m_comboEdit->setVisible(true);
        m_comboEdit->clear();
        for (const auto& opt : f.options) {
            m_comboEdit->addItem(QString::fromStdString(opt));
        }
        int curIdx = m_comboEdit->findText(QString::fromStdString(f.value));
        if (curIdx >= 0) m_comboEdit->setCurrentIndex(curIdx);
        m_comboEdit->setEnabled(!f.isReadOnly);
    } else {
        m_checkEdit->setVisible(false);
        m_comboEdit->setVisible(false);
        m_textEdit->setVisible(true);
        m_textEdit->setText(QString::fromStdString(f.value));
        m_textEdit->setEnabled(!f.isReadOnly);
    }

    m_applyButton->setEnabled(!f.isReadOnly);
}

void FormInspectorDialog::OnApplyFieldValue() {
    if (m_selectedFieldIdx < 0 || m_selectedFieldIdx >= static_cast<int>(m_fields.size()) || !m_doc) {
        return;
    }

    const auto& f = m_fields[m_selectedFieldIdx];

    if (f.type == pdf_api::FormFieldType::CheckBox || f.type == pdf_api::FormFieldType::RadioButton) {
        bool chk = m_checkEdit->isChecked();
        m_formService->SetFieldChecked(*m_doc, f.pageIndex, f.annotIndex, chk);
    } else if (f.type == pdf_api::FormFieldType::ComboBox || f.type == pdf_api::FormFieldType::ListBox) {
        int optIdx = m_comboEdit->currentIndex();
        if (optIdx >= 0) {
            m_formService->SetFieldOption(*m_doc, f.pageIndex, f.annotIndex, optIdx);
        }
    } else {
        std::string txt = m_textEdit->text().toStdString();
        m_formService->SetFieldValue(*m_doc, f.pageIndex, f.annotIndex, txt);
    }

    int savedIdx = m_selectedFieldIdx;
    PopulateTable();
    if (savedIdx >= 0 && savedIdx < static_cast<int>(m_fields.size())) {
        m_table->selectRow(savedIdx);
        UpdateEditorPanel(savedIdx);
    }
    emit FormModified();
}

void FormInspectorDialog::OnResetForm() {
    if (!m_doc) return;

    auto ans = QMessageBox::question(
        this,
        tr("Reset Form"),
        tr("Are you sure you want to restore all form fields to their default values?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (ans == QMessageBox::Yes) {
        m_formService->ResetForm(*m_doc);
        PopulateTable();
        emit FormModified();
    }
}

void FormInspectorDialog::OnFlattenForm() {
    if (!m_doc) return;

    auto ans = QMessageBox::warning(
        this,
        tr("Flatten Form Fields"),
        tr("Flattening permanently converts all interactive form fields into static visual page content.\n\n"
           "Once flattened, fields can no longer be edited interactively. Proceed?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (ans == QMessageBox::Yes) {
        m_formService->FlattenForm(*m_doc);
        PopulateTable();
        emit FormModified();
    }
}

void FormInspectorDialog::OnExportJson() {
    if (!m_doc) return;

    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export Form Data as JSON"),
        QString(),
        tr("JSON Files (*.json)")
    );
    if (path.isEmpty()) return;

    std::string jsonStr = m_formService->ExportFormDataToJson(*m_doc);
    std::ofstream out(path.toStdString());
    if (out.is_open()) {
        out << jsonStr;
        out.close();
        QMessageBox::information(this, tr("Export Successful"), tr("Form data exported successfully to:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not write to destination file."));
    }
}

void FormInspectorDialog::OnImportJson() {
    if (!m_doc) return;

    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Import Form Data from JSON"),
        QString(),
        tr("JSON Files (*.json)")
    );
    if (path.isEmpty()) return;

    std::ifstream in(path.toStdString());
    if (!in.is_open()) {
        QMessageBox::critical(this, tr("Import Failed"), tr("Could not open JSON file."));
        return;
    }

    std::string jsonStr((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    auto res = m_formService->ImportFormDataFromJson(*m_doc, jsonStr);
    if (res.has_value()) {
        PopulateTable();
        emit FormModified();
        QMessageBox::information(this, tr("Import Successful"), tr("Successfully imported and updated %1 field(s).").arg(res.value));
    } else {
        QMessageBox::critical(this, tr("Import Failed"), tr("Failed to import form data:\n%1").arg(QString::fromStdString(res.errorMessage)));
    }
}

} // namespace pdfinity::ui
