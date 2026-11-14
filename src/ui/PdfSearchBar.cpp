// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfSearchBar.hpp"
#include <QHBoxLayout>
#include <QKeyEvent>

namespace pdfinity::ui {

PdfSearchBar::PdfSearchBar(QWidget* parent) : QWidget(parent) {
    SetupUi();
    hide(); // Hidden by default until Ctrl+F
}

void PdfSearchBar::SetupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(8);

    setStyleSheet(
        "PdfSearchBar {"
        "  background-color: rgba(240, 242, 245, 0.95);"
        "  border: 1px solid #d1d5db;"
        "  border-radius: 6px;"
        "}"
    );

    m_inputField = new QLineEdit(this);
    m_inputField->setPlaceholderText(tr("Find in document..."));
    m_inputField->setMinimumWidth(200);
    m_inputField->setClearButtonEnabled(true);
    connect(m_inputField, &QLineEdit::textChanged, this, &PdfSearchBar::OnSearchTriggered);
    connect(m_inputField, &QLineEdit::returnPressed, this, &PdfSearchBar::FindNextRequested);
    layout->addWidget(m_inputField);

    m_matchCountLabel = new QLabel(this);
    m_matchCountLabel->setStyleSheet("color: #6b7280; font-size: 12px; min-width: 65px;");
    layout->addWidget(m_matchCountLabel);

    m_prevBtn = new QPushButton(tr("▲ Prev"), this);
    m_prevBtn->setToolTip(tr("Find Previous (Shift+F3)"));
    connect(m_prevBtn, &QPushButton::clicked, this, &PdfSearchBar::FindPrevRequested);
    layout->addWidget(m_prevBtn);

    m_nextBtn = new QPushButton(tr("▼ Next"), this);
    m_nextBtn->setToolTip(tr("Find Next (F3 or Enter)"));
    connect(m_nextBtn, &QPushButton::clicked, this, &PdfSearchBar::FindNextRequested);
    layout->addWidget(m_nextBtn);

    m_caseCheck = new QCheckBox(tr("Match Case"), this);
    connect(m_caseCheck, &QCheckBox::toggled, this, &PdfSearchBar::OnSearchTriggered);
    layout->addWidget(m_caseCheck);

    m_wordCheck = new QCheckBox(tr("Whole Word"), this);
    connect(m_wordCheck, &QCheckBox::toggled, this, &PdfSearchBar::OnSearchTriggered);
    layout->addWidget(m_wordCheck);

    m_closeBtn = new QPushButton(tr("✕"), this);
    m_closeBtn->setToolTip(tr("Close Search (Esc)"));
    m_closeBtn->setFixedWidth(28);
    connect(m_closeBtn, &QPushButton::clicked, this, &PdfSearchBar::CloseSearch);
    layout->addWidget(m_closeBtn);
}

void PdfSearchBar::OpenSearch() {
    show();
    m_inputField->setFocus();
    m_inputField->selectAll();
}

void PdfSearchBar::CloseSearch() {
    hide();
    emit SearchClosed();
}

void PdfSearchBar::SetMatchInfo(int currentMatch, int totalMatches) {
    if (m_inputField->text().trimmed().isEmpty()) {
        m_matchCountLabel->clear();
    } else if (totalMatches <= 0) {
        m_matchCountLabel->setText(tr("0 matches"));
    } else {
        m_matchCountLabel->setText(tr("%1 of %2").arg(currentMatch + 1).arg(totalMatches));
    }
}

void PdfSearchBar::OnSearchTriggered() {
    QString q = m_inputField->text().trimmed();
    emit SearchRequested(q, m_caseCheck->isChecked(), m_wordCheck->isChecked());
}

void PdfSearchBar::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        CloseSearch();
        event->accept();
    } else if (event->key() == Qt::Key_F3) {
        if (event->modifiers() & Qt::ShiftModifier) {
            emit FindPrevRequested();
        } else {
            emit FindNextRequested();
        }
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

} // namespace pdfinity::ui
