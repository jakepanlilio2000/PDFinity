// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <memory>

namespace pdfinity::ui {

class PreflightComplianceDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreflightComplianceDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );

private slots:
    void onRunPreflight();
    void onValidatePdfA();
    void onConvertToPdfA();
    void onRunAccessibility();

private:
    void setupUi();
    void setupPreflightTab();
    void setupPdfATab();
    void setupAccessibilityTab();

    std::shared_ptr<domain::DocumentModel> m_doc;

    QTabWidget* m_tabWidget{nullptr};

    // Preflight Tab
    QLabel* m_preflightStatusBanner{nullptr};
    QTableWidget* m_preflightTable{nullptr};
    QPushButton* m_runPreflightBtn{nullptr};
    QLabel* m_preflightDisclaimer{nullptr};

    // PDF/A Tab
    QComboBox* m_pdfaProfileCombo{nullptr};
    QLabel* m_pdfaStatusBanner{nullptr};
    QTableWidget* m_pdfaRulesTable{nullptr};
    QPushButton* m_validatePdfABtn{nullptr};
    QPushButton* m_convertPdfABtn{nullptr};
    QLabel* m_pdfaDisclaimer{nullptr};

    // Accessibility Tab
    QLabel* m_accessStatusBanner{nullptr};
    QTableWidget* m_accessTable{nullptr};
    QPushButton* m_runAccessBtn{nullptr};
    QLabel* m_accessDisclaimer{nullptr};
};

} // namespace pdfinity::ui
