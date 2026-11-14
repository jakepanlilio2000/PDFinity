// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ComparisonWorkspaceDialog.hpp"
#include "services/ComparisonService.hpp"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollBar>

namespace pdfinity::ui {

ComparisonWorkspaceDialog::ComparisonWorkspaceDialog(
    std::shared_ptr<domain::DocumentModel> docA,
    std::shared_ptr<pdf_api::IPdfEngine> engine,
    QWidget* parent
)
    : QDialog(parent),
      m_docA(std::move(docA)),
      m_engine(std::move(engine))
{
    setWindowTitle(tr("Document Comparison Workspace"));
    resize(1020, 720);
    setModal(true);

    setupUi();
}

void ComparisonWorkspaceDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // 1. Document Selection Bar
    auto* docLayout = new QHBoxLayout();
    QString nameA = (m_docA && !m_docA->GetPath().empty()) ? QString::fromStdString(m_docA->GetPath().filename().string()) : tr("[No Document A]");
    m_docALabel = new QLabel(QString("<b>Document A:</b> %1").arg(nameA), this);
    m_docALabel->setStyleSheet("background-color: #EFF6FF; padding: 6px 10px; border-radius: 4px; border: 1px solid #BFDBFE;");
    docLayout->addWidget(m_docALabel, 1);

    docLayout->addWidget(new QLabel(tr("<b>Document B:</b>"), this));
    m_docBPathEdit = new QLineEdit(this);
    m_docBPathEdit->setPlaceholderText(tr("Select second PDF document to compare..."));
    docLayout->addWidget(m_docBPathEdit, 2);

    m_browseBBtn = new QPushButton(tr("Browse..."), this);
    connect(m_browseBBtn, &QPushButton::clicked, this, &ComparisonWorkspaceDialog::onBrowseDocB);
    docLayout->addWidget(m_browseBBtn);

    m_compareBtn = new QPushButton(tr("Compare Documents"), this);
    m_compareBtn->setStyleSheet("background-color: #2563EB; color: white; font-weight: bold; padding: 6px 14px; border-radius: 4px;");
    connect(m_compareBtn, &QPushButton::clicked, this, &ComparisonWorkspaceDialog::onRunComparison);
    docLayout->addWidget(m_compareBtn);

    mainLayout->addLayout(docLayout);

    // 2. Metrics Banner
    m_metricsBanner = new QLabel(tr("Select Document B and click 'Compare Documents' to begin visual and text analysis."), this);
    m_metricsBanner->setStyleSheet("background-color: #F8FAFC; color: #475569; padding: 8px 12px; border: 1px solid #E2E8F0; border-radius: 4px; font-size: 12px;");
    mainLayout->addWidget(m_metricsBanner);

    // 3. Navigation & Mode Toolbar
    auto* toolLayout = new QHBoxLayout();

    toolLayout->addWidget(new QLabel(tr("View Mode:"), this));
    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItem(tr("Side-by-Side Synchronized"), 0);
    m_viewModeCombo->addItem(tr("Visual Difference Overlay"), 1);
    m_viewModeCombo->addItem(tr("Text Difference Analysis"), 2);
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ComparisonWorkspaceDialog::onViewModeChanged);
    toolLayout->addWidget(m_viewModeCombo);

    toolLayout->addSpacing(20);

    m_prevPageBtn = new QPushButton(tr("◀ Previous"), this);
    connect(m_prevPageBtn, &QPushButton::clicked, [this]() {
        if (m_currentPage > 0) onPageChanged(m_currentPage - 1);
    });
    toolLayout->addWidget(m_prevPageBtn);

    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setRange(1, 1);
    m_pageSpin->setValue(1);
    connect(m_pageSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int v) {
        onPageChanged(v - 1);
    });
    toolLayout->addWidget(m_pageSpin);

    m_pageTotalLabel = new QLabel(tr("of 1"), this);
    toolLayout->addWidget(m_pageTotalLabel);

    m_nextPageBtn = new QPushButton(tr("Next ▶"), this);
    connect(m_nextPageBtn, &QPushButton::clicked, [this]() {
        int maxP = m_report.pageResults.empty() ? 1 : static_cast<int>(m_report.pageResults.size());
        if (m_currentPage < maxP - 1) onPageChanged(m_currentPage + 1);
    });
    toolLayout->addWidget(m_nextPageBtn);

    toolLayout->addSpacing(20);

    toolLayout->addWidget(new QLabel(tr("Zoom:"), this));
    m_zoomSpin = new QSpinBox(this);
    m_zoomSpin->setRange(25, 400);
    m_zoomSpin->setSingleStep(25);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ComparisonWorkspaceDialog::onZoomChanged);
    toolLayout->addWidget(m_zoomSpin);

    toolLayout->addStretch();
    mainLayout->addLayout(toolLayout);

    // 4. Workspace Views (Splitter / Scroll Areas)
    m_sideBySideSplitter = new QSplitter(Qt::Horizontal, this);

    auto* scrollA = new QScrollArea(m_sideBySideSplitter);
    scrollA->setAlignment(Qt::AlignCenter);
    m_pageViewA = new QLabel(scrollA);
    m_pageViewA->setAlignment(Qt::AlignCenter);
    scrollA->setWidget(m_pageViewA);
    scrollA->setWidgetResizable(true);
    m_sideBySideSplitter->addWidget(scrollA);

    auto* scrollB = new QScrollArea(m_sideBySideSplitter);
    scrollB->setAlignment(Qt::AlignCenter);
    m_pageViewB = new QLabel(scrollB);
    m_pageViewB->setAlignment(Qt::AlignCenter);
    scrollB->setWidget(m_pageViewB);
    scrollB->setWidgetResizable(true);
    m_sideBySideSplitter->addWidget(scrollB);

    // Synchronize scrolling between Document A and Document B
    connect(scrollA->verticalScrollBar(), &QScrollBar::valueChanged, scrollB->verticalScrollBar(), &QScrollBar::setValue);
    connect(scrollB->verticalScrollBar(), &QScrollBar::valueChanged, scrollA->verticalScrollBar(), &QScrollBar::setValue);
    connect(scrollA->horizontalScrollBar(), &QScrollBar::valueChanged, scrollB->horizontalScrollBar(), &QScrollBar::setValue);
    connect(scrollB->horizontalScrollBar(), &QScrollBar::valueChanged, scrollA->horizontalScrollBar(), &QScrollBar::setValue);

    mainLayout->addWidget(m_sideBySideSplitter, 1);

    // Visual Overlay View
    m_overlayScroll = new QScrollArea(this);
    m_overlayScroll->setAlignment(Qt::AlignCenter);
    m_diffOverlayView = new QLabel(m_overlayScroll);
    m_diffOverlayView->setAlignment(Qt::AlignCenter);
    m_overlayScroll->setWidget(m_diffOverlayView);
    m_overlayScroll->setWidgetResizable(true);
    m_overlayScroll->setVisible(false);
    mainLayout->addWidget(m_overlayScroll, 1);

    // Text Diff View Table
    m_textDiffTable = new QTableWidget(this);
    m_textDiffTable->setColumnCount(4);
    m_textDiffTable->setHorizontalHeaderLabels({
        tr("Status"), tr("Page"), tr("Document A Text"), tr("Document B Text")
    });
    m_textDiffTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_textDiffTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_textDiffTable->setAlternatingRowColors(true);
    m_textDiffTable->setVisible(false);
    mainLayout->addWidget(m_textDiffTable, 1);

    // Bottom Close Row
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
}

void ComparisonWorkspaceDialog::onBrowseDocB() {
    QString file = QFileDialog::getOpenFileName(
        this, tr("Select Document B for Comparison"), "", tr("PDF Documents (*.pdf)")
    );
    if (!file.isEmpty()) {
        m_docBPathEdit->setText(file);
    }
}

void ComparisonWorkspaceDialog::onRunComparison() {
    if (!m_docA || !m_docA->IsValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Document A is not loaded."));
        return;
    }

    QString bPath = m_docBPathEdit->text();
    if (bPath.isEmpty() || !std::filesystem::exists(bPath.toStdString())) {
        QMessageBox::critical(this, tr("Error"), tr("Please select a valid path for Document B."));
        return;
    }

    if (!m_engine) {
        QMessageBox::critical(this, tr("Error"), tr("PDF Engine is unavailable."));
        return;
    }

    auto openRes = m_engine->OpenDocument(bPath.toStdString());
    if (!openRes.has_value()) {
        QMessageBox::critical(this, tr("Open Failed"), tr("Failed to open Document B:\n%1").arg(QString::fromStdString(openRes.errorMessage)));
        return;
    }

    m_docB = std::make_shared<domain::DocumentModel>(openRes.value, m_engine, bPath.toStdString());

    auto compRes = services::ComparisonService::Instance().CompareDocuments(*m_docA, *m_docB);
    if (!compRes.has_value()) {
        QMessageBox::critical(this, tr("Comparison Failed"), QString::fromStdString(compRes.errorMessage));
        return;
    }

    m_report = compRes.value;
    int maxPages = std::max(1, static_cast<int>(m_report.pageResults.size()));
    m_pageSpin->setRange(1, maxPages);
    m_pageTotalLabel->setText(QString("of %1").arg(maxPages));
    m_currentPage = 0;
    m_pageSpin->setValue(1);

    int totalAdded = 0, totalDeleted = 0, totalModified = 0;
    for (const auto& l : m_report.textDiffs) {
        if (l.type == pdf_api::DiffType::Added) totalAdded++;
        else if (l.type == pdf_api::DiffType::Deleted) totalDeleted++;
        else if (l.type == pdf_api::DiffType::Modified) totalModified++;
    }

    QString bannerText;
    if (m_report.isIdentical) {
        bannerText = QString("<b>✅ Documents are Identical:</b> 0.0% visual difference, zero text changes across %1 page(s).").arg(maxPages);
        m_metricsBanner->setStyleSheet("background-color: #DCFCE7; color: #166534; padding: 8px 12px; border: 1px solid #86EFAC; border-radius: 4px; font-size: 12px;");
    } else {
        bannerText = QString("<b>⚠️ Differences Detected:</b> Overall Visual Difference: <b>%1%</b> | Text Changes: <b>+%2</b> added, <b>-%3</b> deleted, <b>~%4</b> modified across %5 page(s).")
            .arg(m_report.overallVisualDifferencePercent, 0, 'f', 2)
            .arg(totalAdded).arg(totalDeleted).arg(totalModified).arg(maxPages);
        m_metricsBanner->setStyleSheet("background-color: #FEF3C7; color: #92400E; padding: 8px 12px; border: 1px solid #F59E0B; border-radius: 4px; font-size: 12px;");
    }
    m_metricsBanner->setText(bannerText);

    // Populate Text Diff Table
    m_textDiffTable->setRowCount(static_cast<int>(m_report.textDiffs.size()));
    for (size_t i = 0; i < m_report.textDiffs.size(); ++i) {
        const auto& d = m_report.textDiffs[i];
        QString status;
        QColor bg = Qt::white;

        if (d.type == pdf_api::DiffType::Unchanged) {
            status = "Unchanged";
            bg = QColor(248, 250, 252);
        } else if (d.type == pdf_api::DiffType::Added) {
            status = "+ Added";
            bg = QColor(220, 252, 231);
        } else if (d.type == pdf_api::DiffType::Deleted) {
            status = "- Deleted";
            bg = QColor(254, 226, 226);
        } else {
            status = "~ Modified";
            bg = QColor(254, 243, 199);
        }

        int pIndex = (d.pageIndexA >= 0) ? d.pageIndexA : d.pageIndexB;
        auto* itemStatus = new QTableWidgetItem(status);
        auto* itemPage = new QTableWidgetItem(QString::number(pIndex + 1));
        auto* itemA = new QTableWidgetItem(QString::fromStdString(d.textA));
        auto* itemB = new QTableWidgetItem(QString::fromStdString(d.textB));

        itemStatus->setBackground(bg);
        itemPage->setBackground(bg);
        itemA->setBackground(bg);
        itemB->setBackground(bg);

        m_textDiffTable->setItem(static_cast<int>(i), 0, itemStatus);
        m_textDiffTable->setItem(static_cast<int>(i), 1, itemPage);
        m_textDiffTable->setItem(static_cast<int>(i), 2, itemA);
        m_textDiffTable->setItem(static_cast<int>(i), 3, itemB);
    }

    renderCurrentPage();
}

void ComparisonWorkspaceDialog::onPageChanged(int pageIndex) {
    m_currentPage = pageIndex;
    renderCurrentPage();
}

void ComparisonWorkspaceDialog::onViewModeChanged(int modeIndex) {
    if (modeIndex == 0) { // Side-by-Side
        m_sideBySideSplitter->setVisible(true);
        m_overlayScroll->setVisible(false);
        m_textDiffTable->setVisible(false);
    } else if (modeIndex == 1) { // Visual Diff Overlay
        m_sideBySideSplitter->setVisible(false);
        m_overlayScroll->setVisible(true);
        m_textDiffTable->setVisible(false);
    } else { // Text Diff
        m_sideBySideSplitter->setVisible(false);
        m_overlayScroll->setVisible(false);
        m_textDiffTable->setVisible(true);
    }
}

void ComparisonWorkspaceDialog::onZoomChanged(int zoomPercent) {
    m_zoomPercent = zoomPercent;
    renderCurrentPage();
}

void ComparisonWorkspaceDialog::renderCurrentPage() {
    if (!m_docA || !m_docB) return;

    double dpiScale = (m_zoomPercent / 100.0) * (150.0 / 72.0);
    pdf_api::RenderOptions opt;
    opt.scale = dpiScale;
    opt.renderAnnotations = true;

    QImage imgA;
    if (m_currentPage < m_docA->GetPageCount()) {
        auto rA = m_docA->RenderPage(m_currentPage, opt);
        if (rA.has_value()) {
            const auto& d = rA.value;
            imgA = QImage(d.pixels.data(), d.width, d.height, d.stride > 0 ? d.stride : d.width * 4, QImage::Format_ARGB32).copy();
            m_pageViewA->setPixmap(QPixmap::fromImage(imgA));
        }
    } else {
        m_pageViewA->setText(tr("[Page beyond Document A bounds]"));
    }

    QImage imgB;
    if (m_currentPage < m_docB->GetPageCount()) {
        auto rB = m_docB->RenderPage(m_currentPage, opt);
        if (rB.has_value()) {
            const auto& d = rB.value;
            imgB = QImage(d.pixels.data(), d.width, d.height, d.stride > 0 ? d.stride : d.width * 4, QImage::Format_ARGB32).copy();
            m_pageViewB->setPixmap(QPixmap::fromImage(imgB));
        }
    } else {
        m_pageViewB->setText(tr("[Page beyond Document B bounds]"));
    }

    // Render visual difference overlay
    QImage diffOverlay = services::ComparisonService::Instance().GenerateVisualDiffImage(imgA, imgB);
    if (!diffOverlay.isNull()) {
        m_diffOverlayView->setPixmap(QPixmap::fromImage(diffOverlay));
    }
}

} // namespace pdfinity::ui
