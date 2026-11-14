// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AccessibilityService.hpp"
#include "DocumentIntelligenceService.hpp"
#include "application/Logger.hpp"

#include <fstream>
#include <sstream>

namespace pdfinity::services {

AccessibilityService& AccessibilityService::Instance() {
    static AccessibilityService instance;
    return instance;
}

pdf_api::EngineResult<pdf_api::AccessibilityReport> AccessibilityService::InspectAccessibility(
    domain::DocumentModel& doc
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<pdf_api::AccessibilityReport>::Fail(
            pdf_api::EngineError::Unknown, "Invalid document model for accessibility check"
        );
    }

    auto intelRes = DocumentIntelligenceService::Instance().AnalyzeDocument(doc);
    if (!intelRes.has_value()) {
        return pdf_api::EngineResult<pdf_api::AccessibilityReport>::Fail(
            intelRes.error, "Failed to analyze document for accessibility: " + intelRes.errorMessage
        );
    }

    const auto& intel = intelRes.value;
    pdf_api::AccessibilityReport report;
    report.legalDisclaimer =
        "Accessibility Evaluation Notice: Automated testing evaluates technical PDF/UA (ISO 14289-1) and WCAG 2.1 "
        "structural prerequisites, including Tagged PDF markers, alternative text, language tags, and structure trees. "
        "Automated tools cannot evaluate semantic clarity, visual reading order accuracy, or color contrast sufficiency. "
        "Passing automated checks does not confer a formal legal warranty of compliance with ADA Title III, Section 508, "
        "or EN 301 549 without qualified human accessibility review.";

    // 1. Tagged PDF Check
    report.isTagged = intel.isTaggedPdf;
    if (!report.isTagged) {
        pdf_api::AccessibilityFinding f;
        f.severity = pdf_api::PreflightSeverity::Error;
        f.pageIndex = -1;
        f.ruleId = "WCAG-4.1.2 / PDF/UA-1 7.1";
        f.issue = "Document is not a Tagged PDF (missing /MarkInfo << /Marked true >>). Screen readers cannot interpret content structure.";
        f.remediation = "Re-author or export document with Tagged PDF enabled.";
        report.findings.push_back(f);
    }

    // 2. Structure Tree Root Check
    bool hasStructTree = false;
    if (!doc.GetPath().empty() && std::filesystem::exists(doc.GetPath())) {
        std::ifstream in(doc.GetPath(), std::ios::binary);
        if (in.is_open()) {
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            if (content.find("/StructTreeRoot") != std::string::npos) {
                hasStructTree = true;
            }
        }
    }
    report.hasStructTree = hasStructTree;
    if (!hasStructTree && report.isTagged) {
        pdf_api::AccessibilityFinding f;
        f.severity = pdf_api::PreflightSeverity::Error;
        f.pageIndex = -1;
        f.ruleId = "PDF/UA-1 7.1";
        f.issue = "Document marks itself as tagged but lacks a logical Structure Tree Root (/StructTreeRoot).";
        f.remediation = "Rebuild logical structure hierarchy.";
        report.findings.push_back(f);
    }

    // 3. Document Language Check
    report.hasLanguageSpecified = (intel.language != "Unspecified" && !intel.language.empty());
    report.documentLanguage = intel.language;
    if (!report.hasLanguageSpecified) {
        pdf_api::AccessibilityFinding f;
        f.severity = pdf_api::PreflightSeverity::Warning;
        f.pageIndex = -1;
        f.ruleId = "WCAG-3.1.1";
        f.issue = "Document lacks a primary natural language specification (/Lang). Text-to-speech engines cannot select appropriate pronunciation dictionaries.";
        f.remediation = "Specify document language (e.g. 'en-US') in Document Properties or Catalog.";
        report.findings.push_back(f);
    }

    // 4. Document Title Check
    if (intel.metadata.title.empty()) {
        pdf_api::AccessibilityFinding f;
        f.severity = pdf_api::PreflightSeverity::Warning;
        f.pageIndex = -1;
        f.ruleId = "WCAG-2.4.2";
        f.issue = "Document lacks a descriptive title in its metadata dictionary.";
        f.remediation = "Set a descriptive document title in Document Intelligence metadata editor.";
        report.findings.push_back(f);
    }

    // 5. Image Alternative Text Check
    for (const auto& img : intel.images) {
        if (img.markedContentId < 0) {
            report.totalImagesWithoutAltText++;
            pdf_api::AccessibilityFinding f;
            f.severity = pdf_api::PreflightSeverity::Warning;
            f.pageIndex = img.pageIndex;
            f.ruleId = "WCAG-1.1.1";
            f.issue = "Image on page " + std::to_string(img.pageIndex + 1) + " lacks alternative text (/Alt tag). Assistive technologies cannot describe image content.";
            f.remediation = "Add descriptive alternative text or mark image as decorative artifact.";
            report.findings.push_back(f);
        }
    }

    // 6. Heading Structure Heuristics
    // Inspect bookmarks as proxy for document heading structure
    report.totalHeadingsFound = static_cast<int>(intel.bookmarks.size());
    if (report.totalHeadingsFound == 0 && doc.GetPageCount() > 3) {
        pdf_api::AccessibilityFinding f;
        f.severity = pdf_api::PreflightSeverity::Info;
        f.pageIndex = -1;
        f.ruleId = "WCAG-1.3.1 / WCAG-2.4.5";
        f.issue = "Document contains " + std::to_string(doc.GetPageCount()) + " pages but lacks headings or bookmarks for navigation.";
        f.remediation = "Generate document outline bookmarks or tag heading elements (H1, H2) for rapid section jumping.";
        report.findings.push_back(f);
    }

    report.passedBasicAccessibility = report.isTagged && report.hasLanguageSpecified && (report.totalImagesWithoutAltText == 0);

    LOG_INFO("Accessibility", "Accessibility check finished: Tagged=" + std::string(report.isTagged ? "YES" : "NO") +
             ", Language=" + report.documentLanguage + ", ImagesWithoutAlt=" + std::to_string(report.totalImagesWithoutAltText));

    return pdf_api::EngineResult<pdf_api::AccessibilityReport>::Ok(std::move(report));
}

} // namespace pdfinity::services
