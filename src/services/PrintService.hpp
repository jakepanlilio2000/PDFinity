// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <QObject>
#include <QWidget>
#include "domain/DocumentModel.hpp"
#include <memory>

namespace pdfinity::services {

class PrintService : public QObject {
    Q_OBJECT

public:
    explicit PrintService(QObject* parent = nullptr);
    ~PrintService() override = default;

    static bool PrintDocument(QWidget* parent, const std::shared_ptr<domain::DocumentModel>& doc);
    static bool PrintPreview(QWidget* parent, const std::shared_ptr<domain::DocumentModel>& doc);

signals:
    void PrintProgress(int currentPage, int totalPages);
    void PrintCompleted(bool success);
};

} // namespace pdfinity::services
