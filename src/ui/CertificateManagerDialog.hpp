// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>

namespace pdfinity::ui {

class CertificateManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CertificateManagerDialog(QWidget* parent = nullptr);
    ~CertificateManagerDialog() override = default;

private slots:
    void OnImportCertificate();
    void OnDeleteCertificate();
    void OnRefreshList();

private:
    void SetupUi();

    QTableWidget* m_table{nullptr};
    QPushButton* m_importBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};
};

} // namespace pdfinity::ui
