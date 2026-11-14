// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchProfileEditorDialog.hpp"
#include "services/BatchProfileManager.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QFormLayout>

namespace pdfinity::ui {

BatchProfileEditorDialog::BatchProfileEditorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Batch Profile Editor & Preset Manager"));
    resize(860, 620);
    setModal(true);

    setupUi();
    populateProfilesList();
}

void BatchProfileEditorDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel: Profile List
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    leftLayout->addWidget(new QLabel(tr("<b>Available Profiles:</b>"), this));
    m_profilesList = new QListWidget(this);
    connect(m_profilesList, &QListWidget::currentRowChanged, this, &BatchProfileEditorDialog::onProfileSelected);
    leftLayout->addWidget(m_profilesList);

    auto* leftBtnLayout = new QHBoxLayout();
    m_newBtn = new QPushButton(tr("New"), this);
    m_cloneBtn = new QPushButton(tr("Clone"), this);
    m_deleteBtn = new QPushButton(tr("Delete"), this);
    connect(m_newBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onNewProfile);
    connect(m_cloneBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onCloneProfile);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onDeleteProfile);
    leftBtnLayout->addWidget(m_newBtn);
    leftBtnLayout->addWidget(m_cloneBtn);
    leftBtnLayout->addWidget(m_deleteBtn);
    leftLayout->addLayout(leftBtnLayout);

    splitter->addWidget(leftWidget);

    // Right Panel: Profile Details & Operation Chain
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* headerLayout = new QHBoxLayout();
    m_typeBadge = new QLabel(this);
    m_typeBadge->setStyleSheet("padding: 2px 6px; border-radius: 4px; font-weight: bold; font-size: 11px;");
    headerLayout->addWidget(new QLabel(tr("<b>Profile Details:</b>"), this));
    headerLayout->addStretch();
    headerLayout->addWidget(m_typeBadge);
    rightLayout->addLayout(headerLayout);

    auto* formLayout = new QFormLayout();
    m_nameEdit = new QLineEdit(this);
    m_descEdit = new QTextEdit(this);
    m_descEdit->setMaximumHeight(60);
    formLayout->addRow(tr("Name:"), m_nameEdit);
    formLayout->addRow(tr("Description:"), m_descEdit);
    rightLayout->addLayout(formLayout);

    // Operations chain group
    auto* opsGroup = new QGroupBox(tr("Ordered Operations Chain"), this);
    auto* opsLayout = new QVBoxLayout(opsGroup);

    auto* opsContentLayout = new QHBoxLayout();
    m_opsList = new QListWidget(this);
    connect(m_opsList, &QListWidget::currentRowChanged, this, &BatchProfileEditorDialog::onOperationSelected);
    opsContentLayout->addWidget(m_opsList);

    auto* opsBtnLayout = new QVBoxLayout();
    m_addOpBtn = new QPushButton(tr("Add Operation..."), this);
    m_removeOpBtn = new QPushButton(tr("Remove"), this);
    m_moveUpBtn = new QPushButton(tr("Move Up"), this);
    m_moveDownBtn = new QPushButton(tr("Move Down"), this);

    connect(m_addOpBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onAddOperation);
    connect(m_removeOpBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onRemoveOperation);
    connect(m_moveUpBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onMoveOpUp);
    connect(m_moveDownBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onMoveOpDown);

    opsBtnLayout->addWidget(m_addOpBtn);
    opsBtnLayout->addWidget(m_removeOpBtn);
    opsBtnLayout->addWidget(m_moveUpBtn);
    opsBtnLayout->addWidget(m_moveDownBtn);
    opsBtnLayout->addStretch();
    opsContentLayout->addLayout(opsBtnLayout);
    opsLayout->addLayout(opsContentLayout);

    // Operation Parameters
    m_paramStack = new QStackedWidget(this);

    // Page 0: Optimize
    auto* optPage = new QWidget(this);
    auto* optForm = new QFormLayout(optPage);
    m_optPresetCombo = new QComboBox(this);
    m_optPresetCombo->addItems({"Recommended (150 DPI)", "Prepress (300 DPI Lossless)", "Screen (96 DPI Downsample)", "Custom"});
    connect(m_optPresetCombo, &QComboBox::currentIndexChanged, this, &BatchProfileEditorDialog::onOptimizePresetChanged);
    m_optDpiSpin = new QSpinBox(this);
    m_optDpiSpin->setRange(50, 600);
    m_optDpiSpin->setValue(150);
    m_optQualitySpin = new QSpinBox(this);
    m_optQualitySpin->setRange(10, 100);
    m_optQualitySpin->setValue(85);
    m_optStripMetaCheck = new QCheckBox(tr("Strip document comments and auxiliary metadata"), this);
    optForm->addRow(tr("Preset:"), m_optPresetCombo);
    optForm->addRow(tr("Target DPI:"), m_optDpiSpin);
    optForm->addRow(tr("JPEG Quality (%):"), m_optQualitySpin);
    optForm->addRow("", m_optStripMetaCheck);
    m_paramStack->addWidget(optPage);

    // Page 1: OCR
    auto* ocrPage = new QWidget(this);
    auto* ocrForm = new QFormLayout(ocrPage);
    m_ocrLangCombo = new QComboBox(this);
    m_ocrLangCombo->addItems({"eng (English)", "deu (German)", "fra (French)", "spa (Spanish)", "chi_sim (Chinese)"});
    m_ocrDpiSpin = new QSpinBox(this);
    m_ocrDpiSpin->setRange(72, 600);
    m_ocrDpiSpin->setValue(150);
    ocrForm->addRow(tr("Recognition Language:"), m_ocrLangCombo);
    ocrForm->addRow(tr("Raster Resolution (DPI):"), m_ocrDpiSpin);
    m_paramStack->addWidget(ocrPage);

    // Page 2: PDF/A
    auto* pdfaPage = new QWidget(this);
    auto* pdfaForm = new QFormLayout(pdfaPage);
    m_pdfaProfileCombo = new QComboBox(this);
    m_pdfaProfileCombo->addItems({"PDF/A-1b (ISO 19005-1 Level B)", "PDF/A-2b (ISO 19005-2 Level B)"});
    pdfaForm->addRow(tr("Target Conformance Profile:"), m_pdfaProfileCombo);
    m_paramStack->addWidget(pdfaPage);

    // Page 3: Sanitize
    auto* sanPage = new QWidget(this);
    auto* sanForm = new QFormLayout(sanPage);
    m_sanProfileCombo = new QComboBox(this);
    m_sanProfileCombo->addItems({"Balanced (Metadata + Scripts + Layers)", "Maximum (All Auxiliary Data)"});
    sanForm->addRow(tr("Sanitization Policy:"), m_sanProfileCombo);
    m_paramStack->addWidget(sanPage);

    // Page 4: Export Images
    auto* imgPage = new QWidget(this);
    auto* imgForm = new QFormLayout(imgPage);
    m_imgFormatCombo = new QComboBox(this);
    m_imgFormatCombo->addItems({"PNG", "JPEG", "TIFF", "BMP"});
    m_imgDpiSpin = new QSpinBox(this);
    m_imgDpiSpin->setRange(72, 600);
    m_imgDpiSpin->setValue(300);
    imgForm->addRow(tr("Image Format:"), m_imgFormatCombo);
    imgForm->addRow(tr("Export DPI:"), m_imgDpiSpin);
    m_paramStack->addWidget(imgPage);

    // Page 5: Generic
    auto* genPage = new QWidget(this);
    auto* genLayout = new QVBoxLayout(genPage);
    m_genericLabel = new QLabel(tr("This operation requires no additional configuration parameters."), genPage);
    m_genericLabel->setStyleSheet("color: #64748b; font-style: italic;");
    genLayout->addWidget(m_genericLabel);
    m_paramStack->addWidget(genPage);

    opsLayout->addWidget(new QLabel(tr("<b>Operation Configuration:</b>"), this));
    opsLayout->addWidget(m_paramStack);

    rightLayout->addWidget(opsGroup);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    // Bottom Action Buttons
    auto* bottomLayout = new QHBoxLayout();
    auto* importBtn = new QPushButton(tr("Import Profile..."), this);
    auto* exportBtn = new QPushButton(tr("Export Profile..."), this);
    connect(importBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onImportProfile);
    connect(exportBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onExportProfile);
    bottomLayout->addWidget(importBtn);
    bottomLayout->addWidget(exportBtn);

    bottomLayout->addStretch();

    m_saveBtn = new QPushButton(tr("Save Profile Changes"), this);
    m_saveBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 16px; border-radius: 4px;");
    connect(m_saveBtn, &QPushButton::clicked, this, &BatchProfileEditorDialog::onSaveProfile);
    bottomLayout->addWidget(m_saveBtn);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void BatchProfileEditorDialog::populateProfilesList(const std::string& selectId) {
    m_profilesList->clear();
    auto profiles = services::BatchProfileManager::Instance().GetAllProfiles();

    int selectRow = 0;
    for (int i = 0; i < static_cast<int>(profiles.size()); ++i) {
        const auto& p = profiles[i];
        QString title = QString::fromStdString(p.name);
        if (p.isBuiltIn) {
            title += " [Built-in]";
        }
        auto* item = new QListWidgetItem(title, m_profilesList);
        item->setData(Qt::UserRole, QString::fromStdString(p.id));
        if (!selectId.empty() && p.id == selectId) {
            selectRow = i;
        }
    }

    if (m_profilesList->count() > 0) {
        m_profilesList->setCurrentRow(selectRow);
    }
}

void BatchProfileEditorDialog::onProfileSelected(int row) {
    if (row < 0) return;
    auto id = m_profilesList->item(row)->data(Qt::UserRole).toString().toStdString();
    auto p = services::BatchProfileManager::Instance().FindProfile(id);
    if (p.has_value()) {
        loadProfileIntoEditor(p.value());
    }
}

void BatchProfileEditorDialog::loadProfileIntoEditor(const domain::BatchProfile& profile) {
    m_currentEditingProfile = profile;
    m_isBuiltIn = profile.isBuiltIn;

    m_nameEdit->setText(QString::fromStdString(profile.name));
    m_descEdit->setText(QString::fromStdString(profile.description));

    if (profile.isBuiltIn) {
        m_typeBadge->setText(tr("BUILT-IN (READ-ONLY)"));
        m_typeBadge->setStyleSheet("background-color: #E0E7FF; color: #3730A3; padding: 2px 8px; border-radius: 4px; font-weight: bold;");
        m_nameEdit->setReadOnly(true);
        m_descEdit->setReadOnly(true);
        m_deleteBtn->setEnabled(false);
        m_addOpBtn->setEnabled(false);
        m_removeOpBtn->setEnabled(false);
        m_moveUpBtn->setEnabled(false);
        m_moveDownBtn->setEnabled(false);
        m_saveBtn->setEnabled(false);
    } else {
        m_typeBadge->setText(tr("CUSTOM"));
        m_typeBadge->setStyleSheet("background-color: #DCFCE7; color: #166534; padding: 2px 8px; border-radius: 4px; font-weight: bold;");
        m_nameEdit->setReadOnly(false);
        m_descEdit->setReadOnly(false);
        m_deleteBtn->setEnabled(true);
        m_addOpBtn->setEnabled(true);
        m_removeOpBtn->setEnabled(true);
        m_moveUpBtn->setEnabled(true);
        m_moveDownBtn->setEnabled(true);
        m_saveBtn->setEnabled(true);
    }

    m_opsList->clear();
    for (size_t i = 0; i < profile.operations.size(); ++i) {
        const auto& op = profile.operations[i];
        m_opsList->addItem(QString("%1. %2").arg(i + 1).arg(QString::fromStdString(op.name)));
    }

    if (m_opsList->count() > 0) {
        m_opsList->setCurrentRow(0);
    } else {
        m_paramStack->setCurrentIndex(5);
    }
}

void BatchProfileEditorDialog::onOperationSelected(int row) {
    if (row < 0 || row >= static_cast<int>(m_currentEditingProfile.operations.size())) {
        m_paramStack->setCurrentIndex(5);
        return;
    }
    const auto& op = m_currentEditingProfile.operations[row];
    updateOperationEditorFor(op);
}

void BatchProfileEditorDialog::updateOperationEditorFor(const domain::BatchOperation& op) {
    switch (op.type) {
        case domain::BatchOperationType::Optimize: {
            m_paramStack->setCurrentIndex(0);
            auto itPre = op.parameters.find("preset");
            if (itPre != op.parameters.end()) {
                if (itPre->second == "Prepress") m_optPresetCombo->setCurrentIndex(1);
                else if (itPre->second == "Screen") m_optPresetCombo->setCurrentIndex(2);
                else m_optPresetCombo->setCurrentIndex(0);
            }
            auto itDpi = op.parameters.find("targetDpi");
            if (itDpi != op.parameters.end()) {
                try { m_optDpiSpin->setValue(std::stoi(itDpi->second)); } catch (...) {}
            }
            auto itQ = op.parameters.find("jpegQuality");
            if (itQ != op.parameters.end()) {
                try { m_optQualitySpin->setValue(std::stoi(itQ->second)); } catch (...) {}
            }
            break;
        }
        case domain::BatchOperationType::Ocr: {
            m_paramStack->setCurrentIndex(1);
            auto itLang = op.parameters.find("language");
            if (itLang != op.parameters.end()) {
                int idx = m_ocrLangCombo->findText(QString::fromStdString(itLang->second), Qt::MatchStartsWith);
                if (idx >= 0) m_ocrLangCombo->setCurrentIndex(idx);
            }
            break;
        }
        case domain::BatchOperationType::ConvertToPdfA: {
            m_paramStack->setCurrentIndex(2);
            auto itProf = op.parameters.find("profile");
            if (itProf != op.parameters.end() && itProf->second == "PdfA2b") {
                m_pdfaProfileCombo->setCurrentIndex(1);
            } else {
                m_pdfaProfileCombo->setCurrentIndex(0);
            }
            break;
        }
        case domain::BatchOperationType::Sanitize: {
            m_paramStack->setCurrentIndex(3);
            break;
        }
        case domain::BatchOperationType::ExportImages: {
            m_paramStack->setCurrentIndex(4);
            auto itFmt = op.parameters.find("format");
            if (itFmt != op.parameters.end()) {
                int idx = m_imgFormatCombo->findText(QString::fromStdString(itFmt->second));
                if (idx >= 0) m_imgFormatCombo->setCurrentIndex(idx);
            }
            break;
        }
        default:
            m_paramStack->setCurrentIndex(5);
            break;
    }
}

void BatchProfileEditorDialog::syncCurrentOperationParams() {
    int row = m_opsList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentEditingProfile.operations.size())) return;
    auto& op = m_currentEditingProfile.operations[row];

    switch (op.type) {
        case domain::BatchOperationType::Optimize: {
            int preIdx = m_optPresetCombo->currentIndex();
            op.parameters["preset"] = (preIdx == 1) ? "Prepress" : (preIdx == 2) ? "Screen" : "Recommended";
            op.parameters["targetDpi"] = std::to_string(m_optDpiSpin->value());
            op.parameters["jpegQuality"] = std::to_string(m_optQualitySpin->value());
            op.parameters["stripMetadata"] = m_optStripMetaCheck->isChecked() ? "true" : "false";
            break;
        }
        case domain::BatchOperationType::Ocr: {
            QString lang = m_ocrLangCombo->currentText().split(" ").first();
            op.parameters["language"] = lang.toStdString();
            op.parameters["dpi"] = std::to_string(m_ocrDpiSpin->value());
            break;
        }
        case domain::BatchOperationType::ConvertToPdfA: {
            op.parameters["profile"] = (m_pdfaProfileCombo->currentIndex() == 1) ? "PdfA2b" : "PdfA1b";
            break;
        }
        case domain::BatchOperationType::Sanitize: {
            op.parameters["profile"] = (m_sanProfileCombo->currentIndex() == 1) ? "Maximum" : "Balanced";
            break;
        }
        case domain::BatchOperationType::ExportImages: {
            op.parameters["format"] = m_imgFormatCombo->currentText().toStdString();
            op.parameters["dpi"] = std::to_string(m_imgDpiSpin->value());
            break;
        }
        default:
            break;
    }
}

void BatchProfileEditorDialog::onOptimizePresetChanged(int index) {
    if (index == 0) { // Recommended
        m_optDpiSpin->setValue(150);
        m_optQualitySpin->setValue(85);
        m_optStripMetaCheck->setChecked(false);
    } else if (index == 1) { // Prepress
        m_optDpiSpin->setValue(300);
        m_optQualitySpin->setValue(95);
        m_optStripMetaCheck->setChecked(false);
    } else if (index == 2) { // Screen
        m_optDpiSpin->setValue(96);
        m_optQualitySpin->setValue(75);
        m_optStripMetaCheck->setChecked(true);
    }
}

void BatchProfileEditorDialog::onPdfAProfileChanged(int /*index*/) {}

void BatchProfileEditorDialog::onNewProfile() {
    domain::BatchProfile p;
    p.id = "custom_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    p.name = "New Custom Profile";
    p.description = "Custom batch workflow profile";
    p.isBuiltIn = false;

    domain::BatchOperation defaultOp;
    defaultOp.type = domain::BatchOperationType::Optimize;
    defaultOp.name = "Optimize PDF Streams";
    defaultOp.parameters = {{"preset", "Recommended"}, {"targetDpi", "150"}};
    p.operations.push_back(defaultOp);

    services::BatchProfileManager::Instance().SaveCustomProfile(p);
    populateProfilesList(p.id);
}

void BatchProfileEditorDialog::onCloneProfile() {
    int row = m_profilesList->currentRow();
    if (row < 0) return;

    auto id = m_profilesList->item(row)->data(Qt::UserRole).toString().toStdString();
    auto p = services::BatchProfileManager::Instance().FindProfile(id);
    if (!p.has_value()) return;

    domain::BatchProfile copy = p.value();
    copy.id = "custom_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    copy.name = "Copy of " + copy.name;
    copy.isBuiltIn = false;

    services::BatchProfileManager::Instance().SaveCustomProfile(copy);
    populateProfilesList(copy.id);
}

void BatchProfileEditorDialog::onDeleteProfile() {
    if (m_isBuiltIn) return;

    int ans = QMessageBox::question(
        this, tr("Delete Profile"),
        tr("Are you sure you want to delete profile '%1'?").arg(QString::fromStdString(m_currentEditingProfile.name)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (ans != QMessageBox::Yes) return;

    services::BatchProfileManager::Instance().DeleteCustomProfile(m_currentEditingProfile.id);
    populateProfilesList();
}

void BatchProfileEditorDialog::onAddOperation() {
    if (m_isBuiltIn) return;

    QMenu menu(this);
    auto* actOpt = menu.addAction(tr("Stream Optimization & Downsample"));
    auto* actOcr = menu.addAction(tr("Searchable OCR Text Layer"));
    auto* actPdfA = menu.addAction(tr("Convert to ISO PDF/A"));
    auto* actSan = menu.addAction(tr("Sanitize Metadata & Hidden Content"));
    auto* actImg = menu.addAction(tr("Export Pages to Images"));
    auto* actTxt = menu.addAction(tr("Export Document Text"));

    auto* chosen = menu.exec(QCursor::pos());
    if (!chosen) return;

    domain::BatchOperation op;
    if (chosen == actOpt) {
        op.type = domain::BatchOperationType::Optimize;
        op.name = "Optimize PDF Streams";
        op.parameters = {{"preset", "Recommended"}, {"targetDpi", "150"}};
    } else if (chosen == actOcr) {
        op.type = domain::BatchOperationType::Ocr;
        op.name = "OCR Text Layer";
        op.parameters = {{"language", "eng"}, {"dpi", "150"}};
    } else if (chosen == actPdfA) {
        op.type = domain::BatchOperationType::ConvertToPdfA;
        op.name = "Convert to PDF/A-1b";
        op.parameters = {{"profile", "PdfA1b"}, {"injectOutputIntent", "true"}};
    } else if (chosen == actSan) {
        op.type = domain::BatchOperationType::Sanitize;
        op.name = "Sanitize Metadata";
        op.parameters = {{"profile", "Balanced"}};
    } else if (chosen == actImg) {
        op.type = domain::BatchOperationType::ExportImages;
        op.name = "Export Pages as PNG";
        op.parameters = {{"format", "PNG"}, {"dpi", "300"}};
    } else if (chosen == actTxt) {
        op.type = domain::BatchOperationType::ExportText;
        op.name = "Export Text to UTF-8";
    }

    m_currentEditingProfile.operations.push_back(op);
    int newIdx = static_cast<int>(m_currentEditingProfile.operations.size());
    m_opsList->addItem(QString("%1. %2").arg(newIdx).arg(QString::fromStdString(op.name)));
    m_opsList->setCurrentRow(newIdx - 1);
}

void BatchProfileEditorDialog::onRemoveOperation() {
    if (m_isBuiltIn) return;
    int row = m_opsList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentEditingProfile.operations.size())) return;

    m_currentEditingProfile.operations.erase(m_currentEditingProfile.operations.begin() + row);

    m_opsList->clear();
    for (size_t i = 0; i < m_currentEditingProfile.operations.size(); ++i) {
        const auto& op = m_currentEditingProfile.operations[i];
        m_opsList->addItem(QString("%1. %2").arg(i + 1).arg(QString::fromStdString(op.name)));
    }

    if (row < m_opsList->count()) {
        m_opsList->setCurrentRow(row);
    } else if (m_opsList->count() > 0) {
        m_opsList->setCurrentRow(m_opsList->count() - 1);
    }
}

void BatchProfileEditorDialog::onMoveOpUp() {
    if (m_isBuiltIn) return;
    int row = m_opsList->currentRow();
    if (row <= 0) return;

    std::swap(m_currentEditingProfile.operations[row], m_currentEditingProfile.operations[row - 1]);

    m_opsList->clear();
    for (size_t i = 0; i < m_currentEditingProfile.operations.size(); ++i) {
        const auto& op = m_currentEditingProfile.operations[i];
        m_opsList->addItem(QString("%1. %2").arg(i + 1).arg(QString::fromStdString(op.name)));
    }
    m_opsList->setCurrentRow(row - 1);
}

void BatchProfileEditorDialog::onMoveOpDown() {
    if (m_isBuiltIn) return;
    int row = m_opsList->currentRow();
    if (row < 0 || row >= static_cast<int>(m_currentEditingProfile.operations.size()) - 1) return;

    std::swap(m_currentEditingProfile.operations[row], m_currentEditingProfile.operations[row + 1]);

    m_opsList->clear();
    for (size_t i = 0; i < m_currentEditingProfile.operations.size(); ++i) {
        const auto& op = m_currentEditingProfile.operations[i];
        m_opsList->addItem(QString("%1. %2").arg(i + 1).arg(QString::fromStdString(op.name)));
    }
    m_opsList->setCurrentRow(row + 1);
}

void BatchProfileEditorDialog::onSaveProfile() {
    if (m_isBuiltIn) return;

    syncCurrentOperationParams();
    m_currentEditingProfile.name = m_nameEdit->text().trimmed().toStdString();
    m_currentEditingProfile.description = m_descEdit->toPlainText().trimmed().toStdString();

    if (m_currentEditingProfile.name.empty()) {
        QMessageBox::warning(this, tr("Name Required"), tr("Please provide a name for this profile."));
        return;
    }
    if (m_currentEditingProfile.operations.empty()) {
        QMessageBox::warning(this, tr("Operations Required"), tr("A profile must have at least one operation in its chain."));
        return;
    }

    services::BatchProfileManager::Instance().SaveCustomProfile(m_currentEditingProfile);
    populateProfilesList(m_currentEditingProfile.id);
    QMessageBox::information(this, tr("Profile Saved"), tr("Profile '%1' saved successfully.").arg(QString::fromStdString(m_currentEditingProfile.name)));
}

void BatchProfileEditorDialog::onExportProfile() {
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export Profile to JSON"),
        QString::fromStdString(m_currentEditingProfile.name + ".json"),
        tr("JSON Files (*.json)")
    );
    if (path.isEmpty()) return;

    syncCurrentOperationParams();
    if (services::BatchProfileManager::Instance().ExportProfileToJson(m_currentEditingProfile, path.toStdString())) {
        QMessageBox::information(this, tr("Export Successful"), tr("Profile exported to:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to write profile JSON file."));
    }
}

void BatchProfileEditorDialog::onImportProfile() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Import Profile from JSON"), "", tr("JSON Files (*.json)")
    );
    if (path.isEmpty()) return;

    auto p = services::BatchProfileManager::Instance().ImportProfileFromJson(path.toStdString());
    if (p.has_value()) {
        populateProfilesList(p.value().id);
        QMessageBox::information(this, tr("Import Successful"), tr("Imported profile '%1'.").arg(QString::fromStdString(p.value().name)));
    } else {
        QMessageBox::critical(this, tr("Import Failed"), tr("Failed to parse valid profile from JSON file."));
    }
}

} // namespace pdfinity::ui
