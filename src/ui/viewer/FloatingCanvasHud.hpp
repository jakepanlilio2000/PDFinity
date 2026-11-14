// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "pdf_api/PdfTypes.hpp"
#include "ui/VectorIcons.hpp"

namespace pdfinity::ui {

enum class CanvasPointerMode {
    Select,
    HandPan
};

class FloatingCanvasHud : public QWidget {
    Q_OBJECT

public:
    explicit FloatingCanvasHud(QWidget* parent = nullptr);
    ~FloatingCanvasHud() override = default;

    void UpdatePageInfo(int currentPage, int totalPages);
    void UpdateZoom(double zoomScale);
    void SetPointerMode(CanvasPointerMode mode);
    void SetLayoutMode(pdf_api::PageLayoutMode mode);
    void Reposition();
    void WakeUp();

signals:
    // Page navigation
    void PreviousPageRequested();
    void NextPageRequested();
    void PageSelected(int pageIndex);

    // Pointer mode
    void PointerModeChanged(CanvasPointerMode mode);

    // Zoom controls
    void ZoomInRequested();
    void ZoomOutRequested();
    void ZoomScaleChanged(double scale);
    void FitWidthRequested();
    void FitPageRequested();

    // Layout and rotation
    void LayoutModeChanged(pdf_api::PageLayoutMode mode);
    void RotateRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void OnIdleTimeout();
    void OnZoomComboActivated(int index);

private:
    void SetupUi();
    void SetupAnimation();
    void SetHudOpacity(qreal opacity);

    // Navigation controls
    QPushButton* m_prevBtn{nullptr};
    QSpinBox* m_pageSpinBox{nullptr};
    QLabel* m_pageTotalLabel{nullptr};
    QPushButton* m_nextBtn{nullptr};

    // Pointer Mode controls
    QPushButton* m_selectModeBtn{nullptr};
    QPushButton* m_panModeBtn{nullptr};

    // Zoom controls
    QPushButton* m_zoomOutBtn{nullptr};
    QComboBox* m_zoomCombo{nullptr};
    QPushButton* m_zoomInBtn{nullptr};

    // Layout controls
    QPushButton* m_singlePageBtn{nullptr};
    QPushButton* m_continuousBtn{nullptr};
    QPushButton* m_facingSpreadBtn{nullptr};
    QPushButton* m_rotateBtn{nullptr};

    // Animation & Idle fading
    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation* m_fadeAnimation{nullptr};
    QTimer* m_idleTimer{nullptr};

    int m_totalPages{1};
    CanvasPointerMode m_currentPointerMode{CanvasPointerMode::Select};
    pdf_api::PageLayoutMode m_currentLayoutMode{pdf_api::PageLayoutMode::Continuous};
    bool m_isHovered{false};
};

} // namespace pdfinity::ui