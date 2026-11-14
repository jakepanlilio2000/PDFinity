// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "services/ContentEditingService.hpp"
#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <memory>

namespace pdfinity::ui {

class ObjectInspectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ObjectInspectorDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        int currentPageIndex,
        std::shared_ptr<services::ContentEditingService> editService,
        QWidget* parent = nullptr
    );
    ~ObjectInspectorDialog() override = default;

    void SetActivePage(int pageIndex);
    void RefreshObjects();

signals:
    void ObjectSelected(int pageIndex, int objectIndex);
    void ContentModified();

private slots:
    void OnTableRowSelected(int row);
    void OnApplyTextEdit();
    void OnDeleteObject();
    void OnTranslateObject();
    void OnBringForward();
    void OnSendBackward();
    void OnBringToFront();
    void OnSendToBack();
    void OnAlignObjects(int index);

private:
    void SetupUi();
    void UpdateEditorPanel(int objectIndex);

    std::shared_ptr<domain::DocumentModel> m_doc;
    int m_pageIndex{0};
    std::shared_ptr<services::ContentEditingService> m_editService;
    std::vector<pdf_api::PageObjectData> m_objects;
    int m_selectedObjIndex{-1};

    // UI Widgets
    QLabel* m_layerBadgeLabel{nullptr};
    QLabel* m_fontWarningBanner{nullptr};
    QTableWidget* m_table{nullptr};

    // Editor widgets
    QLabel* m_typeLabel{nullptr};
    QLabel* m_coordsLabel{nullptr};
    QLabel* m_fontInfoLabel{nullptr};
    QLineEdit* m_textEdit{nullptr};
    QDoubleSpinBox* m_fontSizeSpin{nullptr};
    QDoubleSpinBox* m_dxSpin{nullptr};
    QDoubleSpinBox* m_dySpin{nullptr};
    QPushButton* m_applyTextBtn{nullptr};
    QPushButton* m_translateBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
    QComboBox* m_alignCombo{nullptr};
};

} // namespace pdfinity::ui
