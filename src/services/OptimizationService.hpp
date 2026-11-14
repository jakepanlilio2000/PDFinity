// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>
#include <string>
#include <functional>

namespace pdfinity::services {

class OptimizationService {
public:
    static OptimizationService& Instance();

    // Optimize and compress a PDF document
    pdf_api::EngineResult<pdf_api::OptimizationReport> OptimizePdf(
        domain::DocumentModel& doc,
        const std::filesystem::path& srcPath,
        const std::filesystem::path& dstPath,
        const pdf_api::OptimizationOptions& options,
        std::function<void(int current, int total, const std::string& status)> progressCallback = nullptr
    );

    // Apply preset values to an options structure
    static pdf_api::OptimizationOptions GetOptionsForPreset(pdf_api::CompressionPreset preset);

    // Analyze document for potential optimization savings without writing output
    pdf_api::OptimizationReport AnalyzeDocument(
        domain::DocumentModel& doc,
        const std::filesystem::path& srcPath,
        const pdf_api::OptimizationOptions& options
    );

private:
    OptimizationService() = default;
};

} // namespace pdfinity::services
