// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RedactionPreviewDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

namespace pdfinity::ui {

RedactionPreviewDialog::RedactionPreviewDialog(
    const std::vector<pdf_api::RedactionMark>& marks,
    const std::vector<pdf_api::RedactionTargetAnalysis>& analyses,
    const std::filesystem::path& defaultSourcePath,
    QWidget* parent
)
    : QDialog(parent),
      m_sourcePath(defaultSourcePath)
{
    setWindowTitle("Apply Redactions - Pre-Apply Preview & Confirmation");
    resize(760, 520);
    setModal(true);

    setupUi(marks, analyses, defaultSourcePath);
}

void RedactionPreviewDialog::setupUi(
    const std::vector<pdf_api::RedactionMark>& marks,
    const std::vector<pdf_api::RedactionTargetAnalysis>& analyses,
    const std::filesystem::path& defaultSourcePath
) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Header disclaimer banner
    auto* infoLabel = new QLabel(
        "<b>⚠️ Pre-Apply Inspection:</b> The page objects listed below will be permanently purged "
        "from the document's content streams and replaced with solid opaque redaction blocks. "
        "Source document remains untouched.", this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("background-color: #FEF3C7; color: #92400E; border: 1px solid #F59E0B; "
                             "border-radius: 6px; padding: 10px; font-size: 13px;");
    mainLayout->addWidget(infoLabel);

    // Unsupported content banner (if any)
    bool hasUnsupported = false;
    QString unsupportedText;
    for (const auto& a : analyses) {
        if (a.hasUnsupportedContent) {
            hasUnsupported = true;
            for (const auto& w : a.unsupportedWarnings) {
                unsupportedText += "• " + QString::fromStdString(w) + "\n";
            }
        }
    }

    if (hasUnsupported) {
        m_warningBanner = new QLabel(this);
        m_warningBanner->setText("<b>⚠️ Unsupported Content Detected:</b>\n" + unsupportedText +
                                 "Complete destruction cannot be guaranteed without flattening.");
        m_warningBanner->setStyleSheet("background-color: #FEE2E2; color: #991B1B; border: 1px solid #EF4444; "
                                       "border-radius: 6px; padding: 10px; font-size: 12px;");
        m_warningBanner->setWordWrap(true);
        mainLayout->addWidget(m_warningBanner);
    }

    // Table of marks
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Page", "Target Bounds", "Detected Text", "Objects to Purge", "Overlay Text"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_table->setRowCount(static_cast<int>(marks.size()));
    for (size_t i = 0; i < marks.size(); ++i) {
        const auto& m = marks[i];
        m_table->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString("Page %1").arg(m.pageIndex + 1)));

        QString boundsStr = QString("(%1, %2) %3x%4")
            .arg(static_cast<int>(m.rect.x))
            .arg(static_cast<int>(m.rect.y))
            .arg(static_cast<int>(m.rect.width))
            .arg(static_cast<int>(m.rect.height));
        m_table->setItem(static_cast<int>(i), 1, new QTableWidgetItem(boundsStr));

        QString detectedText;
        QString objectsCountStr = "Text: 0, Img: 0";
        if (i < analyses.size()) {
            const auto& a = analyses[i];
            for (const auto& s : a.detectedTextSnippets) {
                if (!detectedText.isEmpty()) detectedText += " | ";
                detectedText += QString::fromStdString(s);
            }
            objectsCountStr = QString("Text: %1, Img: %2, Path: %3")
                .arg(a.intersectingTextObjects)
                .arg(a.intersectingImageObjects)
                .arg(a.intersectingPathObjects);
        }
        if (detectedText.isEmpty()) detectedText = "<No text detected>";
        m_table->setItem(static_cast<int>(i), 2, new QTableWidgetItem(detectedText));
        m_table->setItem(static_cast<int>(i), 3, new QTableWidgetItem(objectsCountStr));
        m_table->setItem(static_cast<int>(i), 4, new QTableWidgetItem(m.overlayText.empty() ? "[Solid Black]" : QString::fromStdString(m.overlayText)));
    }
    mainLayout->addWidget(m_table);

    // Destination file selector
    auto* destLayout = new QHBoxLayout();
    destLayout->addWidget(new QLabel("Save Redacted Copy To:", this));

    std::filesystem::path defDst = defaultSourcePath;
    if (!defaultSourcePath.empty()) {
        std::string stem = defaultSourcePath.stem().string();
        defDst = defaultSourcePath.parent_path() / (stem + "_redacted.pdf");
    } else {
        defDst = std::filesystem::current_path() / "document_redacted.pdf";
    }
    m_destinationPath = defDst;

    m_destPathEdit = new QLineEdit(QString::fromStdString(defDst.string()), this);
    destLayout->addWidget(m_destPathEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &RedactionPreviewDialog::onBrowseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Explicit confirmation checkbox
    m_confirmCheckbox = new QCheckBox(
        "I understand that redaction is permanent and irreversible. "
        "The underlying content will be destroyed in the output document.", this
    );
    m_confirmCheckbox->setStyleSheet("font-weight: bold; color: #1E293B; margin-top: 6px;");
    connect(m_confirmCheckbox, &QCheckBox::toggled, this, &RedactionPreviewDialog::onConfirmToggled);
    mainLayout->addWidget(m_confirmCheckbox);

    // Button row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_applyBtn = new QPushButton("Apply Redactions & Save Copy...", this);
    m_applyBtn->setStyleSheet("background-color: #DC2626; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px;");
    m_applyBtn->setEnabled(false); // Disabled until confirmed!
    connect(m_applyBtn, &QPushButton::clicked, this, &RedactionPreviewDialog::onApplyClicked);
    btnLayout->addWidget(m_applyBtn);

    mainLayout->addLayout(btnLayout);
}

void RedactionPreviewDialog::onBrowseDestination() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Redacted PDF Copy",
        m_destPathEdit ? m_destPathEdit->text() : "",
        "PDF Documents (*.pdf)"
    );
    if (!path.isEmpty()) {
        m_destPathEdit->setText(path);
        m_destinationPath = std::filesystem::path(path.toStdString());
    }
}

void RedactionPreviewDialog::onConfirmToggled(bool checked) {
    if (m_applyBtn) {
        m_applyBtn->setEnabled(checked);
    }
}

void RedactionPreviewDialog::onApplyClicked() {
    if (!m_destPathEdit) return;
    std::filesystem::path chosenDst(m_destPathEdit->text().toStdString());

    std::error_code ec;
    if (std::filesystem::equivalent(m_sourcePath, chosenDst, ec)) {
        QMessageBox::critical(
            this, "Error: In-Place Overwrite Prohibited",
            "Secure redaction requires writing to a separate destination file to guarantee source preservation. Please select a different output filename."
        );
        return;
    }

    m_destinationPath = chosenDst;
    accept();
}

std::filesystem::path RedactionPreviewDialog::GetDestinationPath() const {
    return m_destinationPath;
}

bool RedactionPreviewDialog::HasConfirmedIrreversible() const {
    return m_confirmCheckbox ? m_confirmCheckbox->isChecked() : false;
}

} // namespace pdfinity::ui
