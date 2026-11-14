// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "security/SecureString.hpp"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

namespace pdfinity::ui {

class DigitalSignatureDialog : public QDialog {
    Q_OBJECT

public:
    explicit DigitalSignatureDialog(QWidget* parent = nullptr);
    ~DigitalSignatureDialog() override = default;

    [[nodiscard]] pdf_api::CryptographicSignOptions GetSignOptions() const;
    [[nodiscard]] security::SecureString GetPassword() const;

private slots:
    void OnAccept();
    void OnManageCertificates();

private:
    void SetupUi();
    void RefreshCertificates();

    QComboBox* m_certCombo{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QLineEdit* m_reasonEdit{nullptr};
    QLineEdit* m_locationEdit{nullptr};
    QLineEdit* m_contactEdit{nullptr};
    QCheckBox* m_includeVisualCheck{nullptr};

    pdf_api::CryptographicSignOptions m_options;
    security::SecureString m_password;
};

} // namespace pdfinity::ui
