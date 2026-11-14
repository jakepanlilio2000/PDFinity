// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfViewerWidget.hpp"
#include "AnnotationToolbar.hpp"
#include "XfaNoticeBanner.hpp"
#include "FormInspectorDialog.hpp"
#include "ContentEditingToolbar.hpp"
#include "ObjectInspectorDialog.hpp"
#include "shell/LeftNavRail.hpp"
#include "FloatingHudWidget.hpp"
#include "shell/RightInspectorPanel.hpp"
#include "VectorIcons.hpp"
#include "services/AnnotationCommandManager.hpp"
#include "services/PrintService.hpp"
#include "application/Logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <cmath>

namespace pdfinity::ui {

PdfViewerWidget::PdfViewerWidget(
    std::shared_ptr<domain::DocumentModel> doc,
    QWidget* parent
) : QWidget(parent),
    m_doc(std::move(doc)),
    m_renderService(std::make_unique<services::AsyncRenderService>(this)),
    m_searchService(std::make_unique<services::DocumentSearchService>(this)) {

    SetupUi();
}

PdfViewerWidget::~PdfViewerWidget() = default;

void PdfViewerWidget::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    CreateToolBar();
    mainLayout->addWidget(m_toolBar);

    // XFA Form Notice Banner
    m_xfaBanner = new XfaNoticeBanner(this);
    if (m_doc && m_doc->HasXfa()) {
        auto ft = m_doc->GetFormType();
        m_xfaBanner->SetFormTypeInfo(ft.has_value() && ft.value == pdf_api::FormType::XfaFull);
        m_xfaBanner->setVisible(true);
    } else {
        m_xfaBanner->setVisible(false);
    }
    mainLayout->addWidget(m_xfaBanner);

    // Annotation Toolbar & Command Manager
    m_commandManager = std::make_unique<services::AnnotationCommandManager>(nullptr);

    m_annotToolbar = new AnnotationToolbar(this);
    m_annotToolbar->setVisible(false);
    mainLayout->addWidget(m_annotToolbar);

    // Content Editing Toolbar & Service
    m_contentEditService = std::make_shared<services::ContentEditingService>(nullptr);
    m_contentToolbar = new ContentEditingToolbar(m_contentEditService, this);
    m_contentToolbar->setVisible(false);
    m_contentToolbar->SetDocument(m_doc, 0);
    connect(m_contentToolbar, &ContentEditingToolbar::ContentModified, [this]() {
        if (m_renderService) {
            m_renderService->GetCache().Clear();
            m_renderService->RequestPageRender(m_doc, GetCurrentPage(), GetZoomScale(), devicePixelRatioF(), 10);
        }
        m_canvas->update();
    });
    connect(m_contentToolbar, &ContentEditingToolbar::OpenInspectorRequested, this, &PdfViewerWidget::OpenObjectInspector);
    mainLayout->addWidget(m_contentToolbar);

    // Search bar docked right below toolbar
    m_searchBar = new PdfSearchBar(this);
    connect(m_searchBar, &PdfSearchBar::SearchRequested, this, &PdfViewerWidget::OnSearchRequested);
    connect(m_searchBar, &PdfSearchBar::FindNextRequested, this, &PdfViewerWidget::OnFindNext);
    connect(m_searchBar, &PdfSearchBar::FindPrevRequested, this, &PdfViewerWidget::OnFindPrev);
    connect(m_searchBar, &PdfSearchBar::SearchClosed, [this]() {
        m_canvas->SetSearchMatches({}, -1);
        m_currentMatches.clear();
        m_activeMatchIdx = -1;
    });
    mainLayout->addWidget(m_searchBar);

    // Central Splitter (3-Pane Enterprise Workstation)
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(true);

    // Pane 1: Left Navigation Rail & Drawer (44px vertical rail with 7 drawer panels)
    m_leftRail = new LeftNavRail(m_doc, m_renderService.get(), m_splitter);
    connect(m_leftRail, &LeftNavRail::PageSelected, this, &PdfViewerWidget::GoToPage);
    connect(m_leftRail, &LeftNavRail::AnnotationSelected, [this](int pageIndex, int annotIndex) {
        GoToPage(pageIndex);
        m_canvas->SelectAnnotation(pageIndex, annotIndex);
        if (m_annotToolbar->isHidden()) {
            m_annotToolbar->show();
            m_toggleAnnotAction->setChecked(true);
        }
    });
    m_splitter->addWidget(m_leftRail);

    // Pane 2: Central Canvas Viewport & Floating HUD
    m_scrollArea = new QScrollArea(m_splitter);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: #525659; border: none; }");

    m_canvas = new PdfCanvasWidget(m_doc, m_renderService.get(), m_scrollArea);
    m_canvas->SetCommandManager(m_commandManager.get());
    connect(m_canvas, &PdfCanvasWidget::CurrentPageChanged, this, &PdfViewerWidget::OnCanvasPageChanged);
    connect(m_canvas, &PdfCanvasWidget::ZoomChanged, this, &PdfViewerWidget::OnCanvasZoomChanged);
    connect(m_canvas, &PdfCanvasWidget::AnnotationSelected, [this](int /*page*/, int annotIdx) {
        m_annotToolbar->SetDeleteEnabled(annotIdx >= 0);
        if (annotIdx >= 0 && m_inspector) {
            SelectionPropertiesInfo info;
            info.objectType = tr("Markup Annotation #%1").arg(annotIdx + 1);
            m_inspector->SetSelectionProperties(info);
        } else if (m_inspector) {
            m_inspector->ClearSelection();
        }
    });
    connect(m_canvas, &PdfCanvasWidget::AnnotationModified, [this]() {
        if (m_leftRail) m_leftRail->RefreshAnnotations();
    });

    // Wire annotation toolbar to canvas
    connect(m_annotToolbar, &AnnotationToolbar::ToolChanged, [this](AnnotationToolMode mode) {
        m_canvas->SetAnnotationToolMode(static_cast<int>(mode));
    });
    connect(m_annotToolbar, &AnnotationToolbar::ColorChanged, [this](const QColor& c) {
        m_canvas->SetAnnotationColor(c);
    });
    connect(m_annotToolbar, &AnnotationToolbar::StrokeWidthChanged, [this](float w) {
        m_canvas->SetAnnotationStrokeWidth(w);
    });
    connect(m_annotToolbar, &AnnotationToolbar::StampNameChanged, [this](const QString& s) {
        m_canvas->SetStampName(s);
    });
    connect(m_annotToolbar, &AnnotationToolbar::UndoRequested, this, &PdfViewerWidget::Undo);
    connect(m_annotToolbar, &AnnotationToolbar::RedoRequested, this, &PdfViewerWidget::Redo);
    connect(m_annotToolbar, &AnnotationToolbar::DeleteRequested, [this]() {
        m_canvas->DeleteSelectedAnnotation();
    });

    // Wire command manager to toolbar and rail
    connect(m_commandManager.get(), &services::AnnotationCommandManager::CanUndoChanged,
            m_annotToolbar, &AnnotationToolbar::SetUndoEnabled);
    connect(m_commandManager.get(), &services::AnnotationCommandManager::CanRedoChanged,
            m_annotToolbar, &AnnotationToolbar::SetRedoEnabled);
    connect(m_commandManager.get(), &services::AnnotationCommandManager::CommandExecuted, [this]() {
        if (m_leftRail) m_leftRail->RefreshAnnotations();
    });

    m_scrollArea->setWidget(m_canvas);

    // Floating Bottom Page / Zoom HUD
    m_floatingHud = new FloatingHudWidget(m_scrollArea);
    m_floatingHud->UpdatePageInfo(m_canvas->GetCurrentPage(), m_doc ? m_doc->GetPageCount() : 1);
    m_floatingHud->UpdateZoom(m_canvas->GetZoomScale());
    connect(m_floatingHud, &FloatingHudWidget::PreviousPageRequested, this, [this]() {
        GoToPage(GetCurrentPage() - 1);
    });
    connect(m_floatingHud, &FloatingHudWidget::NextPageRequested, this, [this]() {
        GoToPage(GetCurrentPage() + 1);
    });
    connect(m_floatingHud, &FloatingHudWidget::PageSelected, this, &PdfViewerWidget::GoToPage);
    connect(m_floatingHud, &FloatingHudWidget::ZoomInRequested, this, &PdfViewerWidget::ZoomIn);
    connect(m_floatingHud, &FloatingHudWidget::ZoomOutRequested, this, &PdfViewerWidget::ZoomOut);
    connect(m_floatingHud, &FloatingHudWidget::ZoomResetRequested, this, [this]() { SetZoom(1.0); });
    connect(m_floatingHud, &FloatingHudWidget::FitWidthRequested, this, &PdfViewerWidget::ZoomFitWidth);
    connect(m_floatingHud, &FloatingHudWidget::FitPageRequested, this, &PdfViewerWidget::ZoomFitPage);
    connect(m_floatingHud, &FloatingHudWidget::RotateRequested, this, [this]() {
        if (m_doc) {
            int p = GetCurrentPage();
            int rot = (m_doc->GetPageRotation(p) + 90) % 360;
            m_doc->SetPageRotation(p, rot);
            if (m_renderService) {
                m_renderService->GetCache().Clear();
                m_renderService->RequestPageRender(m_doc, p, GetZoomScale(), devicePixelRatioF(), 10);
            }
            m_canvas->update();
        }
    });

    m_splitter->addWidget(m_scrollArea);

    // Pane 3: Right Contextual Inspector & All Tools Panel (260-340px)
    m_inspector = new RightInspectorPanel(m_splitter);
    m_inspector->SetDocument(m_doc);
    connect(m_inspector, &RightInspectorPanel::CloseRequested, this, &PdfViewerWidget::ToggleInspector);
    connect(m_inspector, &RightInspectorPanel::StrokeColorChanged, [this](const QColor& c) {
        m_canvas->SetAnnotationColor(c);
    });
    connect(m_inspector, &RightInspectorPanel::StrokeWidthChanged, [this](double w) {
        m_canvas->SetAnnotationStrokeWidth(static_cast<float>(w));
    });
    connect(m_inspector, &RightInspectorPanel::DeleteSelectionTriggered, [this]() {
        m_canvas->DeleteSelectedAnnotation();
    });
    m_splitter->addWidget(m_inspector);

    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setSizes({44, 850, 280});

    mainLayout->addWidget(m_splitter);

    // Connect search service
    connect(m_searchService.get(), &services::DocumentSearchService::SearchCompleted,
            this, &PdfViewerWidget::OnSearchCompleted);

    // Initialize page indicator
    int total = m_doc ? m_doc->GetPageCount() : 0;
    m_pageSpinBox->setMaximum(std::max(1, total));
    m_pageCountLabel->setText(tr("/ %1").arg(total));
}

void PdfViewerWidget::CreateToolBar() {
    m_toolBar = new QToolBar(tr("Viewer Toolbar"), this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(18, 18));
    m_toolBar->setStyleSheet("QToolBar { border-bottom: 1px solid #e5e7eb; background: #f9fafb; padding: 3px; }");

    // Toggle Sidebar
    m_toggleSidebarAction = m_toolBar->addAction(tr("Side Panel"));
    m_toggleSidebarAction->setToolTip(tr("Toggle Thumbnails & Outline Sidebar"));
    connect(m_toggleSidebarAction, &QAction::triggered, this, &PdfViewerWidget::ToggleSidebar);

    m_toolBar->addSeparator();

    // Previous Page
    auto* prevAction = m_toolBar->addAction(tr("◀"));
    prevAction->setToolTip(tr("Previous Page (Page Up)"));
    connect(prevAction, &QAction::triggered, [this]() {
        GoToPage(GetCurrentPage() - 1);
    });

    // Page number spinbox
    m_pageSpinBox = new QSpinBox(m_toolBar);
    m_pageSpinBox->setMinimum(1);
    m_pageSpinBox->setValue(1);
    m_pageSpinBox->setFixedWidth(60);
    m_pageSpinBox->setAlignment(Qt::AlignCenter);
    connect(m_pageSpinBox, &QSpinBox::valueChanged, this, &PdfViewerWidget::OnPageNumberChanged);
    m_toolBar->addWidget(m_pageSpinBox);

    m_pageCountLabel = new QLabel(tr("/ 1"), m_toolBar);
    m_pageCountLabel->setStyleSheet("padding-left: 4px; padding-right: 8px; color: #4b5563; font-weight: 500;");
    m_toolBar->addWidget(m_pageCountLabel);

    // Next Page
    auto* nextAction = m_toolBar->addAction(tr("▶"));
    nextAction->setToolTip(tr("Next Page (Page Down)"));
    connect(nextAction, &QAction::triggered, [this]() {
        GoToPage(GetCurrentPage() + 1);
    });

    m_toolBar->addSeparator();

    // Zoom Controls
    auto* zoomOutAction = m_toolBar->addAction(tr("－"));
    zoomOutAction->setToolTip(tr("Zoom Out (Ctrl+Minus)"));
    connect(zoomOutAction, &QAction::triggered, this, &PdfViewerWidget::ZoomOut);

    m_zoomLabel = new QLabel(tr("100%"), m_toolBar);
    m_zoomLabel->setFixedWidth(52);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setStyleSheet("color: #374151; font-weight: 600;");
    m_toolBar->addWidget(m_zoomLabel);

    auto* zoomInAction = m_toolBar->addAction(tr("＋"));
    zoomInAction->setToolTip(tr("Zoom In (Ctrl+Plus)"));
    connect(zoomInAction, &QAction::triggered, this, &PdfViewerWidget::ZoomIn);

    auto* fitWidthAction = m_toolBar->addAction(tr("Fit Width"));
    connect(fitWidthAction, &QAction::triggered, this, &PdfViewerWidget::ZoomFitWidth);

    auto* fitPageAction = m_toolBar->addAction(tr("Fit Page"));
    connect(fitPageAction, &QAction::triggered, this, &PdfViewerWidget::ZoomFitPage);

    m_toolBar->addSeparator();

    // Layout Mode Combo
    m_layoutCombo = new QComboBox(m_toolBar);
    m_layoutCombo->addItem(tr("Continuous"), static_cast<int>(pdf_api::PageLayoutMode::Continuous));
    m_layoutCombo->addItem(tr("Single Page"), static_cast<int>(pdf_api::PageLayoutMode::SinglePage));
    m_layoutCombo->addItem(tr("Facing Pages"), static_cast<int>(pdf_api::PageLayoutMode::FacingPages));
    connect(m_layoutCombo, &QComboBox::currentIndexChanged, [this](int index) {
        auto mode = static_cast<pdf_api::PageLayoutMode>(m_layoutCombo->itemData(index).toInt());
        m_canvas->SetLayoutMode(mode);
    });
    m_toolBar->addWidget(m_layoutCombo);

    m_toolBar->addSeparator();

    // Search Action
    auto* findAction = m_toolBar->addAction(tr("🔍 Find"));
    findAction->setToolTip(tr("Find text in document (Ctrl+F)"));
    connect(findAction, &QAction::triggered, this, &PdfViewerWidget::ToggleFind);

    // Print Action
    auto* printAction = m_toolBar->addAction(tr("🖨 Print..."));
    printAction->setToolTip(tr("Print document (Ctrl+P)"));
    connect(printAction, &QAction::triggered, this, &PdfViewerWidget::Print);

    m_toolBar->addSeparator();

    // Annotation Studio Action
    m_toggleAnnotAction = m_toolBar->addAction(tr("✏️ Annotate"));
    m_toggleAnnotAction->setCheckable(true);
    m_toggleAnnotAction->setToolTip(tr("Toggle Annotation & Markup Studio"));
    connect(m_toggleAnnotAction, &QAction::triggered, this, &PdfViewerWidget::ToggleAnnotationStudio);

    m_toolBar->addSeparator();

    // Form Inspector Action
    m_formInspectorAction = m_toolBar->addAction(tr("📋 Forms"));
    m_formInspectorAction->setToolTip(tr("Inspect and Fill AcroForms (Ctrl+Shift+F)"));
    connect(m_formInspectorAction, &QAction::triggered, this, &PdfViewerWidget::OpenFormInspector);

    m_toolBar->addSeparator();

    // Content Editing Studio Action
    m_toggleContentEditAction = m_toolBar->addAction(tr("📝 Edit Content"));
    m_toggleContentEditAction->setCheckable(true);
    m_toggleContentEditAction->setToolTip(tr("Toggle Native Page Content Editing Studio (Ctrl+Shift+E)"));
    connect(m_toggleContentEditAction, &QAction::triggered, this, &PdfViewerWidget::ToggleContentEditor);

    m_openInspectorAction = m_toolBar->addAction(tr("🔍 Object Inspector"));
    m_openInspectorAction->setToolTip(tr("Inspect Page Content Objects & Native Streams"));
    connect(m_openInspectorAction, &QAction::triggered, this, &PdfViewerWidget::OpenObjectInspector);
}

void PdfViewerWidget::ToggleContentEditor() {
    bool vis = !m_contentToolbar->isVisible();
    m_contentToolbar->setVisible(vis);
    m_toggleContentEditAction->setChecked(vis);
    if (vis) {
        m_contentToolbar->SetDocument(m_doc, GetCurrentPage());
        if (m_annotToolbar->isVisible()) {
            ToggleAnnotationStudio();
        }
    }
}

void PdfViewerWidget::OpenObjectInspector() {
    if (!m_doc) return;
    ObjectInspectorDialog dlg(m_doc, GetCurrentPage(), m_contentEditService, this);
    connect(&dlg, &ObjectInspectorDialog::ContentModified, [this]() {
        if (m_renderService) {
            m_renderService->GetCache().Clear();
            m_renderService->RequestPageRender(m_doc, GetCurrentPage(), GetZoomScale(), devicePixelRatioF(), 10);
        }
        m_canvas->update();
    });
    connect(&dlg, &ObjectInspectorDialog::ObjectSelected, [this](int pageIndex, int /*objectIndex*/) {
        GoToPage(pageIndex);
    });
    dlg.exec();
}

void PdfViewerWidget::OpenFormInspector() {
    if (!m_doc) return;
    FormInspectorDialog dlg(m_doc, this);
    connect(&dlg, &FormInspectorDialog::FormModified, [this]() {
        if (m_renderService) {
            m_renderService->GetCache().Clear();
            m_renderService->RequestPageRender(m_doc, GetCurrentPage(), GetZoomScale(), devicePixelRatioF(), 10);
        }
        m_canvas->update();
    });
    connect(&dlg, &FormInspectorDialog::FieldSelected, [this](int pageIndex, int /*annotIndex*/) {
        GoToPage(pageIndex);
    });
    dlg.exec();
}

void PdfViewerWidget::ToggleAnnotationStudio() {
    bool vis = !m_annotToolbar->isVisible();
    m_annotToolbar->setVisible(vis);
    m_toggleAnnotAction->setChecked(vis);
    if (!vis) {
        m_canvas->SetAnnotationToolMode(0); // Reset to Select
    }
}

void PdfViewerWidget::Undo() {
    if (m_commandManager) {
        m_commandManager->Undo();
        if (m_renderService) {
            m_renderService->GetCache().Clear();
            m_renderService->RequestPageRender(m_doc, GetCurrentPage(), GetZoomScale(), devicePixelRatioF(), 10);
        }
        m_sidebar->PopulateAnnotations();
        m_canvas->update();
    }
}

void PdfViewerWidget::Redo() {
    if (m_commandManager) {
        m_commandManager->Redo();
        if (m_renderService) {
            m_renderService->GetCache().Clear();
            m_renderService->RequestPageRender(m_doc, GetCurrentPage(), GetZoomScale(), devicePixelRatioF(), 10);
        }
        m_sidebar->PopulateAnnotations();
        m_canvas->update();
    }
}

void PdfViewerWidget::SetZoom(double zoom) {
    m_canvas->SetZoomScale(zoom);
}

void PdfViewerWidget::SetLayout(pdf_api::PageLayoutMode mode) {
    int idx = m_layoutCombo->findData(static_cast<int>(mode));
    if (idx != -1) {
        m_layoutCombo->setCurrentIndex(idx);
    }
    m_canvas->SetLayoutMode(mode);
}

void PdfViewerWidget::GoToPage(int pageIndex) {
    m_canvas->GoToPage(pageIndex);
}

void PdfViewerWidget::Print() {
    services::PrintService::PrintDocument(this, m_doc);
}

void PdfViewerWidget::PrintPreview() {
    services::PrintService::PrintPreview(this, m_doc);
}

void PdfViewerWidget::ToggleFind() {
    if (m_searchBar->isVisible()) {
        m_searchBar->CloseSearch();
    } else {
        m_searchBar->OpenSearch();
    }
}

void PdfViewerWidget::ToggleSidebar() {
    if (m_leftRail) {
        m_leftRail->SetDrawerExpanded(!m_leftRail->IsDrawerExpanded());
    } else if (m_sidebar) {
        m_sidebar->setVisible(!m_sidebar->isVisible());
    }
}

void PdfViewerWidget::ToggleInspector() {
    if (m_inspector) {
        m_inspector->setVisible(!m_inspector->isVisible());
    }
}

void PdfViewerWidget::ZoomIn() {
    m_canvas->ZoomIn();
}

void PdfViewerWidget::ZoomOut() {
    m_canvas->ZoomOut();
}

void PdfViewerWidget::ZoomFitWidth() {
    m_canvas->ZoomFitWidth(m_scrollArea->viewport()->width());
}

void PdfViewerWidget::ZoomFitPage() {
    m_canvas->ZoomFitPage(m_scrollArea->viewport()->width(), m_scrollArea->viewport()->height());
}

void PdfViewerWidget::OnPageNumberChanged(int pageNum) {
    m_canvas->GoToPage(pageNum - 1);
}

void PdfViewerWidget::OnCanvasPageChanged(int pageIndex) {
    m_pageSpinBox->blockSignals(true);
    m_pageSpinBox->setValue(pageIndex + 1);
    m_pageSpinBox->blockSignals(false);

    m_sidebar->SetActivePage(pageIndex);
    if (m_contentToolbar) {
        m_contentToolbar->SetDocument(m_doc, pageIndex);
    }
    emit PageChanged(pageIndex, m_doc ? m_doc->GetPageCount() : 0);
}

void PdfViewerWidget::OnCanvasZoomChanged(double zoomScale) {
    int pct = static_cast<int>(std::round(zoomScale * 100.0));
    m_zoomLabel->setText(QString("%1%").arg(pct));
}

void PdfViewerWidget::OnSearchRequested(const QString& query, bool matchCase, bool matchWholeWord) {
    m_searchService->StartSearch(m_doc, query, matchCase, matchWholeWord);
}

void PdfViewerWidget::OnSearchCompleted(const std::vector<pdf_api::TextSearchResult>& allMatches) {
    m_currentMatches = allMatches;
    m_activeMatchIdx = m_currentMatches.empty() ? -1 : 0;

    m_canvas->SetSearchMatches(m_currentMatches, m_activeMatchIdx);
    m_searchBar->SetMatchInfo(m_activeMatchIdx, static_cast<int>(m_currentMatches.size()));
}

void PdfViewerWidget::OnFindNext() {
    if (m_currentMatches.empty()) return;
    m_activeMatchIdx = (m_activeMatchIdx + 1) % static_cast<int>(m_currentMatches.size());
    m_canvas->SetActiveMatchIndex(m_activeMatchIdx);
    m_searchBar->SetMatchInfo(m_activeMatchIdx, static_cast<int>(m_currentMatches.size()));
}

void PdfViewerWidget::OnFindPrev() {
    if (m_currentMatches.empty()) return;
    m_activeMatchIdx = (m_activeMatchIdx - 1 + static_cast<int>(m_currentMatches.size())) % static_cast<int>(m_currentMatches.size());
    m_canvas->SetActiveMatchIndex(m_activeMatchIdx);
    m_searchBar->SetMatchInfo(m_activeMatchIdx, static_cast<int>(m_currentMatches.size()));
}

void PdfViewerWidget::keyPressEvent(QKeyEvent* event) {
    if (event->matches(QKeySequence::Find)) {
        ToggleFind();
        event->accept();
    } else if (event->key() == Qt::Key_F3) {
        if (event->modifiers() & Qt::ShiftModifier) {
            OnFindPrev();
        } else {
            OnFindNext();
        }
        event->accept();
    } else if (event->matches(QKeySequence::ZoomIn)) {
        ZoomIn();
        event->accept();
    } else if (event->matches(QKeySequence::ZoomOut)) {
        ZoomOut();
        event->accept();
    } else if (event->matches(QKeySequence::Print)) {
        Print();
        event->accept();
    } else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_I) {
        ToggleInspector();
        event->accept();
    } else if (event->key() == Qt::Key_PageDown) {
        GoToPage(GetCurrentPage() + 1);
        event->accept();
    } else if (event->key() == Qt::Key_PageUp) {
        GoToPage(GetCurrentPage() - 1);
        event->accept();
    } else if (event->key() == Qt::Key_Home) {
        GoToPage(0);
        event->accept();
    } else if (event->key() == Qt::Key_End) {
        GoToPage(m_doc ? m_doc->GetPageCount() - 1 : 0);
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace pdfinity::ui
