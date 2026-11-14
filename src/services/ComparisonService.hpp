// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <QImage>
#include <vector>
#include <string>

namespace pdfinity::services {

class ComparisonService {
public:
    static ComparisonService& Instance();

    // Compare two documents: text and visual
    pdf_api::EngineResult<pdf_api::DocumentComparisonReport> CompareDocuments(
        domain::DocumentModel& docA,
        domain::DocumentModel& docB,
        double renderDpi = 150.0
    );

    // Generate visual diff image between two rendered pages
    // Identical pixels shown in muted translucent tone, differences highlighted in vivid red
    QImage GenerateVisualDiffImage(
        const QImage& imgA,
        const QImage& imgB,
        double* outDiffPercent = nullptr,
        int* outDiffPixels = nullptr
    );

    // Perform line-by-line text diff between two texts
    std::vector<pdf_api::TextDiffLine> DiffPageTexts(
        int pageIndexA,
        const std::string& textA,
        int pageIndexB,
        const std::string& textB
    );

private:
    ComparisonService() = default;
};

} // namespace pdfinity::services
