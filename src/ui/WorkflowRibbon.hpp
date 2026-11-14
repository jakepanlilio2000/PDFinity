// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QToolButton>
#include "VectorIcons.hpp"

namespace pdfinity::ui {

class WorkflowRibbon : public QWidget {
    Q_OBJECT

public:
    explicit WorkflowRibbon(QWidget* parent = nullptr);
    ~WorkflowRibbon() override = default;

    void SetActiveDocumentAvailable(bool available);

signals:
    // Home / File
    void HomeClicked();
    void OpenClicked();
    void SaveClicked();
    void PrintClicked();
    void ShareClicked();
    void FindClicked();
    void UndoClicked();
    void RedoClicked();

    // Assembly
    void OrganizePagesClicked();
    void MergeDocumentsClicked();
    void SplitDocumentClicked();
    void RotateClicked();

    // Annotate
    void ToggleAnnotationStudioClicked();

    // Forms & Edit
    void ToggleContentEditorClicked();
    void FormInspectorClicked();
    void ObjectInspectorClicked();

    // Security & Sign
    void DigitalSignClicked();
    void VisualSignClicked();
    void ValidateSignaturesClicked();
    void ToggleRedactionStudioClicked();
    void SanitizeClicked();
    void CertificateManagerClicked();

    // Tools
    void OcrStudioClicked();
    void ConversionCenterClicked();
    void OptimizerClicked();
    void DocumentIntelligenceClicked();
    void CompareDocumentsClicked();
    void BatchCenterClicked();
    void SettingsClicked();

private:
    void SetupUi();
    QWidget* CreateHomeTab();
    QWidget* CreateAssemblyTab();
    QWidget* CreateAnnotateTab();
    QWidget* CreateFormsEditTab();
    QWidget* CreateSecurityTab();
    QWidget* CreateToolsTab();

    QToolButton* CreateRibbonButton(
        const QString& label,
        IconType icon,
        const std::function<void()>& onClick,
        const QString& tooltip = "",
        bool isLarge = true
    );

    QTabWidget* m_ribbonTabs{nullptr};
    std::vector<QWidget*> m_docDependentButtons;
};

} // namespace pdfinity::ui
