// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "OptimizationService.hpp"
#include "SanitizationService.hpp"
#include "application/Logger.hpp"

namespace pdfinity::services {

OptimizationService& OptimizationService::Instance() {
    static OptimizationService instance;
    return instance;
}

pdf_api::OptimizationOptions OptimizationService::GetOptionsForPreset(pdf_api::CompressionPreset preset) {
    pdf_api::OptimizationOptions opt;
    opt.preset = preset;

    switch (preset) {
        case pdf_api::CompressionPreset::PrepressLossless:
            opt.targetImageDpi = 300;
            opt.jpegQuality = 95;
            opt.stripMetadata = false;
            opt.removeAnnotations = false;
            opt.cleanUnusedStreams = true;
            break;
        case pdf_api::CompressionPreset::RecommendedStandard:
            opt.targetImageDpi = 150;
            opt.jpegQuality = 80;
            opt.stripMetadata = true;
            opt.removeAnnotations = false;
            opt.cleanUnusedStreams = true;
            break;
        case pdf_api::CompressionPreset::MaximumScreenWeb:
            opt.targetImageDpi = 96;
            opt.jpegQuality = 65;
            opt.stripMetadata = true;
            opt.removeAnnotations = true;
            opt.cleanUnusedStreams = true;
            break;
        case pdf_api::CompressionPreset::Custom:
        default:
            break;
    }
    return opt;
}

pdf_api::OptimizationReport OptimizationService::AnalyzeDocument(
    domain::DocumentModel& doc,
    const std::filesystem::path& srcPath,
    const pdf_api::OptimizationOptions& options
) {
    pdf_api::OptimizationReport report;
    std::error_code ec;
    if (std::filesystem::exists(srcPath, ec)) {
        report.originalSizeBytes = std::filesystem::file_size(srcPath, ec);
    }

    int pageCount = doc.GetPageCount();
    int potentialImages = 0;

    for (int p = 0; p < pageCount; ++p) {
        auto objsRes = doc.GetPageObjects(p);
        if (objsRes.has_value()) {
            for (const auto& obj : objsRes.value) {
                if (obj.type == pdf_api::PageObjectType::Image) {
                    double targetW = obj.bounds.width;
                    if (targetW > 0 && obj.pixelWidth > 0) {
                        double effectiveDpi = (static_cast<double>(obj.pixelWidth) / targetW) * 72.0;
                        if (effectiveDpi > options.targetImageDpi * 1.15) {
                            potentialImages++;
                        }
                    }
                }
            }
        }
    }

    report.imagesResampled = potentialImages;
    report.isSuccess = true;
    return report;
}

pdf_api::EngineResult<pdf_api::OptimizationReport> OptimizationService::OptimizePdf(
    domain::DocumentModel& doc,
    const std::filesystem::path& srcPath,
    const std::filesystem::path& dstPath,
    const pdf_api::OptimizationOptions& options,
    std::function<void(int current, int total, const std::string& status)> progressCallback
) {
    std::error_code ec;
    if (!std::filesystem::exists(srcPath, ec)) {
        return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(
            pdf_api::EngineError::FileNotFound, "Source file not found: " + srcPath.string()
        );
    }

    if (std::filesystem::equivalent(srcPath, dstPath, ec)) {
        return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(
            pdf_api::EngineError::InvalidFormat, "In-place optimization prohibited. Must save to separate destination file."
        );
    }

    pdf_api::OptimizationReport report;
    report.originalSizeBytes = std::filesystem::file_size(srcPath, ec);

    if (progressCallback) progressCallback(1, 4, "Analyzing document and downsampling images...");

    // 1. Engine-level image downsampling & annotation purge
    auto optRes = doc.OptimizeDocument(options);
    if (!optRes.has_value()) {
        report.isSuccess = false;
        report.errorMessage = optRes.errorMessage;
        return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(optRes.error, optRes.errorMessage);
    }
    report.imagesResampled = optRes.value;

    if (progressCallback) progressCallback(2, 4, "Rebuilding clean PDF streams...");

    // 2. Clean serialization to dstPath (FPDF_NO_INCREMENTAL rebuild)
    auto saveRes = doc.SaveCopy(dstPath);
    if (!saveRes.has_value()) {
        report.isSuccess = false;
        report.errorMessage = saveRes.errorMessage;
        return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(saveRes.error, saveRes.errorMessage);
    }

    if (progressCallback) progressCallback(3, 4, "Scrubbing unused metadata and streams...");

    // 3. Metadata and stream cleanup if requested
    if (options.stripMetadata) {
        pdf_api::SanitizationOptions sanOpt;
        sanOpt.scrubInfoMetadata = true;
        sanOpt.scrubXmpMetadata = true;
        SanitizationService::SanitizeFile(dstPath, sanOpt);
    }

    if (progressCallback) progressCallback(4, 4, "Validating output document integrity...");

    // 4. Output validation: check file exists, can be opened, page count matches
    if (!std::filesystem::exists(dstPath, ec) || std::filesystem::file_size(dstPath, ec) == 0) {
        report.isSuccess = false;
        report.errorMessage = "Optimized output file is missing or empty";
        return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(pdf_api::EngineError::SaveFailed, report.errorMessage);
    }

    auto engine = doc.GetEngine();
    if (engine) {
        auto verifyOpen = engine->OpenDocument(dstPath);
        if (!verifyOpen.has_value()) {
            report.isSuccess = false;
            report.errorMessage = "Optimized PDF failed integrity check (unable to reopen)";
            return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(verifyOpen.error, report.errorMessage);
        }
        int verifyPages = engine->GetPageCount(verifyOpen.value);
        engine->CloseDocument(verifyOpen.value);

        if (verifyPages != doc.GetPageCount()) {
            report.isSuccess = false;
            report.errorMessage = "Page count mismatch in optimized output document";
            return pdf_api::EngineResult<pdf_api::OptimizationReport>::Fail(pdf_api::EngineError::CorruptedData, report.errorMessage);
        }
    }

    report.optimizedSizeBytes = std::filesystem::file_size(dstPath, ec);
    if (report.originalSizeBytes > 0) {
        double savedBytes = static_cast<double>(report.originalSizeBytes) - static_cast<double>(report.optimizedSizeBytes);
        report.compressionRatio = (savedBytes / static_cast<double>(report.originalSizeBytes)) * 100.0;
    }
    report.isSuccess = true;

    LOG_INFO("Optimizer", "Document optimized successfully: " + std::to_string(report.originalSizeBytes) +
             " bytes -> " + std::to_string(report.optimizedSizeBytes) + " bytes (" +
             std::to_string(static_cast<int>(report.compressionRatio)) + "% saved)");

    return pdf_api::EngineResult<pdf_api::OptimizationReport>::Ok(report);
}

} // namespace pdfinity::services
