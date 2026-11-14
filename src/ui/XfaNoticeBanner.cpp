// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "XfaNoticeBanner.hpp"
#include <QHBoxLayout>
#include <QMessageBox>

namespace pdfinity::ui {

XfaNoticeBanner::XfaNoticeBanner(QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(12);

    setStyleSheet(
        "XfaNoticeBanner {"
        "  background-color: #fef3c7;"
        "  border-bottom: 1px solid #f59e0b;"
        "}"
    );

    auto* iconLabel = new QLabel(QString::fromUtf8("\u26A0\uFE0F"), this);
    iconLabel->setStyleSheet("font-size: 16px;");
    layout->addWidget(iconLabel);

    m_messageLabel = new QLabel(
        tr("<b>Unsupported Form Format:</b> This PDF contains XML Forms Architecture (XFA) data. "
           "Interactive filling is not supported for XFA forms."),
        this
    );
    m_messageLabel->setStyleSheet("color: #92400e; font-size: 12px;");
    m_messageLabel->setWordWrap(true);
    layout->addWidget(m_messageLabel, 1);

    m_detailsButton = new QPushButton(tr("Technical Details"), this);
    m_detailsButton->setStyleSheet(
        "QPushButton {"
        "  background: #fde68a; border: 1px solid #d97706; border-radius: 4px;"
        "  padding: 4px 10px; color: #78350f; font-size: 11px; font-weight: 500;"
        "}"
        "QPushButton:hover { background: #fcd34d; }"
    );
    connect(m_detailsButton, &QPushButton::clicked, this, &XfaNoticeBanner::OnShowDetails);
    layout->addWidget(m_detailsButton);

    m_closeButton = new QPushButton(QString::fromUtf8("\u2715"), this);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setToolTip(tr("Dismiss Notice"));
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "  border: none; background: transparent; color: #92400e; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #fde68a; border-radius: 12px; }"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::hide);
    layout->addWidget(m_closeButton);
}

void XfaNoticeBanner::SetFormTypeInfo(bool isFullXfa) {
    m_isFullXfa = isFullXfa;
    if (isFullXfa) {
        m_messageLabel->setText(
            tr("<b>Dynamic XFA Form Detected:</b> This document uses dynamic XML Forms Architecture. "
               "XFA is a deprecated legacy specification not supported by ISO 32000-1/2 or Google PDFium. "
               "Fields cannot be filled interactively in PDFinity.")
        );
    } else {
        m_messageLabel->setText(
            tr("<b>Static XFA Form Detected:</b> This document contains foreground XFA data with static fallback. "
               "Interactive scripts and auto-calculations are disabled.")
        );
    }
}

void XfaNoticeBanner::OnShowDetails() {
    QMessageBox::information(
        this,
        tr("XFA Architectural Scope & Limitations"),
        tr("<h3>XML Forms Architecture (XFA) Technical Disclosure</h3>"
           "<p><b>Standard:</b> Legacy XML Forms Architecture (XFA) specification.</p>"
           "<p><b>Status in PDF Standards:</b> Deprecated in PDF 2.0 (ISO 32000-2). Modern open-source PDF "
           "engines, including Chromium PDFium, Apple Quartz, and Mozilla PDF.js, do not support dynamic XFA form execution.</p>"
           "<p><b>PDFinity Scope:</b></p>"
           "<ul>"
           "<li><b>Standard AcroForms (ISO 32000-1):</b> 100% Fully Supported (Text, Checkbox, Radio, Dropdown, ListBox, Flattening).</li>"
           "<li><b>Static XFA:</b> Pre-rendered page content is displayed as static vector/raster visual layers.</li>"
           "<li><b>Dynamic XFA:</b> XML DOM manipulation, dynamic page reflow, and proprietary script engines are unsupported.</li>"
           "</ul>"
           "<p><b>Recommendation:</b> If interactive calculation is required for this specific document, please open it in a compatible legacy reader or request a standard ISO 32000 AcroForm version from the document publisher.</p>")
    );
}

} // namespace pdfinity::ui
