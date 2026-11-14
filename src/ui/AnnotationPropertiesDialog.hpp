// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QColor>

namespace pdfinity::ui {

class AnnotationPropertiesDialog : public QDialog {
    Q_OBJECT
public:
    explicit AnnotationPropertiesDialog(
        const pdf_api::AnnotationData& data,
        QWidget* parent = nullptr
    );
    ~AnnotationPropertiesDialog() override = default;

    [[nodiscard]] pdf_api::AnnotationData GetUpdatedData() const;
    [[nodiscard]] bool DeleteRequested() const noexcept { return m_deleteRequested; }

private slots:
    void OnPickColor();
    void OnPickInteriorColor();
    void OnDeleteClicked();

private:
    void SetupUi();
    void UpdateColorButtons();

    pdf_api::AnnotationData m_data;
    bool m_deleteRequested{false};

    QColor m_currentColor;
    QColor m_currentInteriorColor;

    QPushButton* m_colorBtn{nullptr};
    QPushButton* m_interiorColorBtn{nullptr};
    QDoubleSpinBox* m_strokeWidthSpin{nullptr};
    QLineEdit* m_authorEdit{nullptr};
    QLineEdit* m_subjectEdit{nullptr};
    QTextEdit* m_contentsEdit{nullptr};
};

} // namespace pdfinity::ui
