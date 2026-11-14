// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include "domain/DocumentModel.hpp"
#include <memory>

namespace pdfinity::ui {

class ContextInspectorPanel : public QWidget {
    Q_OBJECT

public:
    explicit ContextInspectorPanel(QWidget* parent = nullptr);
    ~ContextInspectorPanel() override = default;

    void SetDocument(std::shared_ptr<domain::DocumentModel> doc);
    void UpdatePageContext(int currentPage, double zoomScale);

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

private:
    void SetupUi();
    QWidget* CreatePropertiesTab();
    QWidget* CreateToolsPaletteTab();
    QPushButton* CreateToolButton(const QString& title, const QString& desc, IconType icon, const std::function<void()>& onClick);

    std::shared_ptr<domain::DocumentModel> m_doc;

    QTabWidget* m_tabWidget{nullptr};

    // Properties labels
    QLabel* m_propFileName{nullptr};
    QLabel* m_propFileSize{nullptr};
    QLabel* m_propPageCount{nullptr};
    QLabel* m_propPageDimensions{nullptr};
    QLabel* m_propZoomLevel{nullptr};
    QLabel* m_propEncryption{nullptr};
    QLabel* m_propPermissions{nullptr};
    QLabel* m_propSignatures{nullptr};
};

} // namespace pdfinity::ui
