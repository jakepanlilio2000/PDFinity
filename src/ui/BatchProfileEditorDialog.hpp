// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/BatchTypes.hpp"
#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QStackedWidget>

namespace pdfinity::ui {

class BatchProfileEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchProfileEditorDialog(QWidget* parent = nullptr);

private slots:
    void onProfileSelected(int row);
    void onNewProfile();
    void onCloneProfile();
    void onDeleteProfile();
    void onSaveProfile();
    void onImportProfile();
    void onExportProfile();

    void onAddOperation();
    void onRemoveOperation();
    void onMoveOpUp();
    void onMoveOpDown();
    void onOperationSelected(int row);

    // Param sync slots
    void onOptimizePresetChanged(int index);
    void onPdfAProfileChanged(int index);

private:
    void setupUi();
    void populateProfilesList(const std::string& selectId = "");
    void loadProfileIntoEditor(const domain::BatchProfile& profile);
    void updateOperationEditorFor(const domain::BatchOperation& op);
    void syncCurrentOperationParams();

    // UI elements
    QListWidget* m_profilesList{nullptr};
    QPushButton* m_newBtn{nullptr};
    QPushButton* m_cloneBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};

    QLineEdit* m_nameEdit{nullptr};
    QTextEdit* m_descEdit{nullptr};
    QLabel* m_typeBadge{nullptr};

    QListWidget* m_opsList{nullptr};
    QPushButton* m_addOpBtn{nullptr};
    QPushButton* m_removeOpBtn{nullptr};
    QPushButton* m_moveUpBtn{nullptr};
    QPushButton* m_moveDownBtn{nullptr};

    // Operation Configuration Panels
    QStackedWidget* m_paramStack{nullptr};
    // Page 0: Optimize
    QComboBox* m_optPresetCombo{nullptr};
    QSpinBox* m_optDpiSpin{nullptr};
    QSpinBox* m_optQualitySpin{nullptr};
    QCheckBox* m_optStripMetaCheck{nullptr};
    // Page 1: OCR
    QComboBox* m_ocrLangCombo{nullptr};
    QSpinBox* m_ocrDpiSpin{nullptr};
    // Page 2: PDF/A
    QComboBox* m_pdfaProfileCombo{nullptr};
    // Page 3: Sanitize
    QComboBox* m_sanProfileCombo{nullptr};
    // Page 4: Export Images
    QComboBox* m_imgFormatCombo{nullptr};
    QSpinBox* m_imgDpiSpin{nullptr};
    // Page 5: Generic info label
    QLabel* m_genericLabel{nullptr};

    QPushButton* m_saveBtn{nullptr};

    domain::BatchProfile m_currentEditingProfile;
    bool m_isBuiltIn{false};
};

} // namespace pdfinity::ui
