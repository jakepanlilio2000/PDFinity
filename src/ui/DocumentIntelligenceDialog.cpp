// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentIntelligenceDialog.hpp"
#include "services/DocumentIntelligenceService.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>

namespace pdfinity::ui {

DocumentIntelligenceDialog::DocumentIntelligenceDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
)
    : QDialog(parent),
      m_doc(std::move(doc))
{
    setWindowTitle(tr("Document Intelligence & Inspector"));
    resize(820, 620);
    setModal(true);

    setupUi();
    populateData();
}

void DocumentIntelligenceDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    setupMetadataTab();
    setupFontsTab();
    setupImagesTab();
    setupBoxesTab();
    setupAttachmentsTab();

    mainLayout->addWidget(m_tabWidget);

    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &DocumentIntelligenceDialog::onRefreshAnalysis);
    btnLayout->addWidget(refreshBtn);

    btnLayout->addStretch();

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void DocumentIntelligenceDialog::setupMetadataTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    auto* infoGroup = new QGroupBox(tr("Standard Document Info Dictionary"), tab);
    auto* grid = new QGridLayout(infoGroup);

    grid->addWidget(new QLabel(tr("Title:"), infoGroup), 0, 0);
    m_titleEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_titleEdit, 0, 1);

    grid->addWidget(new QLabel(tr("Author:"), infoGroup), 0, 2);
    m_authorEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_authorEdit, 0, 3);

    grid->addWidget(new QLabel(tr("Subject:"), infoGroup), 1, 0);
    m_subjectEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_subjectEdit, 1, 1);

    grid->addWidget(new QLabel(tr("Keywords:"), infoGroup), 1, 2);
    m_keywordsEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_keywordsEdit, 1, 3);

    grid->addWidget(new QLabel(tr("Creator:"), infoGroup), 2, 0);
    m_creatorEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_creatorEdit, 2, 1);

    grid->addWidget(new QLabel(tr("Producer:"), infoGroup), 2, 2);
    m_producerEdit = new QLineEdit(infoGroup);
    grid->addWidget(m_producerEdit, 2, 3);

    m_datesLabel = new QLabel(tr("Creation: — | Modified: —"), infoGroup);
    m_datesLabel->setStyleSheet("color: #64748B; font-size: 11px;");
    grid->addWidget(m_datesLabel, 3, 0, 1, 4);

    layout->addWidget(infoGroup);

    auto* xmpGroup = new QGroupBox(tr("Raw Extensible Metadata Platform (XMP) Packet"), tab);
    auto* xmpLayout = new QVBoxLayout(xmpGroup);

    m_xmpTextEdit = new QTextEdit(xmpGroup);
    m_xmpTextEdit->setFontFamily("Consolas");
    m_xmpTextEdit->setFontPointSize(9.0);
    xmpLayout->addWidget(m_xmpTextEdit);

    layout->addWidget(xmpGroup, 1);

    auto* actLayout = new QHBoxLayout();
    actLayout->addStretch();
    m_saveMetaBtn = new QPushButton(tr("Save Metadata to Copy..."), tab);
    m_saveMetaBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 14px;");
    connect(m_saveMetaBtn, &QPushButton::clicked, this, &DocumentIntelligenceDialog::onSaveMetadataClicked);
    actLayout->addWidget(m_saveMetaBtn);
    layout->addLayout(actLayout);

    m_tabWidget->addTab(tab, tr("Metadata & XMP"));
}

void DocumentIntelligenceDialog::setupFontsTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_fontsTable = new QTableWidget(tab);
    m_fontsTable->setColumnCount(6);
    m_fontsTable->setHorizontalHeaderLabels({
        tr("Base Font Name"), tr("Family"), tr("Embedded"), tr("Subset"), tr("Type"), tr("Pages Used")
    });
    m_fontsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fontsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fontsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fontsTable->setAlternatingRowColors(true);

    layout->addWidget(m_fontsTable);
    m_tabWidget->addTab(tab, tr("Fonts & Encodings"));
}

void DocumentIntelligenceDialog::setupImagesTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_imagesTable = new QTableWidget(tab);
    m_imagesTable->setColumnCount(7);
    m_imagesTable->setHorizontalHeaderLabels({
        tr("Page"), tr("Dimensions (px)"), tr("Effective DPI"), tr("Color Space"), tr("BPP"), tr("Filter"), tr("ICC Profile")
    });
    m_imagesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_imagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_imagesTable->setAlternatingRowColors(true);

    layout->addWidget(m_imagesTable);
    m_tabWidget->addTab(tab, tr("Images & Color Spaces"));
}

void DocumentIntelligenceDialog::setupBoxesTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_boxesTable = new QTableWidget(tab);
    m_boxesTable->setColumnCount(7);
    m_boxesTable->setHorizontalHeaderLabels({
        tr("Page"), tr("MediaBox (pt)"), tr("CropBox (pt)"), tr("BleedBox (pt)"), tr("TrimBox (pt)"), tr("ArtBox (pt)"), tr("Rotation")
    });
    m_boxesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_boxesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_boxesTable->setAlternatingRowColors(true);

    layout->addWidget(m_boxesTable);
    m_tabWidget->addTab(tab, tr("Page Dimensions & Boxes"));
}

void DocumentIntelligenceDialog::setupAttachmentsTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    auto* attGroup = new QGroupBox(tr("Embedded File Attachments"), tab);
    auto* attLayout = new QVBoxLayout(attGroup);

    m_attachmentsTable = new QTableWidget(attGroup);
    m_attachmentsTable->setColumnCount(5);
    m_attachmentsTable->setHorizontalHeaderLabels({
        tr("#"), tr("File Name"), tr("Description"), tr("MIME Type"), tr("Size")
    });
    m_attachmentsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_attachmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_attachmentsTable->setAlternatingRowColors(true);
    attLayout->addWidget(m_attachmentsTable);

    auto* attBtnLayout = new QHBoxLayout();
    m_extractAttachmentBtn = new QPushButton(tr("Export Selected Attachment..."), attGroup);
    connect(m_extractAttachmentBtn, &QPushButton::clicked, this, &DocumentIntelligenceDialog::onExtractAttachmentClicked);
    attBtnLayout->addWidget(m_extractAttachmentBtn);

    m_addAttachmentBtn = new QPushButton(tr("Embed New Attachment..."), attGroup);
    connect(m_addAttachmentBtn, &QPushButton::clicked, this, &DocumentIntelligenceDialog::onAddAttachmentClicked);
    attBtnLayout->addWidget(m_addAttachmentBtn);

    attBtnLayout->addStretch();
    attLayout->addLayout(attBtnLayout);
    layout->addWidget(attGroup);

    auto* layersGroup = new QGroupBox(tr("Optional Content Groups (Document Layers)"), tab);
    auto* layLayout = new QVBoxLayout(layersGroup);
    m_layersStatusLabel = new QLabel(layersGroup);
    m_layersStatusLabel->setWordWrap(true);
    layLayout->addWidget(m_layersStatusLabel);
    layout->addWidget(layersGroup);

    m_tabWidget->addTab(tab, tr("Attachments & Layers"));
}

void DocumentIntelligenceDialog::populateData() {
    if (!m_doc || !m_doc->IsValid()) return;

    auto res = services::DocumentIntelligenceService::Instance().AnalyzeDocument(*m_doc);
    if (!res.has_value()) return;

    const auto& intel = res.value;

    // Metadata
    m_titleEdit->setText(QString::fromStdString(intel.metadata.title));
    m_authorEdit->setText(QString::fromStdString(intel.metadata.author));
    m_subjectEdit->setText(QString::fromStdString(intel.metadata.subject));
    m_keywordsEdit->setText(QString::fromStdString(intel.metadata.keywords));
    m_creatorEdit->setText(QString::fromStdString(intel.metadata.creator));
    m_producerEdit->setText(QString::fromStdString(intel.metadata.producer));

    m_datesLabel->setText(
        QString("Created: <b>%1</b> | Modified: <b>%2</b> | Tagged PDF: <b>%3</b> | Language: <b>%4</b>")
            .arg(QString::fromStdString(intel.metadata.creationDate.empty() ? "—" : intel.metadata.creationDate))
            .arg(QString::fromStdString(intel.metadata.modificationDate.empty() ? "—" : intel.metadata.modificationDate))
            .arg(intel.isTaggedPdf ? "YES" : "NO")
            .arg(QString::fromStdString(intel.language))
    );

    m_xmpTextEdit->setPlainText(QString::fromStdString(intel.rawXmpPacket));

    // Fonts
    m_fontsTable->setRowCount(static_cast<int>(intel.fonts.size()));
    for (size_t i = 0; i < intel.fonts.size(); ++i) {
        const auto& f = intel.fonts[i];
        m_fontsTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(f.baseFontName)));
        m_fontsTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(f.familyName)));
        m_fontsTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(f.isEmbedded ? "✅ Yes" : "❌ No"));
        m_fontsTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(f.isSubset ? "Yes (Subset)" : "No"));
        m_fontsTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::fromStdString(f.fontType)));

        QString pagesStr;
        for (int p : f.pagesUsed) {
            if (!pagesStr.isEmpty()) pagesStr += ", ";
            pagesStr += QString::number(p + 1);
        }
        m_fontsTable->setItem(static_cast<int>(i), 5, new QTableWidgetItem(pagesStr));
    }

    // Images
    m_imagesTable->setRowCount(static_cast<int>(intel.images.size()));
    for (size_t i = 0; i < intel.images.size(); ++i) {
        const auto& img = intel.images[i];
        m_imagesTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::number(img.pageIndex + 1)));
        m_imagesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString("%1 × %2").arg(img.pixelWidth).arg(img.pixelHeight)));
        m_imagesTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString("%1 × %2").arg(static_cast<int>(img.effectiveDpiX)).arg(static_cast<int>(img.effectiveDpiY))));
        m_imagesTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(img.colorSpace)));
        m_imagesTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::number(img.bitsPerPixel)));
        m_imagesTable->setItem(static_cast<int>(i), 5, new QTableWidgetItem(QString::fromStdString(img.filter)));
        m_imagesTable->setItem(static_cast<int>(i), 6, new QTableWidgetItem(img.hasIccProfile ? "✅ Embedded" : "None"));
    }

    // Boxes
    m_boxesTable->setRowCount(static_cast<int>(intel.pageBoxes.size()));
    for (size_t i = 0; i < intel.pageBoxes.size(); ++i) {
        const auto& b = intel.pageBoxes[i];
        m_boxesTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::number(b.pageIndex + 1)));
        m_boxesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString("%1 × %2").arg(b.mediaBox.width, 0, 'f', 1).arg(b.mediaBox.height, 0, 'f', 1)));
        m_boxesTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString("%1 × %2").arg(b.cropBox.width, 0, 'f', 1).arg(b.cropBox.height, 0, 'f', 1)));
        m_boxesTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString("%1 × %2").arg(b.bleedBox.width, 0, 'f', 1).arg(b.bleedBox.height, 0, 'f', 1)));
        m_boxesTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString("%1 × %2").arg(b.trimBox.width, 0, 'f', 1).arg(b.trimBox.height, 0, 'f', 1)));
        m_boxesTable->setItem(static_cast<int>(i), 5, new QTableWidgetItem(QString("%1 × %2").arg(b.artBox.width, 0, 'f', 1).arg(b.artBox.height, 0, 'f', 1)));
        m_boxesTable->setItem(static_cast<int>(i), 6, new QTableWidgetItem(QString("%1°").arg(b.rotation * 90)));
    }

    // Attachments
    m_attachmentsTable->setRowCount(static_cast<int>(intel.attachments.size()));
    for (size_t i = 0; i < intel.attachments.size(); ++i) {
        const auto& a = intel.attachments[i];
        m_attachmentsTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::number(a.index + 1)));
        m_attachmentsTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(a.name)));
        m_attachmentsTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(a.description)));
        m_attachmentsTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(a.mimeType.empty() ? "application/octet-stream" : a.mimeType)));
        m_attachmentsTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString("%1 KB").arg(a.sizeBytes / 1024.0, 0, 'f', 1)));
    }

    if (intel.hasLayers) {
        QString layerList;
        for (const auto& l : intel.layerNames) {
            if (!layerList.isEmpty()) layerList += ", ";
            layerList += QString::fromStdString(l);
        }
        m_layersStatusLabel->setText(QString("<b>✅ Document Layers Active:</b> %1").arg(layerList));
        m_layersStatusLabel->setStyleSheet("color: #166534;");
    } else {
        m_layersStatusLabel->setText(tr("No Optional Content Groups (OCGs / Layers) present in this document."));
        m_layersStatusLabel->setStyleSheet("color: #64748B;");
    }
}

void DocumentIntelligenceDialog::onSaveMetadataClicked() {
    if (!m_doc || !m_doc->IsValid()) return;

    QString defaultName = QString::fromStdString(m_doc->GetPath().stem().string() + "_updated_meta.pdf");
    QString savePath = QFileDialog::getSaveFileName(
        this, tr("Save Document with Updated Metadata"), defaultName, tr("PDF Documents (*.pdf)")
    );
    if (savePath.isEmpty()) return;

    pdf_api::DocumentMetadata meta;
    meta.title = m_titleEdit->text().toStdString();
    meta.author = m_authorEdit->text().toStdString();
    meta.subject = m_subjectEdit->text().toStdString();
    meta.keywords = m_keywordsEdit->text().toStdString();
    meta.creator = m_creatorEdit->text().toStdString();
    meta.producer = m_producerEdit->text().toStdString();

    std::string xmp = m_xmpTextEdit->toPlainText().toStdString();
    auto res = services::DocumentIntelligenceService::Instance().UpdateMetadata(
        *m_doc, savePath.toStdString(), meta, xmp
    );

    if (res.has_value()) {
        QMessageBox::information(
            this, tr("Metadata Saved"),
            tr("Updated metadata successfully saved to:\n%1").arg(savePath)
        );
    } else {
        QMessageBox::critical(this, tr("Save Failed"), QString::fromStdString(res.errorMessage));
    }
}

void DocumentIntelligenceDialog::onExtractAttachmentClicked() {
    int row = m_attachmentsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("No Attachment Selected"), tr("Please select an attachment row to export."));
        return;
    }

    QString defaultName = m_attachmentsTable->item(row, 1)->text();
    QString savePath = QFileDialog::getSaveFileName(this, tr("Export Attachment"), defaultName, tr("All Files (*.*)"));
    if (savePath.isEmpty()) return;

    auto res = services::DocumentIntelligenceService::Instance().ExtractAttachment(
        *m_doc, row, savePath.toStdString()
    );

    if (res.has_value()) {
        QMessageBox::information(this, tr("Attachment Exported"), tr("Attachment saved to:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), QString::fromStdString(res.errorMessage));
    }
}

void DocumentIntelligenceDialog::onAddAttachmentClicked() {
    if (!m_doc || !m_doc->IsValid()) return;

    QString file = QFileDialog::getOpenFileName(this, tr("Select File to Attach"), "", tr("All Files (*.*)"));
    if (file.isEmpty()) return;

    auto res = services::DocumentIntelligenceService::Instance().EmbedAttachment(
        *m_doc, file.toStdString(), "Embedded User Attachment"
    );

    if (res.has_value()) {
        QMessageBox::information(this, tr("Attachment Added"), tr("File attached successfully."));
        populateData();
    } else {
        QMessageBox::critical(this, tr("Attachment Failed"), QString::fromStdString(res.errorMessage));
    }
}

void DocumentIntelligenceDialog::onRefreshAnalysis() {
    populateData();
}

} // namespace pdfinity::ui
