// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>

namespace pdfinity::ui {

class FloatingHudWidget : public QWidget {
    Q_OBJECT

public:
    explicit FloatingHudWidget(QWidget* parent = nullptr);
    ~FloatingHudWidget() override = default;

    void UpdatePageInfo(int currentPage, int totalPages);
    void UpdateZoom(double zoomScale);
    void Reposition();

signals:
    void PreviousPageRequested();
    void NextPageRequested();
    void PageSelected(int pageIndex);
    void ZoomInRequested();
    void ZoomOutRequested();
    void ZoomResetRequested();
    void FitWidthRequested();
    void FitPageRequested();
    void RotateRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void SetupUi();

    QPushButton* m_prevBtn{nullptr};
    QSpinBox* m_pageSpinBox{nullptr};
    QLabel* m_pageTotalLabel{nullptr};
    QPushButton* m_nextBtn{nullptr};

    QPushButton* m_zoomOutBtn{nullptr};
    QLabel* m_zoomPercentLabel{nullptr};
    QPushButton* m_zoomInBtn{nullptr};

    QPushButton* m_fitWidthBtn{nullptr};
    QPushButton* m_fitPageBtn{nullptr};
    QPushButton* m_rotateBtn{nullptr};

    int m_totalPages{1};
};

} // namespace pdfinity::ui
