// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "services/FormService.hpp"
#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <algorithm>

namespace pdfinity::ui {

class FormInspectorDialog : public QDialog {
    Q_OBJECT
public:
    FormInspectorDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );
    ~FormInspectorDialog() override = default;

signals:
    void FormModified();
    void FieldSelected(int pageIndex, int annotIndex);

private slots:
    void OnTableRowSelected(int row);
    void OnApplyFieldValue();
    void OnResetForm();
    void OnFlattenForm();
    void OnExportJson();
    void OnImportJson();

private:
    void SetupUi();
    void PopulateTable();
    void UpdateEditorPanel(int fieldIdx);

    std::shared_ptr<domain::DocumentModel> m_doc;
    std::unique_ptr<services::FormService> m_formService;
    std::vector<pdf_api::FormFieldData> m_fields;

    QTableWidget* m_table{nullptr};
    QLabel* m_summaryLabel{nullptr};

    // Editor panel
    QLabel* m_nameLabel{nullptr};
    QLabel* m_typeLabel{nullptr};
    QLabel* m_altNameLabel{nullptr};
    QLabel* m_rectLabel{nullptr};
    QLabel* m_flagsLabel{nullptr};

    QLineEdit* m_textEdit{nullptr};
    QCheckBox* m_checkEdit{nullptr};
    QComboBox* m_comboEdit{nullptr};
    QPushButton* m_applyButton{nullptr};

    int m_selectedFieldIdx{-1};
};

} // namespace pdfinity::ui
