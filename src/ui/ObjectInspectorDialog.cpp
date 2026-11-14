// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ObjectInspectorDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

namespace pdfinity::ui {

ObjectInspectorDialog::ObjectInspectorDialog(
    std::shared_ptr<domain::DocumentModel> doc,
    int currentPageIndex,
    std::shared_ptr<services::ContentEditingService> editService,
    QWidget* parent
) : QDialog(parent),
    m_doc(std::move(doc)),
    m_pageIndex(currentPageIndex),
    m_editService(std::move(editService)) {

    if (!m_editService) {
        m_editService = std::make_shared<services::ContentEditingService>(this);
    }

    setWindowTitle(tr("Page Content Object Inspector & Native Stream Editor"));
    resize(940, 620);
    SetupUi();
    RefreshObjects();
}

void ObjectInspectorDialog::SetActivePage(int pageIndex) {
    m_pageIndex = pageIndex;
    RefreshObjects();
}

void ObjectInspectorDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Layer Distinction Badge Header
    m_layerBadgeLabel = new QLabel(this);
    m_layerBadgeLabel->setText(
        tr("<b>STORAGE LAYER:</b> <span style='color: #1e40af;'>Native Content Stream (Base PDF Object)</span> "
           "| <i>Edits permanently modify the underlying content stream, not an interactive overlay.</i>")
    );
    m_layerBadgeLabel->setStyleSheet(
        "background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 6px; padding: 8px; font-size: 12px;"
    );
    mainLayout->addWidget(m_layerBadgeLabel);

    // Font Substitution Warning Banner (Hidden by default)
    m_fontWarningBanner = new QLabel(this);
    m_fontWarningBanner->setStyleSheet(
        "background: #fffbeb; border: 1px solid #fde68a; border-radius: 6px; padding: 8px; font-size: 12px; color: #92400e;"
    );
    m_fontWarningBanner->setWordWrap(true);
    m_fontWarningBanner->setVisible(false);
    mainLayout->addWidget(m_fontWarningBanner);

    // Split Content Layout
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // Left: Objects Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        tr("# (Z)"), tr("Type"), tr("Coordinates"), tr("Dimensions"), tr("Preview / Font")
    });
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #d1d5db; border-radius: 6px; gridline-color: #f3f4f6; }"
        "QHeaderView::section { background: #f9fafb; padding: 6px; font-weight: 600; border: none; border-bottom: 1px solid #e5e7eb; }"
    );
    connect(m_table, &QTableWidget::cellClicked, this, &ObjectInspectorDialog::OnTableRowSelected);
    contentLayout->addWidget(m_table, 3);

    // Right: Property Editor Panel
    auto* editorBox = new QGroupBox(tr("Object Properties & Native Manipulation"), this);
    editorBox->setStyleSheet(
        "QGroupBox { font-weight: 600; border: 1px solid #d1d5db; border-radius: 6px; margin-top: 6px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    auto* editorLayout = new QVBoxLayout(editorBox);
    editorLayout->setSpacing(10);

    auto* metaGrid = new QGridLayout();
    metaGrid->setSpacing(8);

    metaGrid->addWidget(new QLabel(tr("<b>Object Type:</b>"), editorBox), 0, 0);
    m_typeLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_typeLabel, 0, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Bounds:</b>"), editorBox), 1, 0);
    m_coordsLabel = new QLabel(tr("-"), editorBox);
    metaGrid->addWidget(m_coordsLabel, 1, 1);

    metaGrid->addWidget(new QLabel(tr("<b>Font / Enc:</b>"), editorBox), 2, 0);
    m_fontInfoLabel = new QLabel(tr("-"), editorBox);
    m_fontInfoLabel->setWordWrap(true);
    metaGrid->addWidget(m_fontInfoLabel, 2, 1);

    editorLayout->addLayout(metaGrid);

    // Text Editor Sub-panel
    auto* textHeader = new QLabel(tr("<b>Native Text Modification:</b>"), editorBox);
    editorLayout->addWidget(textHeader);

    m_textEdit = new QLineEdit(editorBox);
    m_textEdit->setPlaceholderText(tr("Select a text object to edit..."));
    m_textEdit->setEnabled(false);
    editorLayout->addWidget(m_textEdit);

    auto* fontRow = new QHBoxLayout();
    fontRow->addWidget(new QLabel(tr("Font Size:"), editorBox));
    m_fontSizeSpin = new QDoubleSpinBox(editorBox);
    m_fontSizeSpin->setRange(4.0, 144.0);
    m_fontSizeSpin->setValue(12.0);
    m_fontSizeSpin->setEnabled(false);
    fontRow->addWidget(m_fontSizeSpin);
    editorLayout->addLayout(fontRow);

    m_applyTextBtn = new QPushButton(tr("Apply Native Text Edit"), editorBox);
    m_applyTextBtn->setStyleSheet(
        "QPushButton { background: #DC2626; color: white; font-weight: 600; padding: 6px 12px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background: #B91C1C; }"
        "QPushButton:disabled { background: #9ca3af; }"
    );
    m_applyTextBtn->setEnabled(false);
    connect(m_applyTextBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnApplyTextEdit);
    editorLayout->addWidget(m_applyTextBtn);

    editorLayout->addSpacing(8);

    // Translation & Alignment Sub-panel
    auto* transHeader = new QLabel(tr("<b>Translation & Alignment:</b>"), editorBox);
    editorLayout->addWidget(transHeader);

    auto* transRow = new QHBoxLayout();
    transRow->addWidget(new QLabel(tr("dX (pt):"), editorBox));
    m_dxSpin = new QDoubleSpinBox(editorBox);
    m_dxSpin->setRange(-500.0, 500.0);
    m_dxSpin->setValue(0.0);
    transRow->addWidget(m_dxSpin);

    transRow->addWidget(new QLabel(tr("dY (pt):"), editorBox));
    m_dySpin = new QDoubleSpinBox(editorBox);
    m_dySpin->setRange(-500.0, 500.0);
    m_dySpin->setValue(0.0);
    transRow->addWidget(m_dySpin);

    m_translateBtn = new QPushButton(tr("Move"), editorBox);
    connect(m_translateBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnTranslateObject);
    transRow->addWidget(m_translateBtn);
    editorLayout->addLayout(transRow);

    auto* alignRow = new QHBoxLayout();
    alignRow->addWidget(new QLabel(tr("Align:"), editorBox));
    m_alignCombo = new QComboBox(editorBox);
    m_alignCombo->addItems({
        tr("Select Alignment..."),
        tr("Align Left"), tr("Align Center"), tr("Align Right"),
        tr("Align Top"), tr("Align Middle"), tr("Align Bottom")
    });
    connect(m_alignCombo, QOverload<int>::of(&QComboBox::activated), this, &ObjectInspectorDialog::OnAlignObjects);
    alignRow->addWidget(m_alignCombo);
    editorLayout->addLayout(alignRow);

    editorLayout->addSpacing(8);

    // Z-Order Manipulation Sub-panel
    auto* zHeader = new QLabel(tr("<b>Z-Order Manipulation:</b>"), editorBox);
    editorLayout->addWidget(zHeader);

    auto* zRow1 = new QHBoxLayout();
    auto* bringFwdBtn = new QPushButton(tr("Bring Forward"), editorBox);
    connect(bringFwdBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnBringForward);
    zRow1->addWidget(bringFwdBtn);

    auto* sendBwdBtn = new QPushButton(tr("Send Backward"), editorBox);
    connect(sendBwdBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnSendBackward);
    zRow1->addWidget(sendBwdBtn);
    editorLayout->addLayout(zRow1);

    auto* zRow2 = new QHBoxLayout();
    auto* bringFrontBtn = new QPushButton(tr("Bring to Front"), editorBox);
    connect(bringFrontBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnBringToFront);
    zRow2->addWidget(bringFrontBtn);

    auto* sendBackBtn = new QPushButton(tr("Send to Back"), editorBox);
    connect(sendBackBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnSendToBack);
    zRow2->addWidget(sendBackBtn);
    editorLayout->addLayout(zRow2);

    editorLayout->addSpacing(8);

    m_deleteBtn = new QPushButton(tr("Delete Object"), editorBox);
    m_deleteBtn->setStyleSheet("QPushButton { color: #b91c1c; font-weight: 600; }");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ObjectInspectorDialog::OnDeleteObject);
    editorLayout->addWidget(m_deleteBtn);

    editorLayout->addStretch(1);
    contentLayout->addWidget(editorBox, 2);

    mainLayout->addLayout(contentLayout, 1);

    // Bottom Action Row
    auto* bottomRow = new QHBoxLayout();
    auto* pageLabel = new QLabel(tr("Inspecting Page %1").arg(m_pageIndex + 1), this);
    pageLabel->setStyleSheet("font-weight: 600; color: #4b5563;");
    bottomRow->addWidget(pageLabel);

    bottomRow->addStretch(1);

    auto* undoBtn = new QPushButton(tr("Undo"), this);
    undoBtn->setEnabled(m_editService->CanUndo());
    connect(undoBtn, &QPushButton::clicked, [this]() {
        m_editService->Undo();
        RefreshObjects();
        emit ContentModified();
    });
    connect(m_editService.get(), &services::ContentEditingService::CanUndoChanged, undoBtn, &QPushButton::setEnabled);
    bottomRow->addWidget(undoBtn);

    auto* redoBtn = new QPushButton(tr("Redo"), this);
    redoBtn->setEnabled(m_editService->CanRedo());
    connect(redoBtn, &QPushButton::clicked, [this]() {
        m_editService->Redo();
        RefreshObjects();
        emit ContentModified();
    });
    connect(m_editService.get(), &services::ContentEditingService::CanRedoChanged, redoBtn, &QPushButton::setEnabled);
    bottomRow->addWidget(redoBtn);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(closeBtn);

    mainLayout->addLayout(bottomRow);
}

static const char* GetPageObjectTypeString(pdf_api::PageObjectType type) {
    switch (type) {
        case pdf_api::PageObjectType::Text: return "Text";
        case pdf_api::PageObjectType::Image: return "Raster Image";
        case pdf_api::PageObjectType::Path: return "Vector Path";
        case pdf_api::PageObjectType::Shading: return "Shading";
        case pdf_api::PageObjectType::FormXObject: return "Form XObject";
        default: return "Unknown";
    }
}

void ObjectInspectorDialog::RefreshObjects() {
    m_table->setRowCount(0);
    m_objects.clear();
    m_selectedObjIndex = -1;
    m_fontWarningBanner->setVisible(false);

    if (!m_doc || !m_doc->IsValid()) return;

    auto res = m_doc->GetPageObjects(m_pageIndex);
    if (!res.has_value()) return;

    m_objects = res.value;
    m_table->setRowCount(static_cast<int>(m_objects.size()));

    for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
        const auto& obj = m_objects[i];

        auto* zItem = new QTableWidgetItem(QString::number(i));
        zItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, zItem);

        auto* typeItem = new QTableWidgetItem(tr(GetPageObjectTypeString(obj.type)));
        m_table->setItem(i, 1, typeItem);

        auto* coordsItem = new QTableWidgetItem(QString("X: %1, Y: %2")
            .arg(obj.bounds.x, 0, 'f', 1)
            .arg(obj.bounds.y, 0, 'f', 1));
        m_table->setItem(i, 2, coordsItem);

        auto* dimItem = new QTableWidgetItem(QString("%1 x %2 pt")
            .arg(obj.bounds.width, 0, 'f', 1)
            .arg(obj.bounds.height, 0, 'f', 1));
        m_table->setItem(i, 3, dimItem);

        QString detail;
        if (obj.type == pdf_api::PageObjectType::Text) {
            QString fontStatus = obj.isSubsetFont ? " [Subset]" : (obj.isEmbeddedFont ? " [Embedded]" : "");
            detail = QString("'%1' (%2 pt %3%4)")
                .arg(QString::fromStdString(obj.text).left(20))
                .arg(obj.fontSize, 0, 'f', 1)
                .arg(QString::fromStdString(obj.fontName))
                .arg(fontStatus);
        } else if (obj.type == pdf_api::PageObjectType::Image) {
            detail = QString("%1 x %2 px (%3)")
                .arg(obj.pixelWidth)
                .arg(obj.pixelHeight)
                .arg(QString::fromStdString(obj.colorSpace));
        } else if (obj.type == pdf_api::PageObjectType::Path) {
            detail = QString("%1 segments, stroke: %2 pt")
                .arg(obj.segmentCount)
                .arg(obj.strokeWidth, 0, 'f', 1);
        }
        auto* detailItem = new QTableWidgetItem(detail);
        m_table->setItem(i, 4, detailItem);
    }

    if (!m_objects.empty()) {
        m_table->selectRow(0);
        UpdateEditorPanel(0);
    } else {
        UpdateEditorPanel(-1);
    }
}

void ObjectInspectorDialog::OnTableRowSelected(int row) {
    if (row >= 0 && row < static_cast<int>(m_objects.size())) {
        UpdateEditorPanel(row);
        emit ObjectSelected(m_pageIndex, row);
    }
}

void ObjectInspectorDialog::UpdateEditorPanel(int objectIndex) {
    m_selectedObjIndex = objectIndex;

    if (objectIndex < 0 || objectIndex >= static_cast<int>(m_objects.size())) {
        m_typeLabel->setText("-");
        m_coordsLabel->setText("-");
        m_fontInfoLabel->setText("-");
        m_textEdit->clear();
        m_textEdit->setEnabled(false);
        m_fontSizeSpin->setEnabled(false);
        m_applyTextBtn->setEnabled(false);
        m_translateBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        m_fontWarningBanner->setVisible(false);
        return;
    }

    const auto& obj = m_objects[objectIndex];
    m_typeLabel->setText(tr(GetPageObjectTypeString(obj.type)));
    m_coordsLabel->setText(QString("X: %1, Y: %2, W: %3, H: %4")
        .arg(obj.bounds.x, 0, 'f', 1)
        .arg(obj.bounds.y, 0, 'f', 1)
        .arg(obj.bounds.width, 0, 'f', 1)
        .arg(obj.bounds.height, 0, 'f', 1));

    m_translateBtn->setEnabled(obj.canTransform);
    m_deleteBtn->setEnabled(obj.canDelete);

    if (obj.type == pdf_api::PageObjectType::Text) {
        m_textEdit->setEnabled(true);
        m_textEdit->setText(QString::fromStdString(obj.text));
        m_fontSizeSpin->setEnabled(true);
        m_fontSizeSpin->setValue(obj.fontSize);
        m_applyTextBtn->setEnabled(true);

        std::string fontCap = services::ContentEditingService::AnalyzeFontCapabilities(obj);
        m_fontInfoLabel->setText(QString::fromStdString(fontCap));

        if (obj.isSubsetFont) {
            m_fontWarningBanner->setText(
                tr("⚠️ <b>Embedded Subset Font Detected:</b> '%1' does not contain full character sets. "
                   "Entering characters outside the original subset will trigger standard font reconstruction (Helvetica).")
                    .arg(QString::fromStdString(obj.fontName))
            );
            m_fontWarningBanner->setVisible(true);
        } else {
            m_fontWarningBanner->setVisible(false);
        }
    } else {
        m_textEdit->clear();
        m_textEdit->setEnabled(false);
        m_fontSizeSpin->setEnabled(false);
        m_applyTextBtn->setEnabled(false);
        m_fontInfoLabel->setText(tr("N/A"));
        m_fontWarningBanner->setVisible(false);
    }
}

void ObjectInspectorDialog::OnApplyTextEdit() {
    if (m_selectedObjIndex < 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) || !m_doc) {
        return;
    }

    const auto& obj = m_objects[m_selectedObjIndex];
    if (obj.type != pdf_api::PageObjectType::Text) return;

    std::string newText = m_textEdit->text().toStdString();
    double newFontSize = m_fontSizeSpin->value();

    // Check for Layout Collisions and Margin Overflows
    auto warning = services::ContentEditingService::ValidateLayoutSafety(
        obj, newText, newFontSize, m_objects
    );

    if (warning.hasCollision || warning.hasOverflow) {
        auto ans = QMessageBox::warning(
            this,
            tr("Layout Warning: Potential Overlap"),
            QString::fromStdString(warning.warningMessage) +
                tr("\n\nRaw PDF streams do not reflow text automatically. Proceed with native content modification?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (ans != QMessageBox::Yes) {
            return;
        }
    }

    auto cmd = std::make_unique<services::ModifyTextCommand>(
        m_doc,
        m_pageIndex,
        m_selectedObjIndex,
        obj.text,
        obj.fontSize,
        obj.fillColor,
        newText,
        newFontSize,
        obj.fillColor
    );

    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnDeleteObject() {
    if (m_selectedObjIndex < 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) || !m_doc) {
        return;
    }

    auto selectedRows = m_table->selectionModel()->selectedRows();
    std::vector<pdf_api::PageObjectData> toDelete;
    for (const auto& index : selectedRows) {
        int r = index.row();
        if (r >= 0 && r < static_cast<int>(m_objects.size())) {
            toDelete.push_back(m_objects[r]);
        }
    }

    if (toDelete.empty()) {
        toDelete.push_back(m_objects[m_selectedObjIndex]);
    }

    auto cmd = std::make_unique<services::DeleteObjectsCommand>(
        m_doc,
        m_pageIndex,
        std::move(toDelete)
    );

    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnTranslateObject() {
    if (m_selectedObjIndex < 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) || !m_doc) {
        return;
    }

    double dx = m_dxSpin->value();
    double dy = m_dySpin->value();
    if (dx == 0.0 && dy == 0.0) return;

    std::vector<int> indices;
    auto selectedRows = m_table->selectionModel()->selectedRows();
    for (const auto& index : selectedRows) {
        indices.push_back(index.row());
    }
    if (indices.empty()) {
        indices.push_back(m_selectedObjIndex);
    }

    auto cmd = std::make_unique<services::TransformObjectsCommand>(
        m_doc,
        m_pageIndex,
        indices,
        dx,
        dy,
        1.0,
        1.0
    );

    if (m_editService->ExecuteCommand(std::move(cmd))) {
        m_dxSpin->setValue(0.0);
        m_dySpin->setValue(0.0);
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnBringForward() {
    if (m_selectedObjIndex < 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) - 1 || !m_doc) {
        return;
    }
    auto cmd = std::make_unique<services::ReorderObjectCommand>(
        m_doc, m_pageIndex, m_selectedObjIndex, m_selectedObjIndex + 1
    );
    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnSendBackward() {
    if (m_selectedObjIndex <= 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) || !m_doc) {
        return;
    }
    auto cmd = std::make_unique<services::ReorderObjectCommand>(
        m_doc, m_pageIndex, m_selectedObjIndex, m_selectedObjIndex - 1
    );
    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnBringToFront() {
    if (m_selectedObjIndex < 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) - 1 || !m_doc) {
        return;
    }
    auto cmd = std::make_unique<services::ReorderObjectCommand>(
        m_doc, m_pageIndex, m_selectedObjIndex, static_cast<int>(m_objects.size()) - 1
    );
    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnSendToBack() {
    if (m_selectedObjIndex <= 0 || m_selectedObjIndex >= static_cast<int>(m_objects.size()) || !m_doc) {
        return;
    }
    auto cmd = std::make_unique<services::ReorderObjectCommand>(
        m_doc, m_pageIndex, m_selectedObjIndex, 0
    );
    if (m_editService->ExecuteCommand(std::move(cmd))) {
        RefreshObjects();
        emit ContentModified();
    }
}

void ObjectInspectorDialog::OnAlignObjects(int index) {
    if (index <= 0 || !m_doc) return;

    auto selectedRows = m_table->selectionModel()->selectedRows();
    if (selectedRows.size() < 2) {
        QMessageBox::information(this, tr("Alignment Requires Multiple Objects"),
            tr("Please select two or more objects from the list to align them."));
        m_alignCombo->setCurrentIndex(0);
        return;
    }

    std::vector<pdf_api::PageObjectData> selectedObjs;
    std::vector<int> indices;
    for (const auto& item : selectedRows) {
        int r = item.row();
        if (r >= 0 && r < static_cast<int>(m_objects.size())) {
            selectedObjs.push_back(m_objects[r]);
            indices.push_back(r);
        }
    }

    services::AlignmentType alignType;
    switch (index) {
        case 1: alignType = services::AlignmentType::Left; break;
        case 2: alignType = services::AlignmentType::Center; break;
        case 3: alignType = services::AlignmentType::Right; break;
        case 4: alignType = services::AlignmentType::Top; break;
        case 5: alignType = services::AlignmentType::Middle; break;
        case 6: alignType = services::AlignmentType::Bottom; break;
        default: m_alignCombo->setCurrentIndex(0); return;
    }

    auto deltas = services::ContentEditingService::CalculateAlignment(alignType, selectedObjs);
    for (size_t i = 0; i < indices.size(); ++i) {
        if (deltas[i].first != 0.0 || deltas[i].second != 0.0) {
            m_doc->TransformPageObject(m_pageIndex, indices[i], deltas[i].first, deltas[i].second, 1.0, 1.0);
        }
    }

    m_alignCombo->setCurrentIndex(0);
    RefreshObjects();
    emit ContentModified();
}

} // namespace pdfinity::ui
