// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QToolButton>
#include <vector>
#include <functional>
#include "ui/VectorIcons.hpp"

namespace pdfinity::ui {

class WorkflowRibbon : public QWidget {
    Q_OBJECT

public:
    explicit WorkflowRibbon(QWidget* parent = nullptr);
    ~WorkflowRibbon() override = default;

    void SetActiveDocumentAvailable(bool available);
    void SetActiveMode(int index);

signals:
    // Quick Access Persistent Strip
    void HomeClicked();
    void OpenClicked();
    void SaveClicked();
    void PrintClicked();
    void ShareClicked();
    void UndoClicked();
    void RedoClicked();
    void FindClicked();

    // Mode: All Tools
    void AllToolsClicked();

    // Mode: View
    void ViewPanClicked();
    void ViewSelectClicked();
    void ViewZoomInClicked();
    void ViewZoomOutClicked();
    void ViewZoom100Clicked();
    void ViewFitWidthClicked();
    void ViewFitPageClicked();
    void ViewRotateClicked();
    void ViewFullscreenClicked();

    // Mode: Comment / Annotate
    void ToggleAnnotationStudioClicked();
    void CommentHighlightClicked();
    void CommentUnderlineClicked();
    void CommentStrikeoutClicked();
    void CommentNoteClicked();
    void CommentDrawClicked();
    void CommentShapesClicked();
    void CommentCalloutClicked();
    void CommentStampClicked();

    // Mode: Edit
    void ToggleContentEditorClicked();
    void FormInspectorClicked();
    void ObjectInspectorClicked();

    // Mode: Organize
    void OrganizePagesClicked();
    void MergeDocumentsClicked();
    void SplitDocumentClicked();
    void RotateClicked();

    // Mode: Fill & Sign
    void DigitalSignClicked();
    void VisualSignClicked();
    void ValidateSignaturesClicked();
    void CertificateManagerClicked();

    // Mode: Protect & Redact
    void ToggleRedactionStudioClicked();
    void ApplyRedactionsClicked();
    void SanitizeClicked();
    void EncryptClicked();

    // Mode: Convert & OCR
    void OcrStudioClicked();
    void ConversionCenterClicked();
    void OptimizerClicked();
    void DocumentIntelligenceClicked();
    void CompareDocumentsClicked();
    void PreflightClicked();
    void BatchCenterClicked();
    void SettingsClicked();

private:
    void SetupUi();
    QWidget* CreateAllToolsStrip();
    QWidget* CreateViewStrip();
    QWidget* CreateCommentStrip();
    QWidget* CreateEditStrip();
    QWidget* CreateOrganizeStrip();
    QWidget* CreateFillSignStrip();
    QWidget* CreateProtectRedactStrip();
    QWidget* CreateConvertOcrStrip();
    QWidget* CreateQuickAccessStrip();

    QToolButton* CreateRibbonButton(
        const QString& label,
        IconType icon,
        const std::function<void()>& onClick,
        const QString& tooltip = "",
        bool isLarge = false
    );
    QFrame* CreateVerticalDivider();

    QTabWidget* m_modeTabs{nullptr};
    std::vector<QWidget*> m_docDependentButtons;
};

} // namespace pdfinity::ui
