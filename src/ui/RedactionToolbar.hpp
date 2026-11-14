// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QToolBar>
#include <QAction>
#include <QLabel>

namespace pdfinity::ui {

class RedactionToolbar : public QToolBar {
    Q_OBJECT

public:
    explicit RedactionToolbar(QWidget* parent = nullptr);

    void UpdateMarkCount(int count);
    bool IsMarkModeActive() const;
    void SetMarkModeActive(bool active);

signals:
    void markModeToggled(bool active);
    void applyRedactionsRequested();
    void sanitizeDocumentRequested();
    void clearMarksRequested();

private:
    QAction* m_markAction{nullptr};
    QAction* m_applyAction{nullptr};
    QAction* m_sanitizeAction{nullptr};
    QAction* m_clearAction{nullptr};
    QLabel* m_markCountLabel{nullptr};
};

} // namespace pdfinity::ui
