// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpToolCatalog.hpp"
#include "McpSecurityPolicy.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"
#include "domain/DocumentModel.hpp"
#include "services/DocumentIntelligenceService.hpp"
#include "services/ConversionService.hpp"
#include "services/PreflightService.hpp"
#include "services/SignatureService.hpp"
#include "services/OptimizationService.hpp"
#include "services/PdfAComplianceService.hpp"
#include "services/OcrService.hpp"
#include "services/SanitizationService.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pdfinity::mcp {

namespace {

QJsonObject MakeProperty(const QString& type, const QString& desc, const QJsonArray& enumValues = {}) {
    QJsonObject prop;
    prop["type"] = type;
    prop["description"] = desc;
    if (!enumValues.isEmpty()) {
        prop["enum"] = enumValues;
    }
    return prop;
}

} // namespace

McpToolCatalog& McpToolCatalog::Instance() {
    static McpToolCatalog instance;
    return instance;
}

McpToolCatalog::McpToolCatalog() {
    RegisterBuiltInTools();
}

const std::vector<McpToolDefinition>& McpToolCatalog::GetAllTools() const {
    return m_tools;
}

std::optional<McpToolDefinition> McpToolCatalog::FindTool(const std::string& name) const {
    for (const auto& t : m_tools) {
        if (t.name == name) return t;
    }
    return std::nullopt;
}

void McpToolCatalog::RegisterBuiltInTools() {
    m_tools.clear();

    // 1. pdf_inspect (ReadOnly)
    {
        McpToolDefinition t;
        t.name = "pdf_inspect";
        t.description = "Inspects PDF structure: metadata, page dimensions, font usage, image assets, encryption status, and compliance summaries.";
        t.category = McpToolCategory::ReadOnly;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["path"] = MakeProperty("string", "Absolute path to the target PDF file (must be inside an approved root)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 2. pdf_extract_text (ReadOnly)
    {
        McpToolDefinition t;
        t.name = "pdf_extract_text";
        t.description = "Extracts plain text from the entire document or a specific page range.";
        t.category = McpToolCategory::ReadOnly;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["path"] = MakeProperty("string", "Absolute path to the target PDF file");
        props["startPage"] = MakeProperty("integer", "Starting page index (1-based, optional)");
        props["endPage"] = MakeProperty("integer", "Ending page index (1-based, optional)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 3. pdf_search (ReadOnly)
    {
        McpToolDefinition t;
        t.name = "pdf_search";
        t.description = "Searches for textual keywords or patterns within the document and returns occurrence coordinates.";
        t.category = McpToolCategory::ReadOnly;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["path"] = MakeProperty("string", "Absolute path to the target PDF file");
        props["query"] = MakeProperty("string", "Search query keyword");
        props["matchCase"] = MakeProperty("boolean", "Whether search should be case-sensitive (default: false)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path", "query"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 4. pdf_preflight_validate (ReadOnly)
    {
        McpToolDefinition t;
        t.name = "pdf_preflight_validate";
        t.description = "Runs comprehensive preflight verification: PDF/A profile rules, font embedding, page boxes, and encryption integrity.";
        t.category = McpToolCategory::ReadOnly;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["path"] = MakeProperty("string", "Absolute path to the target PDF file");
        props["profile"] = MakeProperty("string", "Target compliance profile to check", QJsonArray{"PdfA1b", "PdfA2b", "General"});
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 5. pdf_verify_signatures (SecuritySensitive)
    {
        McpToolDefinition t;
        t.name = "pdf_verify_signatures";
        t.description = "Validates cryptographic signatures, certificate validity, trust chains, and post-sign document modifications.";
        t.category = McpToolCategory::SecuritySensitive;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["path"] = MakeProperty("string", "Absolute path to the signed PDF file");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 6. pdf_optimize (Write)
    {
        McpToolDefinition t;
        t.name = "pdf_optimize";
        t.description = "Applies stream compression, font deduplication, and image downsampling to produce an optimized output PDF.";
        t.category = McpToolCategory::Write;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["inputPath"] = MakeProperty("string", "Absolute path to the input PDF file");
        props["outputPath"] = MakeProperty("string", "Destination path for optimized PDF (cannot be same as input)");
        props["preset"] = MakeProperty("string", "Compression preset", QJsonArray{"Screen", "Recommended", "Prepress"});
        props["targetDpi"] = MakeProperty("integer", "Target image downsampling DPI (e.g. 96, 150, 300)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"inputPath", "outputPath"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 7. pdf_convert_to_pdfa (Write)
    {
        McpToolDefinition t;
        t.name = "pdf_convert_to_pdfa";
        t.description = "Converts document to ISO 19005 compliant PDF/A archival format, embedding required metadata and color output intents.";
        t.category = McpToolCategory::Write;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["inputPath"] = MakeProperty("string", "Absolute path to the source PDF file");
        props["outputPath"] = MakeProperty("string", "Destination path for converted PDF/A document");
        props["profile"] = MakeProperty("string", "Target profile", QJsonArray{"PdfA1b", "PdfA2b"});
        schema["properties"] = props;
        schema["required"] = QJsonArray{"inputPath", "outputPath"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 8. pdf_ocr (Write)
    {
        McpToolDefinition t;
        t.name = "pdf_ocr";
        t.description = "Performs optical character recognition on scanned page images and injects a searchable text layer.";
        t.category = McpToolCategory::Write;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["inputPath"] = MakeProperty("string", "Absolute path to the source scanned PDF");
        props["outputPath"] = MakeProperty("string", "Destination path for searchable PDF");
        props["language"] = MakeProperty("string", "OCR language pack code (default: 'eng')");
        props["dpi"] = MakeProperty("integer", "Rendering DPI for OCR analysis (default: 150)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"inputPath", "outputPath"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 9. pdf_flatten_forms (Destructive)
    {
        McpToolDefinition t;
        t.name = "pdf_flatten_forms";
        t.description = "Permanently converts interactive AcroForm fields into static visual page content. Destructive: form fields can no longer be edited.";
        t.category = McpToolCategory::Destructive;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["inputPath"] = MakeProperty("string", "Absolute path to the source interactive PDF");
        props["outputPath"] = MakeProperty("string", "Destination path for flattened PDF");
        props["forPrint"] = MakeProperty("boolean", "Flatten for print presentation (default: false)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"inputPath", "outputPath"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 10. pdf_redact_and_sanitize (Destructive)
    {
        McpToolDefinition t;
        t.name = "pdf_redact_and_sanitize";
        t.description = "Permanently scrubs document metadata, XMP streams, embedded JavaScript, attachments, and hidden layer content.";
        t.category = McpToolCategory::Destructive;
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        props["inputPath"] = MakeProperty("string", "Absolute path to the source PDF");
        props["outputPath"] = MakeProperty("string", "Destination path for sanitized PDF");
        props["scrubMetadata"] = MakeProperty("boolean", "Scrub Info and XMP metadata (default: true)");
        props["removeScripts"] = MakeProperty("boolean", "Remove embedded JavaScript actions (default: true)");
        props["removeAttachments"] = MakeProperty("boolean", "Remove embedded file attachments (default: true)");
        schema["properties"] = props;
        schema["required"] = QJsonArray{"inputPath", "outputPath"};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }

    // 11. mcp_list_approved_roots (Admin)
    {
        McpToolDefinition t;
        t.name = "mcp_list_approved_roots";
        t.description = "Lists all directory roots approved by the operator for MCP tool read/write access and the active consent policy.";
        t.category = McpToolCategory::Admin;
        QJsonObject schema;
        schema["type"] = "object";
        schema["properties"] = QJsonObject{};
        t.inputSchema = schema;
        m_tools.push_back(t);
    }
}

McpToolResult McpToolCatalog::ExecuteTool(const std::string& name, const QJsonObject& arguments) {
    auto toolOpt = FindTool(name);
    if (!toolOpt.has_value()) {
        return McpToolResult::Error("Unknown tool: " + name);
    }

    const auto& tool = toolOpt.value();
    auto& policy = McpSecurityPolicy::Instance();

    // 1. Authorize tool execution via security policy
    auto authRes = policy.AuthorizeTool(tool, arguments);
    if (!authRes.allowed) {
        policy.RecordAuditLog("mcp_client", "tools/call", name, "DENIED", authRes.errorMessage);
        return McpToolResult::Error(authRes.errorMessage);
    }

    auto engine = app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        return McpToolResult::Error("Internal Error: PDF engine is not initialized");
    }

    // 2. Dispatch tool invocations
    if (name == "mcp_list_approved_roots") {
        auto roots = policy.GetApprovedRoots();
        QJsonArray arr;
        for (const auto& r : roots) {
            arr.append(QString::fromStdString(r.string()));
        }
        QJsonObject res;
        res["approvedRoots"] = arr;
        res["consentPolicy"] = QString::fromStdString(ConsentPolicyToString(policy.GetConsentPolicy()));
        res["allowDestructive"] = policy.IsAllowDestructive();
        res["mcpEnabled"] = policy.IsEnabled();

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Listed approved roots (" + std::to_string(roots.size()) + ")");
        return McpToolResult::Success(QString(QJsonDocument(res).toJson(QJsonDocument::Indented)).toStdString(), res);
    }

    // Input path validation
    std::filesystem::path inPath;
    if (arguments.contains("path")) {
        inPath = arguments["path"].toString().toStdString();
    } else if (arguments.contains("inputPath")) {
        inPath = arguments["inputPath"].toString().toStdString();
    }

    auto pathCheck = policy.ValidatePath(inPath, true);
    if (!pathCheck.allowed) {
        policy.RecordAuditLog("mcp_client", "tools/call", name, "BLOCKED", pathCheck.errorMessage);
        return McpToolResult::Error(pathCheck.errorMessage);
    }

    // Destination path validation for write/destructive tools
    std::filesystem::path outPath;
    if (arguments.contains("outputPath")) {
        outPath = arguments["outputPath"].toString().toStdString();
        // Destination directory must be within approved roots
        auto outDir = outPath.parent_path();
        if (outDir.empty()) outDir = ".";
        auto outCheck = policy.ValidatePath(outDir, false);
        if (!outCheck.allowed) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "BLOCKED", "Output path outside approved roots: " + outPath.string());
            return McpToolResult::Error("Security Violation: Output path must reside within an approved directory root: " + outPath.string());
        }

        // Source immutability check: output cannot equal input
        std::error_code eqEc;
        if (inPath == outPath || (std::filesystem::exists(outPath) && std::filesystem::equivalent(inPath, outPath, eqEc))) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "BLOCKED", "Source immutability violation: output equals input");
            return McpToolResult::Error("Security Violation: Destination cannot overwrite the source input file directly.");
        }
    }

    // Tool: pdf_inspect
    if (name == "pdf_inspect") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);
        auto reportRes = services::DocumentIntelligenceService::Instance().AnalyzeDocument(doc);
        if (!reportRes.has_value()) {
            return McpToolResult::Error("Document analysis failed: " + reportRes.errorMessage);
        }
        const auto& report = reportRes.value;

        QJsonObject out;
        out["pageCount"] = doc.GetPageCount();
        out["fileSizeBytes"] = static_cast<qint64>(doc.GetFileSize());
        out["isTaggedPdf"] = report.isTaggedPdf;
        out["language"] = QString::fromStdString(report.language);

        QJsonObject meta;
        meta["title"] = QString::fromStdString(report.metadata.title);
        meta["author"] = QString::fromStdString(report.metadata.author);
        meta["subject"] = QString::fromStdString(report.metadata.subject);
        meta["creator"] = QString::fromStdString(report.metadata.creator);
        meta["producer"] = QString::fromStdString(report.metadata.producer);
        out["metadata"] = meta;
        out["fontCount"] = static_cast<int>(report.fonts.size());
        out["imageCount"] = static_cast<int>(report.images.size());
        out["attachmentCount"] = static_cast<int>(report.attachments.size());

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Inspected " + inPath.filename().string());
        return McpToolResult::Success(QString(QJsonDocument(out).toJson(QJsonDocument::Indented)).toStdString(), out);
    }

    // Tool: pdf_extract_text
    if (name == "pdf_extract_text") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        int totalPages = doc.GetPageCount();
        int startPage = arguments.contains("startPage") ? std::clamp(arguments["startPage"].toInt(), 1, totalPages) : 1;
        int endPage = arguments.contains("endPage") ? std::clamp(arguments["endPage"].toInt(), startPage, totalPages) : totalPages;

        std::string fullText;
        for (int p = startPage; p <= endPage; ++p) {
            fullText += "--- Page " + std::to_string(p) + " ---\n";
            auto textRes = doc.ExtractPageText(p - 1);
            if (textRes.has_value()) {
                fullText += textRes.value;
            }
            fullText += "\n\n";
        }

        QJsonObject res;
        res["startPage"] = startPage;
        res["endPage"] = endPage;
        res["totalPages"] = totalPages;
        res["characterCount"] = static_cast<int>(fullText.length());

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Extracted text (" + std::to_string(fullText.length()) + " chars)");
        return McpToolResult::Success(fullText, res);
    }

    // Tool: pdf_search
    if (name == "pdf_search") {
        std::string query = arguments["query"].toString().toStdString();
        bool matchCase = arguments["matchCase"].toBool(false);

        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        QJsonArray matchesArr;
        int totalOccurrences = 0;

        for (int p = 0; p < doc.GetPageCount(); ++p) {
            auto hitsRes = doc.SearchPageText(p, query, matchCase, false);
            if (hitsRes.has_value()) {
                for (const auto& hit : hitsRes.value) {
                    totalOccurrences++;
                    QJsonObject matchObj;
                    matchObj["page"] = p + 1;
                    matchObj["charIndex"] = hit.charIndex;
                    matchObj["charCount"] = hit.charCount;
                    if (!hit.rects.empty()) {
                        matchObj["x"] = hit.rects[0].x;
                        matchObj["y"] = hit.rects[0].y;
                        matchObj["width"] = hit.rects[0].width;
                        matchObj["height"] = hit.rects[0].height;
                    }
                    matchesArr.append(matchObj);
                }
            }
        }

        QJsonObject res;
        res["query"] = QString::fromStdString(query);
        res["totalMatches"] = totalOccurrences;
        res["matches"] = matchesArr;

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Searched '" + query + "' (" + std::to_string(totalOccurrences) + " matches)");
        return McpToolResult::Success(QString(QJsonDocument(res).toJson(QJsonDocument::Indented)).toStdString(), res);
    }

    // Tool: pdf_preflight_validate
    if (name == "pdf_preflight_validate") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        services::PreflightOptions preflightOpt;
        auto reportRes = services::PreflightService::Instance().RunPreflight(doc, preflightOpt);
        if (!reportRes.has_value()) {
            return McpToolResult::Error("Preflight run failed: " + reportRes.errorMessage);
        }
        const auto& report = reportRes.value;

        QJsonObject res;
        res["isPassed"] = report.isPassed;
        res["totalErrors"] = report.totalErrors;
        res["totalWarnings"] = report.totalWarnings;
        res["totalInfos"] = report.totalInfos;
        res["summary"] = QString::fromStdString(report.summary);

        QJsonArray issues;
        for (const auto& finding : report.findings) {
            QJsonObject iss;
            iss["title"] = QString::fromStdString(finding.title);
            iss["severity"] = (finding.severity == pdf_api::PreflightSeverity::Error) ? "Error" :
                              (finding.severity == pdf_api::PreflightSeverity::Warning) ? "Warning" : "Info";
            iss["page"] = finding.pageIndex + 1;
            iss["description"] = QString::fromStdString(finding.description);
            iss["remediation"] = QString::fromStdString(finding.remediation);
            issues.append(iss);
        }
        res["findings"] = issues;

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Preflight verified: " + std::string(report.isPassed ? "PASS" : "FAIL"));
        return McpToolResult::Success(QString(QJsonDocument(res).toJson(QJsonDocument::Indented)).toStdString(), res);
    }

    // Tool: pdf_verify_signatures
    if (name == "pdf_verify_signatures") {
        auto valRes = services::SignatureService::ValidateDocumentSignatures(inPath);
        if (!valRes.has_value()) {
            return McpToolResult::Error("Signature validation failed: " + valRes.errorMessage);
        }

        const auto& sigs = valRes.value;
        QJsonArray arr;
        for (const auto& sig : sigs) {
            QJsonObject sObj;
            sObj["fieldName"] = QString::fromStdString(sig.fieldName);
            sObj["signerName"] = QString::fromStdString(sig.certificate.subject);
            sObj["reason"] = QString::fromStdString(sig.reason);
            sObj["signingTime"] = QString::fromStdString(sig.signingTime);
            sObj["hasValidDigest"] = sig.hasValidDigest;
            sObj["isTampered"] = sig.isTampered;
            sObj["isCertValid"] = sig.isCertValid;
            sObj["trustSummary"] = QString::fromStdString(sig.trustSummary);
            arr.append(sObj);
        }

        QJsonObject res;
        res["totalSignatures"] = static_cast<int>(sigs.size());
        res["signatures"] = arr;

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Signatures verified (" + std::to_string(sigs.size()) + ")");
        return McpToolResult::Success(QString(QJsonDocument(res).toJson(QJsonDocument::Indented)).toStdString(), res);
    }

    // Tool: pdf_optimize
    if (name == "pdf_optimize") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        pdf_api::OptimizationOptions opt;
        std::string presetStr = arguments["preset"].toString("Recommended").toStdString();
        if (presetStr == "Screen") opt.preset = pdf_api::CompressionPreset::MaximumScreenWeb;
        else if (presetStr == "Prepress") opt.preset = pdf_api::CompressionPreset::PrepressLossless;
        else opt.preset = pdf_api::CompressionPreset::RecommendedStandard;

        if (arguments.contains("targetDpi")) {
            opt.targetImageDpi = arguments["targetDpi"].toInt(150);
        }

        auto optRes = services::OptimizationService::Instance().OptimizePdf(doc, inPath, outPath, opt);
        if (!optRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", optRes.errorMessage);
            return McpToolResult::Error("Optimization failed: " + optRes.errorMessage);
        }

        QJsonObject res;
        res["originalSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(inPath));
        res["optimizedSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(outPath));
        res["savedBytes"] = res["originalSizeBytes"].toInteger() - res["optimizedSizeBytes"].toInteger();
        res["outputPath"] = QString::fromStdString(outPath.string());

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Optimized to " + outPath.filename().string());
        return McpToolResult::Success("Document optimized successfully: " + outPath.string(), res);
    }

    // Tool: pdf_convert_to_pdfa
    if (name == "pdf_convert_to_pdfa") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        pdf_api::PdfAConversionOptions opt;
        std::string profStr = arguments["profile"].toString("PdfA1b").toStdString();
        opt.targetProfile = (profStr == "PdfA2b") ? pdf_api::PdfAProfile::PdfA2b : pdf_api::PdfAProfile::PdfA1b;
        opt.embedMissingMetadata = true;
        opt.stripProhibitedActions = true;
        opt.injectOutputIntent = true;

        auto convRes = services::PdfAComplianceService::Instance().ConvertToPdfA(doc, outPath, opt);
        if (!convRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", convRes.errorMessage);
            return McpToolResult::Error("PDF/A conversion failed: " + convRes.errorMessage);
        }

        QJsonObject res;
        res["outputPath"] = QString::fromStdString(outPath.string());
        res["profile"] = QString::fromStdString(profStr);
        res["outputSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(outPath));

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Converted to " + profStr);
        return McpToolResult::Success("Document converted to " + profStr + " successfully: " + outPath.string(), res);
    }

    // Tool: pdf_ocr
    if (name == "pdf_ocr") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        pdf_api::OcrOptions opt;
        opt.language = arguments["language"].toString("eng").toStdString();
        opt.targetDpi = arguments["dpi"].toInt(150);

        auto ocrRes = services::OcrService::Instance().ProcessDocumentOcr(doc, outPath, opt);
        if (!ocrRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", ocrRes.errorMessage);
            return McpToolResult::Error("OCR processing failed: " + ocrRes.errorMessage);
        }

        QJsonObject res;
        res["outputPath"] = QString::fromStdString(outPath.string());
        res["language"] = QString::fromStdString(opt.language);
        res["outputSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(outPath));

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "OCR processed to " + outPath.filename().string());
        return McpToolResult::Success("OCR processing completed successfully: " + outPath.string(), res);
    }

    // Tool: pdf_flatten_forms (Destructive)
    if (name == "pdf_flatten_forms") {
        auto openRes = engine->OpenDocument(inPath);
        if (!openRes.has_value()) {
            return McpToolResult::Error("Failed to open document: " + openRes.errorMessage);
        }
        domain::DocumentModel doc(openRes.value, engine, inPath);

        bool forPrint = arguments["forPrint"].toBool(false);
        auto flatRes = doc.FlattenDocument(forPrint);
        if (!flatRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", flatRes.errorMessage);
            return McpToolResult::Error("Form flattening failed: " + flatRes.errorMessage);
        }

        auto saveRes = doc.SaveCopy(outPath);
        if (!saveRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", saveRes.errorMessage);
            return McpToolResult::Error("Failed to save flattened PDF: " + saveRes.errorMessage);
        }

        QJsonObject res;
        res["outputPath"] = QString::fromStdString(outPath.string());
        res["forPrint"] = forPrint;
        res["outputSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(outPath));

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Flattened forms: " + outPath.filename().string());
        return McpToolResult::Success("Forms flattened successfully: " + outPath.string(), res);
    }

    // Tool: pdf_redact_and_sanitize (Destructive)
    if (name == "pdf_redact_and_sanitize") {
        pdf_api::SanitizationOptions opt;
        opt.scrubInfoMetadata = arguments["scrubMetadata"].toBool(true);
        opt.scrubXmpMetadata = arguments["scrubMetadata"].toBool(true);
        opt.removeJavaScript = arguments["removeScripts"].toBool(true);
        opt.removeAttachments = arguments["removeAttachments"].toBool(true);
        opt.removeHiddenLayers = true;

        auto sanRes = services::SanitizationService::ScrubMetadataAndRebuild(inPath, outPath, opt);
        if (!sanRes.has_value()) {
            policy.RecordAuditLog("mcp_client", "tools/call", name, "ERROR", sanRes.errorMessage);
            return McpToolResult::Error("Sanitization failed: " + sanRes.errorMessage);
        }

        QJsonObject res;
        res["outputPath"] = QString::fromStdString(outPath.string());
        res["outputSizeBytes"] = static_cast<qint64>(std::filesystem::file_size(outPath));

        policy.RecordAuditLog("mcp_client", "tools/call", name, "SUCCESS", "Sanitized to " + outPath.filename().string());
        return McpToolResult::Success("Sanitization completed successfully: " + outPath.string(), res);
    }

    return McpToolResult::Error("Unhandled tool: " + name);
}

} // namespace pdfinity::mcp
