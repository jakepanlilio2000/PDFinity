// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QTabWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <filesystem>
#include <memory>

namespace pdfinity::ui {

class ConversionCenterDialog : public QDialog {
    Q_OBJECT

public:
    ConversionCenterDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        std::shared_ptr<pdf_api::IPdfEngine> engine,
        QWidget* parent = nullptr
    );

    std::filesystem::path GetResultPath() const;

private slots:
    void onExportImages();
    void onExportText();
    void onExportHtml();
    void onAddImages();
    void onRemoveImage();
    void onMoveImageUp();
    void onMoveImageDown();
    void onCreatePdfFromImages();
    void onBrowseOfficeInput();
    void onConvertOfficeDoc();

private:
    void setupUi();
    void setupExportTab();
    void setupCreateTab();
    void updateOfficeEngineStatus();

    std::shared_ptr<domain::DocumentModel> m_doc;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::filesystem::path m_resultPath;

    QTabWidget* m_tabWidget{nullptr};

    // Export Tab Controls
    QComboBox* m_imgFormatCombo{nullptr};
    QComboBox* m_imgDpiCombo{nullptr};
    QLineEdit* m_imgOutDirEdit{nullptr};
    QLineEdit* m_textOutPathEdit{nullptr};
    QLineEdit* m_htmlOutPathEdit{nullptr};

    // Create Tab Controls (Images to PDF)
    QListWidget* m_imageListWidget{nullptr};
    QComboBox* m_pageFitCombo{nullptr};
    QLineEdit* m_imgToPdfOutEdit{nullptr};

    // Create Tab Controls (Office to PDF)
    QLabel* m_officeStatusBanner{nullptr};
    QLineEdit* m_officeInputEdit{nullptr};
    QLineEdit* m_officeOutputEdit{nullptr};
    QPushButton* m_officeConvertBtn{nullptr};
};

} // namespace pdfinity::ui
