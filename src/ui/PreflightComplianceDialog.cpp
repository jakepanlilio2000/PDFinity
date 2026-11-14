// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PreflightComplianceDialog.hpp"
#include "services/PreflightService.hpp"
#include "services/PdfAComplianceService.hpp"
#include "services/AccessibilityService.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>

namespace pdfinity::ui {

PreflightComplianceDialog::PreflightComplianceDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
)
    : QDialog(parent),
      m_doc(std::move(doc))
{
    setWindowTitle(tr("Preflight & Standards Compliance Studio"));
    resize(860, 640);
    setModal(true);

    setupUi();

    // Auto-run checks on startup
    onRunPreflight();
    onValidatePdfA();
    onRunAccessibility();
}

void PreflightComplianceDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    setupPreflightTab();
    setupPdfATab();
    setupAccessibilityTab();

    mainLayout->addWidget(m_tabWidget);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void PreflightComplianceDialog::setupPreflightTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_preflightStatusBanner = new QLabel(tab);
    m_preflightStatusBanner->setWordWrap(true);
    layout->addWidget(m_preflightStatusBanner);

    m_preflightTable = new QTableWidget(tab);
    m_preflightTable->setColumnCount(6);
    m_preflightTable->setHorizontalHeaderLabels({
        tr("Severity"), tr("Category"), tr("Page"), tr("Issue / Finding"), tr("Remediation"), tr("Inspection Type")
    });
    m_preflightTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_preflightTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_preflightTable->setAlternatingRowColors(true);
    layout->addWidget(m_preflightTable);

    m_preflightDisclaimer = new QLabel(tab);
    m_preflightDisclaimer->setWordWrap(true);
    m_preflightDisclaimer->setStyleSheet("color: #64748B; font-size: 11px; padding: 4px;");
    layout->addWidget(m_preflightDisclaimer);

    auto* btnRow = new QHBoxLayout();
    m_runPreflightBtn = new QPushButton(tr("Re-Run Preflight Checks"), tab);
    m_runPreflightBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_runPreflightBtn, &QPushButton::clicked, this, &PreflightComplianceDialog::onRunPreflight);
    btnRow->addWidget(m_runPreflightBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_tabWidget->addTab(tab, tr("1. Preflight Inspection"));
}

void PreflightComplianceDialog::setupPdfATab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    auto* ctrlRow = new QHBoxLayout();
    ctrlRow->addWidget(new QLabel(tr("Target ISO Profile:"), tab));
    m_pdfaProfileCombo = new QComboBox(tab);
    m_pdfaProfileCombo->addItem(tr("PDF/A-1b (ISO 19005-1:2005 Level B)"), static_cast<int>(pdf_api::PdfAProfile::PdfA1b));
    m_pdfaProfileCombo->addItem(tr("PDF/A-2b (ISO 19005-2:2011 Level B)"), static_cast<int>(pdf_api::PdfAProfile::PdfA2b));
    ctrlRow->addWidget(m_pdfaProfileCombo);

    m_validatePdfABtn = new QPushButton(tr("Validate Profile Compliance"), tab);
    connect(m_validatePdfABtn, &QPushButton::clicked, this, &PreflightComplianceDialog::onValidatePdfA);
    ctrlRow->addWidget(m_validatePdfABtn);

    ctrlRow->addStretch();

    m_convertPdfABtn = new QPushButton(tr("Convert to Compliant PDF/A..."), tab);
    m_convertPdfABtn->setStyleSheet("background-color: #16A34A; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_convertPdfABtn, &QPushButton::clicked, this, &PreflightComplianceDialog::onConvertToPdfA);
    ctrlRow->addWidget(m_convertPdfABtn);

    layout->addLayout(ctrlRow);

    m_pdfaStatusBanner = new QLabel(tab);
    m_pdfaStatusBanner->setWordWrap(true);
    layout->addWidget(m_pdfaStatusBanner);

    m_pdfaRulesTable = new QTableWidget(tab);
    m_pdfaRulesTable->setColumnCount(4);
    m_pdfaRulesTable->setHorizontalHeaderLabels({
        tr("ISO Clause"), tr("Specification Rule"), tr("Conformance Status"), tr("Failure Detail / Evidence")
    });
    m_pdfaRulesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_pdfaRulesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_pdfaRulesTable->setAlternatingRowColors(true);
    layout->addWidget(m_pdfaRulesTable);

    m_pdfaDisclaimer = new QLabel(tab);
    m_pdfaDisclaimer->setWordWrap(true);
    m_pdfaDisclaimer->setStyleSheet("color: #64748B; font-size: 11px; padding: 4px;");
    layout->addWidget(m_pdfaDisclaimer);

    m_tabWidget->addTab(tab, tr("2. PDF/A Standards Validation"));
}

void PreflightComplianceDialog::setupAccessibilityTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_accessStatusBanner = new QLabel(tab);
    m_accessStatusBanner->setWordWrap(true);
    layout->addWidget(m_accessStatusBanner);

    m_accessTable = new QTableWidget(tab);
    m_accessTable->setColumnCount(5);
    m_accessTable->setHorizontalHeaderLabels({
        tr("Severity"), tr("Rule / Guideline"), tr("Page"), tr("Accessibility Issue"), tr("Actionable Remediation")
    });
    m_accessTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_accessTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_accessTable->setAlternatingRowColors(true);
    layout->addWidget(m_accessTable);

    m_accessDisclaimer = new QLabel(tab);
    m_accessDisclaimer->setWordWrap(true);
    m_accessDisclaimer->setStyleSheet("color: #64748B; font-size: 11px; padding: 4px;");
    layout->addWidget(m_accessDisclaimer);

    auto* btnRow = new QHBoxLayout();
    m_runAccessBtn = new QPushButton(tr("Re-Run Accessibility Checks"), tab);
    m_runAccessBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_runAccessBtn, &QPushButton::clicked, this, &PreflightComplianceDialog::onRunAccessibility);
    btnRow->addWidget(m_runAccessBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_tabWidget->addTab(tab, tr("3. Accessibility & Tagged PDF"));
}

void PreflightComplianceDialog::onRunPreflight() {
    if (!m_doc || !m_doc->IsValid()) return;

    auto res = services::PreflightService::Instance().RunPreflight(*m_doc);
    if (!res.has_value()) return;

    const auto& rep = res.value;
    m_preflightDisclaimer->setText(QString::fromStdString(rep.disclaimer));

    if (rep.isPassed) {
        m_preflightStatusBanner->setText(
            QString("<b>✅ Preflight Passed:</b> No critical errors detected. Found %1 warning(s) and %2 informational note(s).")
                .arg(rep.totalWarnings).arg(rep.totalInfos)
        );
        m_preflightStatusBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; padding: 8px 12px; border: 1px solid #86EFAC; border-radius: 4px;");
    } else {
        m_preflightStatusBanner->setText(
            QString("<b>⚠️ Preflight Errors Detected:</b> Found <b>%1 critical error(s)</b> and %2 warning(s) that require attention before publishing.")
                .arg(rep.totalErrors).arg(rep.totalWarnings)
        );
        m_preflightStatusBanner->setStyleSheet("background-color: #FEE2E2; color: #991B1B; padding: 8px 12px; border: 1px solid #FCA5A5; border-radius: 4px;");
    }

    m_preflightTable->setRowCount(static_cast<int>(rep.findings.size()));
    for (size_t i = 0; i < rep.findings.size(); ++i) {
        const auto& f = rep.findings[i];

        QString sevStr;
        QColor sevColor = Qt::white;
        if (f.severity == pdf_api::PreflightSeverity::Error) {
            sevStr = "❌ Error";
            sevColor = QColor(254, 226, 226);
        } else if (f.severity == pdf_api::PreflightSeverity::Warning) {
            sevStr = "⚠️ Warning";
            sevColor = QColor(254, 243, 199);
        } else {
            sevStr = "ℹ️ Notice";
            sevColor = QColor(241, 245, 249);
        }

        QString catStr;
        switch (f.category) {
            case pdf_api::PreflightCategory::Font: catStr = "Font"; break;
            case pdf_api::PreflightCategory::Image: catStr = "Image"; break;
            case pdf_api::PreflightCategory::PageBox: catStr = "Page Box"; break;
            case pdf_api::PreflightCategory::Encryption: catStr = "Security"; break;
            case pdf_api::PreflightCategory::Structure: catStr = "Structure"; break;
            case pdf_api::PreflightCategory::Metadata: catStr = "Metadata"; break;
        }

        auto* itemSev = new QTableWidgetItem(sevStr);
        itemSev->setBackground(sevColor);

        m_preflightTable->setItem(static_cast<int>(i), 0, itemSev);
        m_preflightTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(catStr));
        m_preflightTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(f.pageIndex >= 0 ? QString::number(f.pageIndex + 1) : tr("Document")));
        m_preflightTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(f.title + "\n" + f.description)));
        m_preflightTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::fromStdString(f.remediation)));
        m_preflightTable->setItem(static_cast<int>(i), 5, new QTableWidgetItem(f.isHeuristic ? tr("Heuristic Guideline") : tr("Deterministic Rule")));
    }
}

void PreflightComplianceDialog::onValidatePdfA() {
    if (!m_doc || !m_doc->IsValid()) return;

    auto profile = static_cast<pdf_api::PdfAProfile>(m_pdfaProfileCombo->currentData().toInt());
    auto res = services::PdfAComplianceService::Instance().ValidateCompliance(*m_doc, profile);
    if (!res.has_value()) return;

    const auto& rep = res.value;
    m_pdfaDisclaimer->setText(QString::fromStdString(rep.disclaimer));

    QString profName = (profile == pdf_api::PdfAProfile::PdfA1b) ? "PDF/A-1b" : "PDF/A-2b";
    if (rep.isCompliant) {
        m_pdfaStatusBanner->setText(
            QString("<b>✅ Conformance Verified:</b> Document satisfies all audited technical requirements for <b>%1</b>.").arg(profName)
        );
        m_pdfaStatusBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; padding: 8px 12px; border: 1px solid #86EFAC; border-radius: 4px;");
    } else {
        m_pdfaStatusBanner->setText(
            QString("<b>❌ Non-Compliant:</b> Document fails one or more mandatory clauses for <b>%1</b>. Click 'Convert to Compliant PDF/A' to remediate where viable.").arg(profName)
        );
        m_pdfaStatusBanner->setStyleSheet("background-color: #FEE2E2; color: #991B1B; padding: 8px 12px; border: 1px solid #FCA5A5; border-radius: 4px;");
    }

    m_pdfaRulesTable->setRowCount(static_cast<int>(rep.ruleChecks.size()));
    for (size_t i = 0; i < rep.ruleChecks.size(); ++i) {
        const auto& r = rep.ruleChecks[i];
        m_pdfaRulesTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(r.clause)));
        m_pdfaRulesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(r.description)));

        auto* itemPass = new QTableWidgetItem(r.passed ? "✅ Passed" : "❌ Failed");
        itemPass->setBackground(r.passed ? QColor(220, 252, 231) : QColor(254, 226, 226));
        m_pdfaRulesTable->setItem(static_cast<int>(i), 2, itemPass);

        m_pdfaRulesTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(r.failureDetail)));
    }
}

void PreflightComplianceDialog::onConvertToPdfA() {
    if (!m_doc || !m_doc->IsValid()) return;

    auto profile = static_cast<pdf_api::PdfAProfile>(m_pdfaProfileCombo->currentData().toInt());
    QString profSuffix = (profile == pdf_api::PdfAProfile::PdfA1b) ? "_pdfa1b.pdf" : "_pdfa2b.pdf";

    QString defaultDst;
    if (!m_doc->GetPath().empty()) {
        auto src = m_doc->GetPath();
        defaultDst = QString::fromStdString((src.parent_path() / (src.stem().string() + profSuffix.toStdString())).string());
    } else {
        defaultDst = "document" + profSuffix;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this, tr("Save Compliant PDF/A Document"), defaultDst, tr("PDF Documents (*.pdf)")
    );
    if (savePath.isEmpty()) return;

    pdf_api::PdfAConversionOptions opts;
    opts.targetProfile = profile;
    opts.embedMissingMetadata = true;
    opts.stripProhibitedActions = true;
    opts.injectOutputIntent = true;

    auto res = services::PdfAComplianceService::Instance().ConvertToPdfA(*m_doc, savePath.toStdString(), opts);
    if (res.has_value()) {
        const auto& report = res.value;
        if (report.isCompliant) {
            QMessageBox::information(
                this, tr("PDF/A Conversion Complete"),
                tr("Compliant PDF/A document created successfully!\nSaved to:\n%1").arg(savePath)
            );
        } else {
            QMessageBox::warning(
                this, tr("Partial PDF/A Compliance"),
                tr("PDF/A structure was generated, but some elements (such as unembedded fonts) require source re-authoring to reach 100% compliance.\nSaved to:\n%1").arg(savePath)
            );
        }
    } else {
        QMessageBox::critical(this, tr("Conversion Failed"), QString::fromStdString(res.errorMessage));
    }
}

void PreflightComplianceDialog::onRunAccessibility() {
    if (!m_doc || !m_doc->IsValid()) return;

    auto res = services::AccessibilityService::Instance().InspectAccessibility(*m_doc);
    if (!res.has_value()) return;

    const auto& rep = res.value;
    m_accessDisclaimer->setText(QString::fromStdString(rep.legalDisclaimer));

    QString banner;
    if (rep.passedBasicAccessibility) {
        banner = QString("<b>✅ Basic Accessibility Prerequisites Satisfied:</b> Document is tagged (%1 headings, language: <b>%2</b>) with alternative text on images.")
            .arg(rep.totalHeadingsFound).arg(QString::fromStdString(rep.documentLanguage));
        m_accessStatusBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; padding: 8px 12px; border: 1px solid #86EFAC; border-radius: 4px;");
    } else {
        banner = QString("<b>⚠️ Accessibility Deficiencies Detected:</b> Tagged: <b>%1</b> | Language: <b>%2</b> | Images lacking Alt text: <b>%3</b>.")
            .arg(rep.isTagged ? "YES" : "NO")
            .arg(QString::fromStdString(rep.documentLanguage))
            .arg(rep.totalImagesWithoutAltText);
        m_accessStatusBanner->setStyleSheet("background-color: #FEF3C7; color: #92400E; padding: 8px 12px; border: 1px solid #F59E0B; border-radius: 4px;");
    }
    m_accessStatusBanner->setText(banner);

    m_accessTable->setRowCount(static_cast<int>(rep.findings.size()));
    for (size_t i = 0; i < rep.findings.size(); ++i) {
        const auto& f = rep.findings[i];

        QString sevStr = (f.severity == pdf_api::PreflightSeverity::Error) ? "❌ Error" : "⚠️ Warning";
        auto* itemSev = new QTableWidgetItem(sevStr);
        itemSev->setBackground((f.severity == pdf_api::PreflightSeverity::Error) ? QColor(254, 226, 226) : QColor(254, 243, 199));

        m_accessTable->setItem(static_cast<int>(i), 0, itemSev);
        m_accessTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(f.ruleId)));
        m_accessTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(f.pageIndex >= 0 ? QString::number(f.pageIndex + 1) : tr("Document")));
        m_accessTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(f.issue)));
        m_accessTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::fromStdString(f.remediation)));
    }
}

} // namespace pdfinity::ui
