// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PreflightService.hpp"
#include "DocumentIntelligenceService.hpp"
#include "application/Logger.hpp"

namespace pdfinity::services {

PreflightService& PreflightService::Instance() {
    static PreflightService instance;
    return instance;
}

pdf_api::EngineResult<pdf_api::PreflightReport> PreflightService::RunPreflight(
    domain::DocumentModel& doc,
    const PreflightOptions& options
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<pdf_api::PreflightReport>::Fail(
            pdf_api::EngineError::Unknown, "Invalid document model for preflight"
        );
    }

    auto intelRes = DocumentIntelligenceService::Instance().AnalyzeDocument(doc);
    if (!intelRes.has_value()) {
        return pdf_api::EngineResult<pdf_api::PreflightReport>::Fail(
            intelRes.error, "Failed to analyze document: " + intelRes.errorMessage
        );
    }

    const auto& intel = intelRes.value;
    pdf_api::PreflightReport report;
    report.disclaimer =
        "Notice on Preflight Findings: Preflight inspections identify publishing risks, structural anomalies, and "
        "format limitations using automated deterministic rules and heuristic thresholds. Passing automated preflight "
        "does not constitute a formal warranty of commercial press readiness or legal compliance.";

    // 1. Check Fonts
    for (const auto& font : intel.fonts) {
        if (!font.isEmbedded) {
            pdf_api::PreflightFinding f;
            f.severity = options.requireAllFontsEmbedded ? pdf_api::PreflightSeverity::Error : pdf_api::PreflightSeverity::Warning;
            f.category = pdf_api::PreflightCategory::Font;
            f.pageIndex = font.pagesUsed.empty() ? -1 : font.pagesUsed[0];
            f.title = "Non-Embedded Font: " + font.baseFontName;
            f.description = "Font program is not embedded in the document. Viewers without this font installed will substitute alternative glyphs, altering layout.";
            f.remediation = "Re-export PDF with full font embedding enabled or convert text to vector paths.";
            f.isHeuristic = false; // Deterministic PDF structure check
            report.findings.push_back(f);
        }
    }

    // 2. Check Images
    for (const auto& img : intel.images) {
        double effectiveDpi = std::min(img.effectiveDpiX, img.effectiveDpiY);
        if (effectiveDpi > 0.0 && effectiveDpi < options.minImageDpi) {
            pdf_api::PreflightFinding f;
            f.severity = pdf_api::PreflightSeverity::Warning;
            f.category = pdf_api::PreflightCategory::Image;
            f.pageIndex = img.pageIndex;
            f.title = "Low Resolution Image (" + std::to_string(static_cast<int>(effectiveDpi)) + " DPI)";
            f.description = "Image on page " + std::to_string(img.pageIndex + 1) + " has an effective resolution below " +
                            std::to_string(static_cast<int>(options.minImageDpi)) + " DPI. It may appear blurry or pixelated in print.";
            f.remediation = "Replace with higher-resolution raster asset (at least 150 DPI for screen, 300 DPI for print).";
            f.isHeuristic = true; // Heuristic publishing guideline
            report.findings.push_back(f);
        }

        if (img.colorSpace == "DeviceCMYK") {
            pdf_api::PreflightFinding f;
            f.severity = pdf_api::PreflightSeverity::Info;
            f.category = pdf_api::PreflightCategory::Image;
            f.pageIndex = img.pageIndex;
            f.title = "CMYK Color Space Detected";
            f.description = "Image on page " + std::to_string(img.pageIndex + 1) + " uses DeviceCMYK colorspace.";
            f.remediation = "Verify target printer profile supports CMYK without unwanted color shift.";
            f.isHeuristic = false;
            report.findings.push_back(f);
        }
    }

    // 3. Check Page Boxes
    if (options.checkPageBoxInconsistencies) {
        for (const auto& box : intel.pageBoxes) {
            if (box.mediaBox.width <= 0 || box.mediaBox.height <= 0) {
                pdf_api::PreflightFinding f;
                f.severity = pdf_api::PreflightSeverity::Error;
                f.category = pdf_api::PreflightCategory::PageBox;
                f.pageIndex = box.pageIndex;
                f.title = "Invalid MediaBox Dimensions";
                f.description = "Page " + std::to_string(box.pageIndex + 1) + " specifies zero or negative MediaBox dimensions.";
                f.remediation = "Repair document page boundaries to standard paper dimensions.";
                f.isHeuristic = false;
                report.findings.push_back(f);
            }

            // Check if CropBox exceeds MediaBox
            if (box.cropBox.width > box.mediaBox.width * 1.05 || box.cropBox.height > box.mediaBox.height * 1.05) {
                pdf_api::PreflightFinding f;
                f.severity = pdf_api::PreflightSeverity::Warning;
                f.category = pdf_api::PreflightCategory::PageBox;
                f.pageIndex = box.pageIndex;
                f.title = "CropBox Exceeds MediaBox";
                f.description = "Page " + std::to_string(box.pageIndex + 1) + " specifies a CropBox larger than its enclosing MediaBox.";
                f.remediation = "Clip CropBox to fit inside MediaBox.";
                f.isHeuristic = false;
                report.findings.push_back(f);
            }
        }
    }

    // 4. Check Encryption & Security
    auto perms = doc.GetPermissions();
    if (perms.isEncrypted && options.warnOnEncryption) {
        pdf_api::PreflightFinding f;
        f.severity = pdf_api::PreflightSeverity::Warning;
        f.category = pdf_api::PreflightCategory::Encryption;
        f.pageIndex = -1;
        f.title = "Document Password Protection / Encryption Active";
        f.description = "Document is protected by encryption (" + perms.securityHandler + "). PDF/A and automated archiving strictly prohibit document encryption.";
        f.remediation = "Decrypt document or remove password before archiving or compliance submission.";
        f.isHeuristic = false;
        report.findings.push_back(f);
    }

    // 5. Check Metadata
    if (options.warnOnMissingMetadata) {
        if (intel.metadata.title.empty()) {
            pdf_api::PreflightFinding f;
            f.severity = pdf_api::PreflightSeverity::Info;
            f.category = pdf_api::PreflightCategory::Metadata;
            f.pageIndex = -1;
            f.title = "Missing Document Title";
            f.description = "Document Info dictionary lacks a Title attribute. Required for PDF/A and PDF/UA accessibility.";
            f.remediation = "Add document title in Document Intelligence metadata editor.";
            f.isHeuristic = false;
            report.findings.push_back(f);
        }
    }

    // Tally severities
    for (const auto& f : report.findings) {
        if (f.severity == pdf_api::PreflightSeverity::Error) report.totalErrors++;
        else if (f.severity == pdf_api::PreflightSeverity::Warning) report.totalWarnings++;
        else if (f.severity == pdf_api::PreflightSeverity::Info) report.totalInfos++;
    }

    report.isPassed = (report.totalErrors == 0);
    report.summary = "Preflight check finished: " + std::to_string(report.totalErrors) + " error(s), " +
                     std::to_string(report.totalWarnings) + " warning(s), " +
                     std::to_string(report.totalInfos) + " notice(s).";

    LOG_INFO("Preflight", report.summary);
    return pdf_api::EngineResult<pdf_api::PreflightReport>::Ok(std::move(report));
}

} // namespace pdfinity::services
