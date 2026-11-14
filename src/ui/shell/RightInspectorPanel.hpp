// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLineEdit>
#include <memory>
#include <functional>
#include "domain/DocumentModel.hpp"
#include "ui/VectorIcons.hpp"

namespace pdfinity::ui {

struct SelectionPropertiesInfo {
    QString objectType{"Annotation"};
    double x{0.0};
    double y{0.0};
    double width{100.0};
    double height{30.0};
    QColor strokeColor{220, 38, 38};
    QColor fillColor{Qt::transparent};
    double strokeWidth{2.0};
    double opacity{1.0};
    QString author{"PDFinity User"};
    QString contents{""};
};

class RightInspectorPanel : public QWidget {
    Q_OBJECT

public:
    explicit RightInspectorPanel(QWidget* parent = nullptr);
    ~RightInspectorPanel() override = default;

    void SetDocument(std::shared_ptr<domain::DocumentModel> doc);
    void UpdatePageContext(int currentPage, double zoomScale);
    void SetSelectionProperties(const SelectionPropertiesInfo& info);
    void ClearSelection();
    void SetAllToolsMode();
    void SetSelectionMode();

signals:
    void CloseRequested();

    // Suite Tool Signals
    void OrganizePagesTriggered();
    void MergeDocumentsTriggered();
    void SplitDocumentTriggered();
    void ContentEditorTriggered();
    void FormInspectorTriggered();
    void ObjectInspectorTriggered();
    void DigitalSignTriggered();
    void VisualSignTriggered();
    void ValidateSignaturesTriggered();
    void RedactionStudioTriggered();
    void SanitizeTriggered();
    void CertificateManagerTriggered();
    void OcrStudioTriggered();
    void ConversionCenterTriggered();
    void OptimizerTriggered();
    void DocumentIntelligenceTriggered();
    void CompareDocumentsTriggered();
    void BatchCenterTriggered();
    void PreflightTriggered();

    // Contextual Property Modification Signals
    void StrokeColorChanged(const QColor& color);
    void FillColorChanged(const QColor& color);
    void StrokeWidthChanged(double width);
    void OpacityChanged(double opacity);
    void DeleteSelectionTriggered();

private:
    void SetupUi();
    QWidget* CreateAllToolsWidget();
    QWidget* CreateSelectionPropertiesWidget();
    QPushButton* CreateToolCardButton(
        const QString& title,
        const QString& desc,
        IconType icon,
        const std::function<void()>& onClick
    );

    std::shared_ptr<domain::DocumentModel> m_doc;

    QLabel* m_headerTitle{nullptr};
    QStackedWidget* m_stack{nullptr};

    // Properties UI controls
    QLabel* m_selectionTypeLabel{nullptr};
    QDoubleSpinBox* m_spinX{nullptr};
    QDoubleSpinBox* m_spinY{nullptr};
    QDoubleSpinBox* m_spinW{nullptr};
    QDoubleSpinBox* m_spinH{nullptr};
    QPushButton* m_strokeColorBtn{nullptr};
    QPushButton* m_fillColorBtn{nullptr};
    QDoubleSpinBox* m_spinStrokeWidth{nullptr};
    QSlider* m_opacitySlider{nullptr};
    QLabel* m_opacityLabel{nullptr};
    QLineEdit* m_authorEdit{nullptr};
    QLineEdit* m_contentsEdit{nullptr};

    QColor m_currentStrokeColor{220, 38, 38};
    QColor m_currentFillColor{Qt::transparent};
};

} // namespace pdfinity::ui