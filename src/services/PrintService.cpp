// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PrintService.hpp"
#include "application/Logger.hpp"

#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QProgressDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <algorithm>

namespace pdfinity::services {

PrintService::PrintService(QObject* parent) : QObject(parent) {}

bool PrintService::PrintDocument(QWidget* parent, const std::shared_ptr<domain::DocumentModel>& doc) {
    if (!doc || !doc->IsValid()) {
        QMessageBox::warning(parent, tr("Print Error"), tr("No valid document loaded for printing."));
        return false;
    }

    if (!doc->GetPermissions().canPrint) {
        QMessageBox::warning(
            parent,
            tr("Printing Prohibited"),
            tr("Document permissions prevent printing this file.")
        );
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QString::fromStdString(doc->GetPath().filename().string()));

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(tr("Print - PDFinity"));
    int pageCount = doc->GetPageCount();
    dialog.setMinMax(1, pageCount);
    dialog.setFromTo(1, pageCount);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    int fromPage = printer.fromPage() > 0 ? printer.fromPage() - 1 : 0;
    int toPage = printer.toPage() > 0 ? std::min(printer.toPage() - 1, pageCount - 1) : pageCount - 1;
    if (fromPage > toPage) std::swap(fromPage, toPage);
    int pagesToPrint = toPage - fromPage + 1;

    QProgressDialog progress(tr("Printing document..."), tr("Cancel"), 0, pagesToPrint, parent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(parent, tr("Print Error"), tr("Failed to initialize printer canvas."));
        return false;
    }

    bool firstPage = true;
    for (int p = fromPage; p <= toPage; ++p) {
        progress.setValue(p - fromPage);
        QCoreApplication::processEvents();

        if (progress.wasCanceled()) {
            painter.end();
            return false;
        }

        if (!firstPage) {
            printer.newPage();
        }
        firstPage = false;

        // Render page at printer DPI
        QRect pageRect = printer.pageLayout().paintRectPixels(printer.resolution());
        pdf_api::RenderOptions opts;
        opts.scale = 2.0; // High resolution rendering for printer
        opts.dpi = printer.resolution();
        opts.renderAnnotations = true;
        opts.renderFormFields = true;

        auto res = doc->RenderPage(p, opts);
        if (res.has_value() && !res.value.pixels.empty()) {
            const auto& raw = res.value;
            QImage img(raw.pixels.data(), raw.width, raw.height, raw.stride, QImage::Format_ARGB32_Premultiplied);
            painter.drawImage(pageRect, img);
        }
    }

    painter.end();
    progress.setValue(pagesToPrint);
    LOG_INFO("Print", "Document successfully printed: " + doc->GetPath().filename().string());
    return true;
}

bool PrintService::PrintPreview(QWidget* parent, const std::shared_ptr<domain::DocumentModel>& doc) {
    if (!doc || !doc->IsValid()) {
        QMessageBox::warning(parent, tr("Print Preview Error"), tr("No valid document loaded for preview."));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QString::fromStdString(doc->GetPath().filename().string()));

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(tr("Print Preview - PDFinity"));

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested, [&doc](QPrinter* pPrinter) {
        QPainter painter;
        if (!painter.begin(pPrinter)) return;

        int pageCount = doc->GetPageCount();
        bool first = true;
        for (int p = 0; p < pageCount; ++p) {
            if (!first) pPrinter->newPage();
            first = false;

            QRect pageRect = pPrinter->pageLayout().paintRectPixels(pPrinter->resolution());
            pdf_api::RenderOptions opts;
            opts.scale = 1.5;
            opts.dpi = pPrinter->resolution();
            opts.renderAnnotations = true;

            auto res = doc->RenderPage(p, opts);
            if (res.has_value() && !res.value.pixels.empty()) {
                const auto& raw = res.value;
                QImage img(raw.pixels.data(), raw.width, raw.height, raw.stride, QImage::Format_ARGB32_Premultiplied);
                painter.drawImage(pageRect, img);
            }
        }
        painter.end();
    });

    preview.exec();
    return true;
}

} // namespace pdfinity::services
