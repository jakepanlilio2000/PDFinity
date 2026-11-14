// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QScrollArea>
#include <QTableWidget>
#include <QSplitter>
#include <memory>

namespace pdfinity::ui {

class ComparisonWorkspaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit ComparisonWorkspaceDialog(
        std::shared_ptr<domain::DocumentModel> docA,
        std::shared_ptr<pdf_api::IPdfEngine> engine,
        QWidget* parent = nullptr
    );

private slots:
    void onBrowseDocB();
    void onRunComparison();
    void onPageChanged(int pageIndex);
    void onViewModeChanged(int modeIndex);
    void onZoomChanged(int zoomPercent);

private:
    void setupUi();
    void updateViews();
    void renderCurrentPage();

    std::shared_ptr<domain::DocumentModel> m_docA;
    std::shared_ptr<domain::DocumentModel> m_docB;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;

    pdf_api::DocumentComparisonReport m_report;
    int m_currentPage{0};
    int m_zoomPercent{100};

    // Header Controls
    QLabel* m_docALabel{nullptr};
    QLineEdit* m_docBPathEdit{nullptr};
    QPushButton* m_browseBBtn{nullptr};
    QPushButton* m_compareBtn{nullptr};
    QLabel* m_metricsBanner{nullptr};

    // View Navigation
    QComboBox* m_viewModeCombo{nullptr};
    QPushButton* m_prevPageBtn{nullptr};
    QPushButton* m_nextPageBtn{nullptr};
    QSpinBox* m_pageSpin{nullptr};
    QLabel* m_pageTotalLabel{nullptr};
    QSpinBox* m_zoomSpin{nullptr};

    // Workspace Views
    QSplitter* m_sideBySideSplitter{nullptr};
    QLabel* m_pageViewA{nullptr};
    QLabel* m_pageViewB{nullptr};
    QLabel* m_diffOverlayView{nullptr};
    QScrollArea* m_overlayScroll{nullptr};
    QTableWidget* m_textDiffTable{nullptr};
};

} // namespace pdfinity::ui
