// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QToolBar>
#include <QColor>
#include <QActionGroup>

class QComboBox;
class QDoubleSpinBox;
class QPushButton;

namespace pdfinity::ui {

enum class AnnotationToolMode {
    Select,
    Highlight,
    Underline,
    Strikeout,
    StickyNote,
    FreehandInk,
    Rectangle,
    Circle,
    Line,
    Arrow,
    Stamp,
    FreeText
};

class AnnotationToolbar : public QToolBar {
    Q_OBJECT
public:
    explicit AnnotationToolbar(QWidget* parent = nullptr);
    ~AnnotationToolbar() override = default;

    [[nodiscard]] AnnotationToolMode GetActiveTool() const noexcept { return m_activeTool; }
    [[nodiscard]] QColor GetCurrentColor() const noexcept { return m_currentColor; }
    [[nodiscard]] float GetCurrentStrokeWidth() const noexcept { return m_strokeWidth; }
    [[nodiscard]] QString GetCurrentStampName() const;

    void SetUndoEnabled(bool enabled);
    void SetRedoEnabled(bool enabled);
    void SetDeleteEnabled(bool enabled);

signals:
    void ToolChanged(AnnotationToolMode mode);
    void ColorChanged(const QColor& color);
    void StrokeWidthChanged(float width);
    void StampNameChanged(const QString& name);
    void UndoRequested();
    void RedoRequested();
    void DeleteRequested();

private slots:
    void OnToolTriggered(QAction* action);
    void OnPickColor();
    void OnWidthChanged(double val);
    void OnStampSelected(int index);

private:
    void SetupTools();
    void UpdateColorIcon();

    AnnotationToolMode m_activeTool{AnnotationToolMode::Select};
    QColor m_currentColor{255, 235, 59, 180}; // Default highlighter yellow
    float m_strokeWidth{2.0f};

    QActionGroup* m_toolGroup{nullptr};
    QAction* m_selectAction{nullptr};
    QAction* m_highlightAction{nullptr};
    QAction* m_underlineAction{nullptr};
    QAction* m_strikeoutAction{nullptr};
    QAction* m_noteAction{nullptr};
    QAction* m_inkAction{nullptr};
    QAction* m_rectAction{nullptr};
    QAction* m_circleAction{nullptr};
    QAction* m_lineAction{nullptr};
    QAction* m_arrowAction{nullptr};
    QAction* m_stampAction{nullptr};
    QAction* m_textAction{nullptr};

    QAction* m_undoAction{nullptr};
    QAction* m_redoAction{nullptr};
    QAction* m_deleteAction{nullptr};

    QPushButton* m_colorBtn{nullptr};
    QDoubleSpinBox* m_widthSpin{nullptr};
    QComboBox* m_stampCombo{nullptr};
};

} // namespace pdfinity::ui
