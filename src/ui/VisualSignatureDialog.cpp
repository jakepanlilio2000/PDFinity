// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "VisualSignatureDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QPainter>
#include <QImage>
#include <QMessageBox>

namespace pdfinity::ui {

// SignatureCanvasWidget
SignatureCanvasWidget::SignatureCanvasWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 160);
    setStyleSheet("background-color: #FAFAFD; border: 1px dashed #A0A0C0; border-radius: 4px;");
}

void SignatureCanvasWidget::Clear() {
    m_paths.clear();
    m_currentPath.clear();
    update();
}

void SignatureCanvasWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(10, 30, 120), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);

    for (const auto& path : m_paths) {
        for (size_t i = 1; i < path.size(); ++i) {
            p.drawLine(QPointF(path[i-1].x, path[i-1].y), QPointF(path[i].x, path[i].y));
        }
    }

    if (m_currentPath.size() > 1) {
        for (size_t i = 1; i < m_currentPath.size(); ++i) {
            p.drawLine(QPointF(m_currentPath[i-1].x, m_currentPath[i-1].y), QPointF(m_currentPath[i].x, m_currentPath[i].y));
        }
    }
}

void SignatureCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_currentPath.clear();
        m_currentPath.push_back({event->position().x(), event->position().y()});
    }
}

void SignatureCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDrawing) {
        m_currentPath.push_back({event->position().x(), event->position().y()});
        update();
    }
}

void SignatureCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        if (m_currentPath.size() > 1) {
            m_paths.push_back(m_currentPath);
        }
        m_currentPath.clear();
        update();
    }
}

// VisualSignatureDialog
VisualSignatureDialog::VisualSignatureDialog(int pageIndex, QWidget* parent)
    : QDialog(parent), m_pageIndex(pageIndex) {
    setWindowTitle(tr("Place Visual Signature Appearance"));
    setMinimumWidth(500);
    SetupUi();
}

void VisualSignatureDialog::SetupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // TRUTH IN UI BADGE: Explicitly disclose that visual signatures have NO cryptographic properties
    auto* banner = new QLabel(
        tr("⚠️ <b>VISUAL SIGNATURE ONLY:</b> This is a graphical appearance stamp. "
           "It does <b>not</b> digitally encrypt the document, verify signer identity, or detect file tampering. "
           "Use <i>Cryptographic Digital Sign...</i> for legally binding PKCS#7 signatures."),
        this
    );
    banner->setWordWrap(true);
    banner->setStyleSheet(
        "background-color: #FFF9E6; color: #855A00; border: 1px solid #FFE082; "
        "border-radius: 4px; padding: 10px; font-size: 11px;"
    );
    mainLayout->addWidget(banner);

    m_tabWidget = new QTabWidget(this);

    // TAB 1: DRAW
    auto* drawWidget = new QWidget(this);
    auto* drawLayout = new QVBoxLayout(drawWidget);
    m_canvas = new SignatureCanvasWidget(this);
    drawLayout->addWidget(m_canvas);

    auto* clearBtn = new QPushButton(tr("Clear Canvas"), this);
    connect(clearBtn, &QPushButton::clicked, this, &VisualSignatureDialog::OnClearDrawing);
    drawLayout->addWidget(clearBtn, 0, Qt::AlignRight);
    m_tabWidget->addTab(drawWidget, tr("✍️ Draw"));

    // TAB 2: TYPE
    auto* typeWidget = new QWidget(this);
    auto* typeLayout = new QFormLayout(typeWidget);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Your Full Name"));
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Job Title or Organization"));

    m_typePreview = new QLabel(tr("<i>Preview will appear here</i>"), this);
    m_typePreview->setStyleSheet(
        "background-color: #FAFAFD; border: 1px dashed #A0A0C0; padding: 16px; "
        "font-family: 'Segoe Script', cursive; font-size: 20px; color: #0A1E78;"
    );
    m_typePreview->setAlignment(Qt::AlignCenter);

    auto updatePreview = [this]() {
        QString text = m_nameEdit->text().trimmed();
        if (text.isEmpty()) text = tr("Preview will appear here");
        m_typePreview->setText(text);
    };
    connect(m_nameEdit, &QLineEdit::textChanged, updatePreview);

    typeLayout->addRow(tr("Signer Name:"), m_nameEdit);
    typeLayout->addRow(tr("Title / Affiliation:"), m_titleEdit);
    typeLayout->addRow(tr("Appearance:"), m_typePreview);
    m_tabWidget->addTab(typeWidget, tr("⌨️ Type"));

    // TAB 3: IMPORT IMAGE
    auto* imgWidget = new QWidget(this);
    auto* imgLayout = new QVBoxLayout(imgWidget);
    auto* browseLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit(this);
    m_imagePathEdit->setReadOnly(true);
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &VisualSignatureDialog::OnBrowseImage);
    browseLayout->addWidget(m_imagePathEdit);
    browseLayout->addWidget(browseBtn);
    imgLayout->addLayout(browseLayout);

    m_imagePreview = new QLabel(tr("No image selected"), this);
    m_imagePreview->setMinimumHeight(100);
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setStyleSheet("background-color: #FAFAFD; border: 1px dashed #A0A0C0;");
    imgLayout->addWidget(m_imagePreview);
    m_tabWidget->addTab(imgWidget, tr("🖼️ Import Image"));

    mainLayout->addWidget(m_tabWidget);

    // Coordinates Box
    auto* posGroup = new QGroupBox(tr("Placement on Page"), this);
    auto* posLayout = new QHBoxLayout(posGroup);

    m_xSpin = new QDoubleSpinBox(this);
    m_xSpin->setRange(0, 2000);
    m_xSpin->setValue(72.0); // 1 inch from left
    m_ySpin = new QDoubleSpinBox(this);
    m_ySpin->setRange(0, 2000);
    m_ySpin->setValue(100.0);
    m_wSpin = new QDoubleSpinBox(this);
    m_wSpin->setRange(50, 1000);
    m_wSpin->setValue(200.0);
    m_hSpin = new QDoubleSpinBox(this);
    m_hSpin->setRange(20, 500);
    m_hSpin->setValue(75.0);

    posLayout->addWidget(new QLabel(tr("X:")));
    posLayout->addWidget(m_xSpin);
    posLayout->addWidget(new QLabel(tr("Y:")));
    posLayout->addWidget(m_ySpin);
    posLayout->addWidget(new QLabel(tr("Width:")));
    posLayout->addWidget(m_wSpin);
    posLayout->addWidget(new QLabel(tr("Height:")));
    posLayout->addWidget(m_hSpin);

    mainLayout->addWidget(posGroup);

    // Dialog Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    auto* applyBtn = new QPushButton(tr("Apply Signature"), this);
    applyBtn->setDefault(true);
    connect(applyBtn, &QPushButton::clicked, this, &VisualSignatureDialog::OnAccept);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(applyBtn);
    mainLayout->addLayout(btnLayout);
}

void VisualSignatureDialog::OnClearDrawing() {
    if (m_canvas) m_canvas->Clear();
}

void VisualSignatureDialog::OnBrowseImage() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select Signature Image"), "", tr("Images (*.png *.jpg *.jpeg *.bmp)")
    );
    if (!path.isEmpty()) {
        m_imagePathEdit->setText(path);
        QImage img(path);
        if (!img.isNull()) {
            QImage bgra = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            m_imageWidth = bgra.width();
            m_imageHeight = bgra.height();
            m_imageBgra.resize(bgra.sizeInBytes());
            std::memcpy(m_imageBgra.data(), bgra.bits(), bgra.sizeInBytes());
            m_imagePreview->setPixmap(QPixmap::fromImage(img).scaled(200, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void VisualSignatureDialog::OnAccept() {
    m_resultData.pageIndex = m_pageIndex;
    m_resultData.rect.x = m_xSpin->value();
    m_resultData.rect.y = m_ySpin->value();
    m_resultData.rect.width = m_wSpin->value();
    m_resultData.rect.height = m_hSpin->value();
    m_resultData.isCryptographicBound = false; // Strictly visual!

    if (m_tabWidget->currentIndex() == 0) {
        // Draw tab
        m_resultData.inkPaths = m_canvas->GetPaths();
        m_resultData.signerName = "Handwritten Signature";
        m_resultData.title = "Visual Stamp";
    } else if (m_tabWidget->currentIndex() == 1) {
        // Type tab
        m_resultData.signerName = m_nameEdit->text().trimmed().toStdString();
        m_resultData.title = m_titleEdit->text().trimmed().toStdString();
    } else {
        // Import tab
        m_resultData.imageBgra = m_imageBgra;
        m_resultData.imageWidth = m_imageWidth;
        m_resultData.imageHeight = m_imageHeight;
        m_resultData.signerName = "Imported Signature Image";
        m_resultData.title = "Visual Image Stamp";
    }

    accept();
}

pdf_api::VisualSignatureData VisualSignatureDialog::GetSignatureData() const {
    return m_resultData;
}

} // namespace pdfinity::ui
