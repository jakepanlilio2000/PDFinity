// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <memory>

namespace pdfinity::ui {

class DocumentIntelligenceDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocumentIntelligenceDialog(
        std::shared_ptr<domain::DocumentModel> doc,
        QWidget* parent = nullptr
    );

private slots:
    void onSaveMetadataClicked();
    void onExtractAttachmentClicked();
    void onAddAttachmentClicked();
    void onRefreshAnalysis();

private:
    void setupUi();
    void setupMetadataTab();
    void setupFontsTab();
    void setupImagesTab();
    void setupBoxesTab();
    void setupAttachmentsTab();

    void populateData();

    std::shared_ptr<domain::DocumentModel> m_doc;

    QTabWidget* m_tabWidget{nullptr};

    // Metadata Tab
    QLineEdit* m_titleEdit{nullptr};
    QLineEdit* m_authorEdit{nullptr};
    QLineEdit* m_subjectEdit{nullptr};
    QLineEdit* m_keywordsEdit{nullptr};
    QLineEdit* m_creatorEdit{nullptr};
    QLineEdit* m_producerEdit{nullptr};
    QLabel* m_datesLabel{nullptr};
    QTextEdit* m_xmpTextEdit{nullptr};
    QPushButton* m_saveMetaBtn{nullptr};

    // Fonts Tab
    QTableWidget* m_fontsTable{nullptr};

    // Images Tab
    QTableWidget* m_imagesTable{nullptr};

    // Boxes Tab
    QTableWidget* m_boxesTable{nullptr};

    // Attachments & Layers Tab
    QTableWidget* m_attachmentsTable{nullptr};
    QLabel* m_layersStatusLabel{nullptr};
    QPushButton* m_extractAttachmentBtn{nullptr};
    QPushButton* m_addAttachmentBtn{nullptr};
};

} // namespace pdfinity::ui
