// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ConversionCenterDialog.hpp"
#include "services/ConversionService.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

namespace pdfinity::ui {

ConversionCenterDialog::ConversionCenterDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    std::shared_ptr<pdf_api::IPdfEngine> engine,
    QWidget* parent
)
    : QDialog(parent),
      m_doc(std::move(doc)),
      m_engine(std::move(engine))
{
    setWindowTitle("Document Conversion Center");
    resize(720, 560);
    setModal(true);

    setupUi();
}

void ConversionCenterDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    setupExportTab();
    setupCreateTab();

    mainLayout->addWidget(m_tabWidget);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
}

void ConversionCenterDialog::setupExportTab() {
    auto* exportWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(exportWidget);
    layout->setSpacing(14);

    // Section 1: Export to Images
    auto* imgGroup = new QGroupBox("1. Export PDF Pages to Images", exportWidget);
    auto* imgLayout = new QVBoxLayout(imgGroup);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("Format:", imgGroup));
    m_imgFormatCombo = new QComboBox(imgGroup);
    m_imgFormatCombo->addItem("PNG (Lossless)", static_cast<int>(pdf_api::ExportImageFormat::PNG));
    m_imgFormatCombo->addItem("JPEG (Compact)", static_cast<int>(pdf_api::ExportImageFormat::JPEG));
    m_imgFormatCombo->addItem("TIFF (Print / Archival)", static_cast<int>(pdf_api::ExportImageFormat::TIFF));
    m_imgFormatCombo->addItem("BMP (Bitmap)", static_cast<int>(pdf_api::ExportImageFormat::BMP));
    row1->addWidget(m_imgFormatCombo);

    row1->addWidget(new QLabel("DPI:", imgGroup));
    m_imgDpiCombo = new QComboBox(imgGroup);
    m_imgDpiCombo->addItem("150 DPI (Standard Screen)", 150);
    m_imgDpiCombo->addItem("300 DPI (High Quality Print)", 300);
    m_imgDpiCombo->addItem("72 DPI (Thumbnail)", 72);
    row1->addWidget(m_imgDpiCombo);
    row1->addStretch();
    imgLayout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("Output Folder:", imgGroup));
    m_imgOutDirEdit = new QLineEdit(imgGroup);
    if (m_doc && !m_doc->GetPath().empty()) {
        m_imgOutDirEdit->setText(QString::fromStdString(m_doc->GetPath().parent_path().string()));
    } else {
        m_imgOutDirEdit->setText(QString::fromStdString(std::filesystem::current_path().string()));
    }
    row2->addWidget(m_imgOutDirEdit, 1);

    auto* browseImgDir = new QPushButton("Browse...", imgGroup);
    connect(browseImgDir, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Images Output Directory", m_imgOutDirEdit->text());
        if (!dir.isEmpty()) m_imgOutDirEdit->setText(dir);
    });
    row2->addWidget(browseImgDir);

    auto* exportImgBtn = new QPushButton("Export Images", imgGroup);
    exportImgBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 5px 12px;");
    connect(exportImgBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onExportImages);
    row2->addWidget(exportImgBtn);
    imgLayout->addLayout(row2);
    layout->addWidget(imgGroup);

    // Section 2: Export to Text
    auto* txtGroup = new QGroupBox("2. Export Document Text (UTF-8)", exportWidget);
    auto* txtLayout = new QHBoxLayout(txtGroup);

    txtLayout->addWidget(new QLabel("Output Text File:", txtGroup));
    m_textOutPathEdit = new QLineEdit(txtGroup);
    if (m_doc && !m_doc->GetPath().empty()) {
        auto src = m_doc->GetPath();
        m_textOutPathEdit->setText(QString::fromStdString((src.parent_path() / (src.stem().string() + ".txt")).string()));
    }
    txtLayout->addWidget(m_textOutPathEdit, 1);

    auto* browseTxt = new QPushButton("Browse...", txtGroup);
    connect(browseTxt, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save Text File", m_textOutPathEdit->text(), "Text Files (*.txt)");
        if (!file.isEmpty()) m_textOutPathEdit->setText(file);
    });
    txtLayout->addWidget(browseTxt);

    auto* exportTxtBtn = new QPushButton("Export Text", txtGroup);
    exportTxtBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 5px 12px;");
    connect(exportTxtBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onExportText);
    txtLayout->addWidget(exportTxtBtn);
    layout->addWidget(txtGroup);

    // Section 3: Export to HTML
    auto* htmlGroup = new QGroupBox("3. Export to Semantic HTML (Explicit Fidelity Disclosure)", exportWidget);
    auto* htmlLayout = new QHBoxLayout(htmlGroup);

    htmlLayout->addWidget(new QLabel("Output HTML File:", htmlGroup));
    m_htmlOutPathEdit = new QLineEdit(htmlGroup);
    if (m_doc && !m_doc->GetPath().empty()) {
        auto src = m_doc->GetPath();
        m_htmlOutPathEdit->setText(QString::fromStdString((src.parent_path() / (src.stem().string() + ".html")).string()));
    }
    htmlLayout->addWidget(m_htmlOutPathEdit, 1);

    auto* browseHtml = new QPushButton("Browse...", htmlGroup);
    connect(browseHtml, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save HTML File", m_htmlOutPathEdit->text(), "HTML Documents (*.html)");
        if (!file.isEmpty()) m_htmlOutPathEdit->setText(file);
    });
    htmlLayout->addWidget(browseHtml);

    auto* exportHtmlBtn = new QPushButton("Export HTML", htmlGroup);
    exportHtmlBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 5px 12px;");
    connect(exportHtmlBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onExportHtml);
    htmlLayout->addWidget(exportHtmlBtn);
    layout->addWidget(htmlGroup);

    layout->addStretch();
    m_tabWidget->addTab(exportWidget, "Export Document");
}

void ConversionCenterDialog::setupCreateTab() {
    auto* createWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(createWidget);
    layout->setSpacing(14);

    // Group A: Images to PDF
    auto* imgToPdfGroup = new QGroupBox("A. Create PDF from Images", createWidget);
    auto* imgToPdfLayout = new QVBoxLayout(imgToPdfGroup);

    m_imageListWidget = new QListWidget(imgToPdfGroup);
    m_imageListWidget->setMaximumHeight(110);
    imgToPdfLayout->addWidget(m_imageListWidget);

    auto* listBtnLayout = new QHBoxLayout();
    auto* addImgBtn = new QPushButton("Add Images...", imgToPdfGroup);
    connect(addImgBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onAddImages);
    listBtnLayout->addWidget(addImgBtn);

    auto* remImgBtn = new QPushButton("Remove", imgToPdfGroup);
    connect(remImgBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onRemoveImage);
    listBtnLayout->addWidget(remImgBtn);

    auto* upBtn = new QPushButton("Move Up", imgToPdfGroup);
    connect(upBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onMoveImageUp);
    listBtnLayout->addWidget(upBtn);

    auto* downBtn = new QPushButton("Move Down", imgToPdfGroup);
    connect(downBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onMoveImageDown);
    listBtnLayout->addWidget(downBtn);
    listBtnLayout->addStretch();
    imgToPdfLayout->addLayout(listBtnLayout);

    auto* optRow = new QHBoxLayout();
    optRow->addWidget(new QLabel("Page Sizing:", imgToPdfGroup));
    m_pageFitCombo = new QComboBox(imgToPdfGroup);
    m_pageFitCombo->addItem("Match Image Resolution & Size", static_cast<int>(pdf_api::ImagePageFit::MatchImageSize));
    m_pageFitCombo->addItem("Fit to Standard A4", static_cast<int>(pdf_api::ImagePageFit::FitA4));
    m_pageFitCombo->addItem("Fit to Standard US Letter", static_cast<int>(pdf_api::ImagePageFit::FitLetter));
    optRow->addWidget(m_pageFitCombo, 1);
    imgToPdfLayout->addLayout(optRow);

    auto* saveRow = new QHBoxLayout();
    saveRow->addWidget(new QLabel("Output PDF:", imgToPdfGroup));
    m_imgToPdfOutEdit = new QLineEdit(QString::fromStdString((std::filesystem::current_path() / "images_combined.pdf").string()), imgToPdfGroup);
    saveRow->addWidget(m_imgToPdfOutEdit, 1);

    auto* browseSave = new QPushButton("Browse...", imgToPdfGroup);
    connect(browseSave, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save Combined PDF", m_imgToPdfOutEdit->text(), "PDF Documents (*.pdf)");
        if (!file.isEmpty()) m_imgToPdfOutEdit->setText(file);
    });
    saveRow->addWidget(browseSave);

    auto* createPdfBtn = new QPushButton("Create PDF", imgToPdfGroup);
    createPdfBtn->setStyleSheet("background-color: #16A34A; color: white; font-weight: bold; padding: 5px 14px;");
    connect(createPdfBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onCreatePdfFromImages);
    saveRow->addWidget(createPdfBtn);
    imgToPdfLayout->addLayout(saveRow);

    layout->addWidget(imgToPdfGroup);

    // Group B: Office to PDF
    auto* officeGroup = new QGroupBox("B. Convert Office Documents (Word, Excel, PowerPoint)", createWidget);
    auto* officeLayout = new QVBoxLayout(officeGroup);

    m_officeStatusBanner = new QLabel(officeGroup);
    m_officeStatusBanner->setWordWrap(true);
    officeLayout->addWidget(m_officeStatusBanner);

    auto* offInRow = new QHBoxLayout();
    offInRow->addWidget(new QLabel("Office File:", officeGroup));
    m_officeInputEdit = new QLineEdit(officeGroup);
    m_officeInputEdit->setPlaceholderText("Select .docx, .xlsx, or .pptx document...");
    offInRow->addWidget(m_officeInputEdit, 1);

    auto* browseOffIn = new QPushButton("Browse...", officeGroup);
    connect(browseOffIn, &QPushButton::clicked, this, &ConversionCenterDialog::onBrowseOfficeInput);
    offInRow->addWidget(browseOffIn);
    officeLayout->addLayout(offInRow);

    auto* offOutRow = new QHBoxLayout();
    offOutRow->addWidget(new QLabel("Output PDF:", officeGroup));
    m_officeOutputEdit = new QLineEdit(officeGroup);
    offOutRow->addWidget(m_officeOutputEdit, 1);

    auto* browseOffOut = new QPushButton("Browse...", officeGroup);
    connect(browseOffOut, &QPushButton::clicked, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "Save Converted PDF", m_officeOutputEdit->text(), "PDF Documents (*.pdf)");
        if (!file.isEmpty()) m_officeOutputEdit->setText(file);
    });
    offOutRow->addWidget(browseOffOut);

    m_officeConvertBtn = new QPushButton("Convert to PDF", officeGroup);
    m_officeConvertBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 5px 14px;");
    connect(m_officeConvertBtn, &QPushButton::clicked, this, &ConversionCenterDialog::onConvertOfficeDoc);
    offOutRow->addWidget(m_officeConvertBtn);
    officeLayout->addLayout(offOutRow);

    layout->addWidget(officeGroup);
    updateOfficeEngineStatus();

    m_tabWidget->addTab(createWidget, "Create PDF");
}

void ConversionCenterDialog::updateOfficeEngineStatus() {
    auto status = services::ConversionService::Instance().DetectOfficeEngine();
    if (status.isAvailable) {
        m_officeStatusBanner->setText(
            QString("<b>✅ Office Engine Detected:</b> %1 (%2) is available.")
                .arg(QString::fromStdString(status.engineName))
                .arg(QString::fromStdString(status.version))
        );
        m_officeStatusBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; border: 1px solid #86EFAC; "
                                            "border-radius: 6px; padding: 8px; font-size: 12px;");
        m_officeConvertBtn->setEnabled(true);
        m_officeConvertBtn->setToolTip("Convert selected Office document to PDF using LibreOffice");
    } else {
        // Verification: Missing optional engines produce actionable messages, NOT fake success!
        m_officeStatusBanner->setText(
            QString("<b>⚠️ Office Engine Not Found:</b><br>%1")
                .arg(QString::fromStdString(status.installGuidance).replace("\n", "<br>"))
        );
        m_officeStatusBanner->setStyleSheet("background-color: #FEF3C7; color: #92400E; border: 1px solid #F59E0B; "
                                            "border-radius: 6px; padding: 8px; font-size: 11px;");
        m_officeConvertBtn->setEnabled(false);
        m_officeConvertBtn->setToolTip("LibreOffice engine is missing. Install LibreOffice to enable Office conversion.");
    }
}

void ConversionCenterDialog::onExportImages() {
    if (!m_doc || !m_doc->IsValid()) {
        QMessageBox::critical(this, "Error", "No document open to export.");
        return;
    }
    std::filesystem::path outDir(m_imgOutDirEdit->text().toStdString());
    std::string baseName = m_doc->GetPath().stem().string();

    pdf_api::PdfToImageOptions opt;
    opt.format = static_cast<pdf_api::ExportImageFormat>(m_imgFormatCombo->currentData().toInt());
    opt.dpi = m_imgDpiCombo->currentData().toInt();

    auto res = services::ConversionService::Instance().ExportPdfToImages(*m_doc, outDir, baseName, opt);
    if (res.has_value()) {
        QMessageBox::information(
            this, "Export Complete",
            QString("Successfully exported %1 page image(s) to:\n%2")
                .arg(res.value.size())
                .arg(QString::fromStdString(outDir.string()))
        );
    } else {
        QMessageBox::critical(this, "Export Failed", QString::fromStdString(res.errorMessage));
    }
}

void ConversionCenterDialog::onExportText() {
    if (!m_doc || !m_doc->IsValid()) {
        QMessageBox::critical(this, "Error", "No document open to export.");
        return;
    }
    std::filesystem::path outFile(m_textOutPathEdit->text().toStdString());
    auto res = services::ConversionService::Instance().ExportPdfToText(*m_doc, outFile);
    if (res.has_value()) {
        QMessageBox::information(
            this, "Export Complete",
            QString("Document text successfully exported to:\n%1").arg(QString::fromStdString(outFile.string()))
        );
    } else {
        QMessageBox::critical(this, "Export Failed", QString::fromStdString(res.errorMessage));
    }
}

void ConversionCenterDialog::onExportHtml() {
    if (!m_doc || !m_doc->IsValid()) {
        QMessageBox::critical(this, "Error", "No document open to export.");
        return;
    }
    std::filesystem::path outFile(m_htmlOutPathEdit->text().toStdString());
    auto res = services::ConversionService::Instance().ExportPdfToHtml(*m_doc, outFile);
    if (res.has_value()) {
        QMessageBox::information(
            this, "Export Complete",
            QString("Document exported to HTML with fidelity disclaimer to:\n%1")
                .arg(QString::fromStdString(outFile.string()))
        );
    } else {
        QMessageBox::critical(this, "Export Failed", QString::fromStdString(res.errorMessage));
    }
}

void ConversionCenterDialog::onAddImages() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select Images to Convert", "", "Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff)"
    );
    for (const auto& f : files) {
        m_imageListWidget->addItem(f);
    }
}

void ConversionCenterDialog::onRemoveImage() {
    auto items = m_imageListWidget->selectedItems();
    for (auto* item : items) {
        delete m_imageListWidget->takeItem(m_imageListWidget->row(item));
    }
}

void ConversionCenterDialog::onMoveImageUp() {
    int row = m_imageListWidget->currentRow();
    if (row > 0) {
        auto* item = m_imageListWidget->takeItem(row);
        m_imageListWidget->insertItem(row - 1, item);
        m_imageListWidget->setCurrentRow(row - 1);
    }
}

void ConversionCenterDialog::onMoveImageDown() {
    int row = m_imageListWidget->currentRow();
    if (row >= 0 && row < m_imageListWidget->count() - 1) {
        auto* item = m_imageListWidget->takeItem(row);
        m_imageListWidget->insertItem(row + 1, item);
        m_imageListWidget->setCurrentRow(row + 1);
    }
}

void ConversionCenterDialog::onCreatePdfFromImages() {
    if (m_imageListWidget->count() == 0) {
        QMessageBox::information(this, "No Images", "Please add one or more images to convert.");
        return;
    }

    std::vector<std::filesystem::path> paths;
    for (int i = 0; i < m_imageListWidget->count(); ++i) {
        paths.push_back(m_imageListWidget->item(i)->text().toStdString());
    }

    std::filesystem::path outFile(m_imgToPdfOutEdit->text().toStdString());
    pdf_api::ImageToPdfOptions opt;
    opt.pageFit = static_cast<pdf_api::ImagePageFit>(m_pageFitCombo->currentData().toInt());

    auto res = services::ConversionService::Instance().ConvertImagesToPdf(paths, outFile, opt, m_engine);
    if (res.has_value()) {
        m_resultPath = outFile;
        QMessageBox::information(
            this, "PDF Created",
            QString("Successfully created PDF from %1 image(s):\n%2")
                .arg(paths.size())
                .arg(QString::fromStdString(outFile.string()))
        );
        accept();
    } else {
        QMessageBox::critical(this, "Creation Failed", QString::fromStdString(res.errorMessage));
    }
}

void ConversionCenterDialog::onBrowseOfficeInput() {
    QString file = QFileDialog::getOpenFileName(
        this, "Select Office Document", "", "Office Documents (*.docx *.doc *.xlsx *.xls *.pptx *.ppt)"
    );
    if (!file.isEmpty()) {
        m_officeInputEdit->setText(file);
        QFileInfo fi(file);
        m_officeOutputEdit->setText(fi.path() + "/" + fi.completeBaseName() + ".pdf");
    }
}

void ConversionCenterDialog::onConvertOfficeDoc() {
    std::filesystem::path inPath(m_officeInputEdit->text().toStdString());
    std::filesystem::path outPath(m_officeOutputEdit->text().toStdString());

    auto res = services::ConversionService::Instance().ConvertOfficeToPdf(inPath, outPath);
    if (res.has_value()) {
        m_resultPath = outPath;
        QMessageBox::information(
            this, "Conversion Complete",
            QString("Office document successfully converted to PDF:\n%1")
                .arg(QString::fromStdString(outPath.string()))
        );
        accept();
    } else {
        QMessageBox::critical(this, "Conversion Failed", QString::fromStdString(res.errorMessage));
    }
}

std::filesystem::path ConversionCenterDialog::GetResultPath() const {
    return m_resultPath;
}

} // namespace pdfinity::ui
