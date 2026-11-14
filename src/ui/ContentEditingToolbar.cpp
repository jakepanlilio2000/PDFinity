// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ContentEditingToolbar.hpp"
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <algorithm>

namespace pdfinity::ui {

ContentEditingToolbar::ContentEditingToolbar(
    std::shared_ptr<services::ContentEditingService> service,
    QWidget* parent
) : QToolBar(tr("Content Editing Studio"), parent),
    m_service(std::move(service)) {

    if (!m_service) {
        m_service = std::make_shared<services::ContentEditingService>(nullptr);
    }

    setObjectName("ContentEditingToolbar");
    setMovable(false);
    SetupActions();
}

void ContentEditingToolbar::SetDocument(std::shared_ptr<domain::DocumentModel> doc, int currentPageIndex) {
    m_doc = std::move(doc);
    m_pageIndex = currentPageIndex;
}

void ContentEditingToolbar::SetupActions() {
    // Mode distinction badge action (non-clickable indicator)
    auto* badgeAction = addAction(tr("LAYER: Native Stream"));
    badgeAction->setEnabled(false);
    badgeAction->setToolTip(tr("All edits apply directly to the base PDF page content stream (not an overlay annotation)."));

    addSeparator();

    m_insertTextAction = addAction(tr("T+ Add Text"));
    m_insertTextAction->setToolTip(tr("Insert native text object into PDF content stream"));
    connect(m_insertTextAction, &QAction::triggered, this, &ContentEditingToolbar::OnInsertText);

    m_insertShapeAction = addAction(tr("⬜+ Add Rect"));
    m_insertShapeAction->setToolTip(tr("Insert native vector rectangle into PDF content stream"));
    connect(m_insertShapeAction, &QAction::triggered, this, &ContentEditingToolbar::OnInsertShape);

    m_insertImageAction = addAction(tr("🖼+ Add Image"));
    m_insertImageAction->setToolTip(tr("Insert native raster image XObject into PDF content stream"));
    connect(m_insertImageAction, &QAction::triggered, this, &ContentEditingToolbar::OnInsertImage);

    addSeparator();

    m_undoAction = addAction(tr("Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(m_service->CanUndo());
    connect(m_undoAction, &QAction::triggered, [this]() {
        m_service->Undo();
        emit ContentModified();
    });
    connect(m_service.get(), &services::ContentEditingService::CanUndoChanged, m_undoAction, &QAction::setEnabled);

    m_redoAction = addAction(tr("Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(m_service->CanRedo());
    connect(m_redoAction, &QAction::triggered, [this]() {
        m_service->Redo();
        emit ContentModified();
    });
    connect(m_service.get(), &services::ContentEditingService::CanRedoChanged, m_redoAction, &QAction::setEnabled);

    addSeparator();

    m_gridSnapAction = addAction(tr("⊞ Snap Grid"));
    m_gridSnapAction->setCheckable(true);
    m_gridSnapAction->setChecked(true);
    m_gridSnapAction->setToolTip(tr("Toggle coordinate grid and snapping guidelines"));

    m_inspectorAction = addAction(tr("📋 Object Inspector..."));
    m_inspectorAction->setToolTip(tr("Open object inspector and native stream property editor"));
    connect(m_inspectorAction, &QAction::triggered, this, &ContentEditingToolbar::OpenInspectorRequested);
}

void ContentEditingToolbar::OnInsertText() {
    if (!m_doc || !m_doc->IsValid()) return;

    bool ok = false;
    QString text = QInputDialog::getText(
        this,
        tr("Insert Native Text Object"),
        tr("Enter text to embed into page content stream:"),
        QLineEdit::Normal,
        tr("Sample Native Text"),
        &ok
    );
    if (!ok || text.trimmed().isEmpty()) return;

    // Standard insertion at default coordinates (x=100, y=500)
    auto cmd = std::make_unique<services::InsertTextCommand>(
        m_doc,
        m_pageIndex,
        text.toStdString(),
        100.0,
        500.0,
        "Helvetica",
        14.0,
        pdf_api::PdfColor{0, 0, 0, 255}
    );

    if (m_service->ExecuteCommand(std::move(cmd))) {
        emit ContentModified();
    }
}

void ContentEditingToolbar::OnInsertShape() {
    if (!m_doc || !m_doc->IsValid()) return;

    pdf_api::PdfRect rect{100.0, 400.0, 150.0, 100.0};
    pdf_api::PdfColor stroke{0, 0, 0, 255};
    pdf_api::PdfColor fill{220, 235, 252, 200}; // light blue

    auto cmd = std::make_unique<services::InsertShapeCommand>(
        m_doc,
        m_pageIndex,
        rect,
        stroke,
        fill,
        2.0
    );

    if (m_service->ExecuteCommand(std::move(cmd))) {
        emit ContentModified();
    }
}

void ContentEditingToolbar::OnInsertImage() {
    if (!m_doc || !m_doc->IsValid()) return;

    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Insert Image Object"),
        QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)")
    );
    if (path.isEmpty()) return;

    QImage img(path);
    if (img.isNull()) {
        QMessageBox::critical(this, tr("Failed to Load Image"), tr("Could not decode image file: %1").arg(path));
        return;
    }

    QImage convImg = img.convertToFormat(QImage::Format_ARGB32);
    std::vector<uint8_t> bgraPixels(convImg.constBits(), convImg.constBits() + convImg.sizeInBytes());

    double targetW = std::min(200.0, static_cast<double>(convImg.width()));
    double targetH = targetW * (static_cast<double>(convImg.height()) / convImg.width());

    auto res = m_doc->InsertPageImage(
        m_pageIndex,
        bgraPixels,
        convImg.width(),
        convImg.height(),
        100.0,
        300.0,
        targetW,
        targetH
    );

    if (res.has_value()) {
        emit ContentModified();
    } else {
        QMessageBox::critical(this, tr("Insertion Failed"), tr("Could not insert image into page content stream."));
    }
}

} // namespace pdfinity::ui
