// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RedactionToolbar.hpp"

namespace pdfinity::ui {

RedactionToolbar::RedactionToolbar(QWidget* parent)
    : QToolBar("Redaction & Sanitization", parent)
{
    setObjectName("RedactionToolbar");
    setMovable(false);

    m_markAction = addAction("⬛ Mark Redaction");
    m_markAction->setCheckable(true);
    m_markAction->setToolTip("Drag on page to mark a region for secure content destruction");

    connect(m_markAction, &QAction::toggled, this, &RedactionToolbar::markModeToggled);

    m_applyAction = addAction("✂️ Apply Redactions...");
    m_applyAction->setToolTip("Inspect and permanently apply registered redaction marks");
    m_applyAction->setEnabled(false);
    connect(m_applyAction, &QAction::triggered, this, &RedactionToolbar::applyRedactionsRequested);

    m_clearAction = addAction("🗑 Clear Marks");
    m_clearAction->setToolTip("Clear all unapplied redaction marks");
    m_clearAction->setEnabled(false);
    connect(m_clearAction, &QAction::triggered, this, &RedactionToolbar::clearMarksRequested);

    addSeparator();

    m_sanitizeAction = addAction("🛡 Sanitize Document...");
    m_sanitizeAction->setToolTip("Strip metadata, attachments, JavaScript, and hidden layers");
    connect(m_sanitizeAction, &QAction::triggered, this, &RedactionToolbar::sanitizeDocumentRequested);

    addSeparator();

    m_markCountLabel = new QLabel("Marks: 0", this);
    m_markCountLabel->setStyleSheet("color: #71717A; font-weight: bold; padding-left: 8px; padding-right: 8px;");
    addWidget(m_markCountLabel);
}

void RedactionToolbar::UpdateMarkCount(int count) {
    if (m_markCountLabel) {
        m_markCountLabel->setText(QString("Marks: %1").arg(count));
    }
    if (m_applyAction) {
        m_applyAction->setEnabled(count > 0);
    }
    if (m_clearAction) {
        m_clearAction->setEnabled(count > 0);
    }
}

bool RedactionToolbar::IsMarkModeActive() const {
    return m_markAction ? m_markAction->isChecked() : false;
}

void RedactionToolbar::SetMarkModeActive(bool active) {
    if (m_markAction && m_markAction->isChecked() != active) {
        m_markAction->setChecked(active);
    }
}

} // namespace pdfinity::ui
