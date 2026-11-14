// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "OcrStudioDialog.hpp"
#include "services/OcrService.hpp"
#include "domain/PageRangeParser.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

namespace pdfinity::ui {

OcrStudioDialog::OcrStudioDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
)
    : QDialog(parent),
      m_doc(std::move(doc))
{
    setWindowTitle("OCR Studio - Generate Searchable PDF");
    resize(640, 500);
    setModal(true);

    setupUi();
    updateEngineStatus();
}

void OcrStudioDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Status Banner
    m_statusBanner = new QLabel(this);
    m_statusBanner->setWordWrap(true);
    mainLayout->addWidget(m_statusBanner);

    // OCR Configuration Group
    auto* optGroup = new QGroupBox("Recognition & Page Options", this);
    auto* optLayout = new QVBoxLayout(optGroup);

    // Language
    auto* langLayout = new QHBoxLayout();
    langLayout->addWidget(new QLabel("Recognition Language:", this));
    m_langCombo = new QComboBox(this);
    m_langCombo->addItem("English (eng)", "eng");
    m_langCombo->addItem("German (deu)", "deu");
    m_langCombo->addItem("French (fra)", "fra");
    m_langCombo->addItem("Spanish (spa)", "spa");
    m_langCombo->addItem("Simplified Chinese (chi_sim)", "chi_sim");
    m_langCombo->addItem("Japanese (jpn)", "jpn");
    langLayout->addWidget(m_langCombo, 1);
    optLayout->addLayout(langLayout);

    // DPI
    auto* dpiLayout = new QHBoxLayout();
    dpiLayout->addWidget(new QLabel("Recognition DPI:", this));
    m_dpiCombo = new QComboBox(this);
    m_dpiCombo->addItem("300 DPI (High Fidelity, Recommended)", 300);
    m_dpiCombo->addItem("150 DPI (Fast / Screen)", 150);
    m_dpiCombo->addItem("600 DPI (Ultra Fine)", 600);
    dpiLayout->addWidget(m_dpiCombo, 1);
    optLayout->addLayout(dpiLayout);

    // Page Range
    auto* rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(new QLabel("Pages to Process:", this));
    m_pageRangeCombo = new QComboBox(this);
    m_pageRangeCombo->addItem("All Pages");
    m_pageRangeCombo->addItem("Custom Range...");
    rangeLayout->addWidget(m_pageRangeCombo, 1);

    m_customRangeEdit = new QLineEdit(this);
    m_customRangeEdit->setPlaceholderText("e.g. 1-5, 8");
    m_customRangeEdit->setEnabled(false);
    connect(m_pageRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        if (m_customRangeEdit) m_customRangeEdit->setEnabled(idx == 1);
    });
    rangeLayout->addWidget(m_customRangeEdit, 1);
    optLayout->addLayout(rangeLayout);

    // Fidelity / Searchable layer preservation
    m_preserveImageCheck = new QCheckBox(
        "Preserve original page appearance & inject invisible searchable text layer", this
    );
    m_preserveImageCheck->setChecked(true);
    optLayout->addWidget(m_preserveImageCheck);

    mainLayout->addWidget(optGroup);

    // Destination Path Group
    auto* destLayout = new QHBoxLayout();
    destLayout->addWidget(new QLabel("Save Searchable PDF To:", this));

    std::filesystem::path defDst;
    if (m_doc && !m_doc->GetPath().empty()) {
        auto srcPath = m_doc->GetPath();
        defDst = srcPath.parent_path() / (srcPath.stem().string() + "_searchable.pdf");
    } else {
        defDst = std::filesystem::current_path() / "document_searchable.pdf";
    }
    m_destinationPath = defDst;

    m_destPathEdit = new QLineEdit(QString::fromStdString(defDst.string()), this);
    destLayout->addWidget(m_destPathEdit, 1);

    auto* browseBtn = new QPushButton("Browse...", this);
    connect(browseBtn, &QPushButton::clicked, this, &OcrStudioDialog::onBrowseDestination);
    destLayout->addWidget(browseBtn);
    mainLayout->addLayout(destLayout);

    // Progress
    m_progressLabel = new QLabel("Ready to start OCR processing.", this);
    m_progressLabel->setStyleSheet("color: #64748B; font-size: 12px;");
    mainLayout->addWidget(m_progressLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    // Button Row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_startBtn = new QPushButton("Start OCR Processing...", this);
    m_startBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 8px 18px; border-radius: 4px;");
    connect(m_startBtn, &QPushButton::clicked, this, &OcrStudioDialog::onStartOcr);
    btnLayout->addWidget(m_startBtn);

    mainLayout->addLayout(btnLayout);
}

void OcrStudioDialog::updateEngineStatus() {
    auto status = services::OcrService::Instance().DetectOcrEngine();
    if (status.isAvailable) {
        m_statusBanner->setText(
            QString("<b>✅ OCR Engine Ready:</b> %1 (%2) detected in system PATH.")
                .arg(QString::fromStdString(status.engineName))
                .arg(QString::fromStdString(status.engineVersion.empty() ? "CLI Ready" : status.engineVersion))
        );
        m_statusBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; border: 1px solid #86EFAC; "
                                      "border-radius: 6px; padding: 10px; font-size: 13px;");
    } else {
        m_statusBanner->setText(
            QString("<b>⚠️ OCR Engine Notice:</b> Tesseract OCR was not found in system PATH.<br>%1<br>"
                    "<i>(Synthetic pattern OCR fallback is enabled for testing and verification)</i>")
                .arg(QString::fromStdString(status.installGuidance).replace("\n", "<br>"))
        );
        m_statusBanner->setStyleSheet("background-color: #FEF3C7; color: #92400E; border: 1px solid #F59E0B; "
                                      "border-radius: 6px; padding: 10px; font-size: 12px;");
    }
}

void OcrStudioDialog::onBrowseDestination() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Searchable PDF",
        m_destPathEdit ? m_destPathEdit->text() : "",
        "PDF Documents (*.pdf)"
    );
    if (!path.isEmpty()) {
        m_destPathEdit->setText(path);
        m_destinationPath = std::filesystem::path(path.toStdString());
    }
}

void OcrStudioDialog::onStartOcr() {
    if (!m_doc || !m_doc->IsValid()) {
        QMessageBox::critical(this, "Error", "No valid PDF document is open.");
        return;
    }

    if (m_destPathEdit) {
        m_destinationPath = std::filesystem::path(m_destPathEdit->text().toStdString());
    }

    std::error_code ec;
    if (std::filesystem::equivalent(m_doc->GetPath(), m_destinationPath, ec)) {
        QMessageBox::critical(
            this, "Error: In-Place Overwrite Prohibited",
            "Please select a separate destination file to preserve the original source document."
        );
        return;
    }

    auto options = GetOptions();

    m_startBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);

    auto progressCb = [this](int cur, int tot, const std::string& msg) {
        int percent = (tot > 0) ? (cur * 100 / tot) : 0;
        m_progressBar->setValue(percent);
        m_progressLabel->setText(QString::fromStdString(msg));
        QApplication::processEvents();
    };

    auto res = services::OcrService::Instance().ProcessDocumentOcr(
        *m_doc, m_destinationPath, options, progressCb
    );

    m_startBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);

    if (res.has_value()) {
        QMessageBox::information(
            this, "OCR Complete",
            QString("Searchable PDF created successfully!\nSaved to:\n%1")
                .arg(QString::fromStdString(m_destinationPath.string()))
        );
        accept();
    } else {
        QMessageBox::critical(
            this, "OCR Failed",
            QString::fromStdString(res.errorMessage)
        );
    }
}

std::filesystem::path OcrStudioDialog::GetDestinationPath() const {
    return m_destinationPath;
}

pdf_api::OcrOptions OcrStudioDialog::GetOptions() const {
    pdf_api::OcrOptions opt;
    if (m_langCombo) {
        opt.language = m_langCombo->currentData().toString().toStdString();
    }
    if (m_dpiCombo) {
        opt.targetDpi = m_dpiCombo->currentData().toInt();
    }
    if (m_preserveImageCheck) {
        opt.preserveOriginalImage = m_preserveImageCheck->isChecked();
        opt.makeSearchablePdf = m_preserveImageCheck->isChecked();
    }

    if (m_pageRangeCombo && m_pageRangeCombo->currentIndex() == 1 && m_customRangeEdit) {
        int pageCount = m_doc ? m_doc->GetPageCount() : 1;
        auto parseRes = domain::PageRangeParser::Parse(m_customRangeEdit->text().toStdString(), pageCount);
        if (parseRes.has_value()) {
            opt.pageIndices = parseRes.value;
        }
    }

    return opt;
}

} // namespace pdfinity::ui
