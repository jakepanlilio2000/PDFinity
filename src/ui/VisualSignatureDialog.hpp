// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QPaintEvent>
#include <QMouseEvent>

namespace pdfinity::ui {

class SignatureCanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit SignatureCanvasWidget(QWidget* parent = nullptr);
    void Clear();
    [[nodiscard]] const std::vector<std::vector<pdf_api::PdfPoint>>& GetPaths() const { return m_paths; }
    [[nodiscard]] bool HasStrokes() const { return !m_paths.empty(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    std::vector<std::vector<pdf_api::PdfPoint>> m_paths;
    std::vector<pdf_api::PdfPoint> m_currentPath;
    bool m_isDrawing{false};
};

class VisualSignatureDialog : public QDialog {
    Q_OBJECT

public:
    explicit VisualSignatureDialog(int pageIndex, QWidget* parent = nullptr);
    ~VisualSignatureDialog() override = default;

    [[nodiscard]] pdf_api::VisualSignatureData GetSignatureData() const;

private slots:
    void OnBrowseImage();
    void OnClearDrawing();
    void OnAccept();

private:
    void SetupUi();

    int m_pageIndex{0};
    QTabWidget* m_tabWidget{nullptr};

    // Tab 1: Draw
    SignatureCanvasWidget* m_canvas{nullptr};

    // Tab 2: Type
    QLineEdit* m_nameEdit{nullptr};
    QLineEdit* m_titleEdit{nullptr};
    QLabel* m_typePreview{nullptr};

    // Tab 3: Import Image
    QLineEdit* m_imagePathEdit{nullptr};
    QLabel* m_imagePreview{nullptr};
    std::vector<uint8_t> m_imageBgra;
    int m_imageWidth{0};
    int m_imageHeight{0};

    // Coordinates
    QDoubleSpinBox* m_xSpin{nullptr};
    QDoubleSpinBox* m_ySpin{nullptr};
    QDoubleSpinBox* m_wSpin{nullptr};
    QDoubleSpinBox* m_hSpin{nullptr};

    pdf_api::VisualSignatureData m_resultData;
};

} // namespace pdfinity::ui
