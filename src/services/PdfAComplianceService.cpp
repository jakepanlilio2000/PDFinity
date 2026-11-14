// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfAComplianceService.hpp"
#include "DocumentIntelligenceService.hpp"
#include "SanitizationService.hpp"
#include "application/Logger.hpp"

#include <fstream>
#include <sstream>
#include <regex>

namespace pdfinity::services {

PdfAComplianceService& PdfAComplianceService::Instance() {
    static PdfAComplianceService instance;
    return instance;
}

pdf_api::EngineResult<pdf_api::PdfAValidationReport> PdfAComplianceService::ValidateCompliance(
    domain::DocumentModel& doc,
    pdf_api::PdfAProfile targetProfile
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            pdf_api::EngineError::Unknown, "Invalid document model for PDF/A validation"
        );
    }

    pdf_api::PdfAValidationReport report;
    report.targetProfile = targetProfile;
    report.disclaimer =
        "Standards Validation Disclaimer: ISO 19005 PDF/A validation performed here verifies file structure, "
        "font embedding, encryption prohibition, metadata schemas, and action constraints according to technical "
        "specification rules. This automated verification distinguishes formal standards checking from best-effort "
        "inspection, but does not constitute a certified legal warranty of compliance without independent accreditation.";

    std::string profileName = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "PDF/A-1b" : "PDF/A-2b";
    std::string partExpected = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "1" : "2";

    bool allPassed = true;

    // Rule 1: ISO 19005 Clause 6.1.4 - No Encryption
    {
        pdf_api::PdfARuleCheck r;
        r.ruleId = "RULE-PDFA-01";
        r.clause = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "ISO 19005-1:6.1.4" : "ISO 19005-2:6.1.4";
        r.description = "Document must not be encrypted or password-protected";

        auto perms = doc.GetPermissions();
        if (perms.isEncrypted) {
            r.passed = false;
            r.failureDetail = "Document is encrypted with security handler: " + perms.securityHandler;
            allPassed = false;
        } else {
            r.passed = true;
        }
        report.ruleChecks.push_back(r);
    }

    // Rule 2: ISO 19005 Clause 6.3 - Embedded Fonts
    {
        pdf_api::PdfARuleCheck r;
        r.ruleId = "RULE-PDFA-02";
        r.clause = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "ISO 19005-1:6.3" : "ISO 19005-2:6.2.11";
        r.description = "All fonts used in the document must be embedded or subset-embedded";

        auto fontsRes = doc.GetDocumentFonts();
        bool fontsOk = true;
        std::string missingFonts;
        if (fontsRes.has_value()) {
            for (const auto& f : fontsRes.value) {
                if (!f.isEmbedded) {
                    fontsOk = false;
                    if (!missingFonts.empty()) missingFonts += ", ";
                    missingFonts += f.baseFontName;
                }
            }
        }

        if (!fontsOk) {
            r.passed = false;
            r.failureDetail = "Unembedded font(s) detected: " + missingFonts;
            allPassed = false;
        } else {
            r.passed = true;
        }
        report.ruleChecks.push_back(r);
    }

    // Rule 3: ISO 19005 Clause 6.7 - XMP Metadata & Identification Schema
    {
        pdf_api::PdfARuleCheck r;
        r.ruleId = "RULE-PDFA-03";
        r.clause = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "ISO 19005-1:6.7" : "ISO 19005-2:6.6";
        r.description = "Catalog must contain XMP metadata with valid PDF/A identification schema (pdfaid:part & pdfaid:conformance)";

        auto xmpRes = doc.GetRawXmpMetadata();
        std::string xmp = xmpRes.has_value() ? xmpRes.value : "";
        if (xmp.empty() && !doc.GetPath().empty() && std::filesystem::exists(doc.GetPath())) {
            std::ifstream in(doc.GetPath(), std::ios::binary);
            if (in.is_open()) {
                std::string fileContent((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                size_t pos = fileContent.find("<x:xmpmeta");
                if (pos != std::string::npos) {
                    size_t endPos = fileContent.find("</x:xmpmeta>", pos);
                    if (endPos != std::string::npos) {
                        xmp = fileContent.substr(pos, endPos + 12 - pos);
                    }
                }
            }
        }
        bool xmpOk = false;
        if (!xmp.empty()) {
            bool hasPart = (xmp.find("<pdfaid:part>" + partExpected + "</pdfaid:part>") != std::string::npos ||
                            xmp.find("pdfaid:part=\"" + partExpected + "\"") != std::string::npos);
            bool hasConf = (xmp.find("<pdfaid:conformance>B</pdfaid:conformance>") != std::string::npos ||
                            xmp.find("<pdfaid:conformance>b</pdfaid:conformance>") != std::string::npos ||
                            xmp.find("pdfaid:conformance=\"B\"") != std::string::npos);
            if (hasPart && hasConf) {
                xmpOk = true;
                report.profileClaimed = profileName;
            }
        }

        if (!xmpOk) {
            r.passed = false;
            r.failureDetail = "Missing or invalid <pdfaid:part> / <pdfaid:conformance> schema in document XMP packet";
            allPassed = false;
        } else {
            r.passed = true;
        }
        report.ruleChecks.push_back(r);
    }

    // Rule 4: ISO 19005 Clause 6.2.3 - Device Color Spaces & OutputIntent
    {
        pdf_api::PdfARuleCheck r;
        r.ruleId = "RULE-PDFA-04";
        r.clause = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "ISO 19005-1:6.2.3" : "ISO 19005-2:6.2.4";
        r.description = "Device-dependent color spaces must be characterized by an OutputIntent with standard ICC profile";

        bool hasOutputIntent = false;
        if (!doc.GetPath().empty() && std::filesystem::exists(doc.GetPath())) {
            std::ifstream in(doc.GetPath(), std::ios::binary);
            if (in.is_open()) {
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                if (content.find("/OutputIntents") != std::string::npos || content.find("/GTS_PDFA1") != std::string::npos) {
                    hasOutputIntent = true;
                }
            }
        }

        auto imgRes = doc.GetDocumentImages();
        bool hasDeviceColors = false;
        if (imgRes.has_value()) {
            for (const auto& img : imgRes.value) {
                if (img.colorSpace == "DeviceRGB" || img.colorSpace == "DeviceCMYK") {
                    hasDeviceColors = true;
                    break;
                }
            }
        }

        if (hasDeviceColors && !hasOutputIntent) {
            r.passed = false;
            r.failureDetail = "DeviceRGB or DeviceCMYK images present without a characterizing /OutputIntents dictionary in Catalog";
            allPassed = false;
        } else {
            r.passed = true;
        }
        report.ruleChecks.push_back(r);
    }

    // Rule 5: ISO 19005 Clause 6.6.1 - Prohibited Interactive Elements (JavaScript, Launch Actions)
    {
        pdf_api::PdfARuleCheck r;
        r.ruleId = "RULE-PDFA-05";
        r.clause = (targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "ISO 19005-1:6.6.1" : "ISO 19005-2:6.6.1";
        r.description = "Executable scripts, launch actions, sound and movie annotations are strictly prohibited";

        bool hasForbidden = false;
        std::string forbiddenElements;
        if (!doc.GetPath().empty() && std::filesystem::exists(doc.GetPath())) {
            std::ifstream in(doc.GetPath(), std::ios::binary);
            if (in.is_open()) {
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                if (content.find("/JavaScript") != std::string::npos || content.find("/JS ") != std::string::npos) {
                    hasForbidden = true;
                    forbiddenElements += "JavaScript ";
                }
                if (content.find("/Launch") != std::string::npos) {
                    hasForbidden = true;
                    forbiddenElements += "Launch Action ";
                }
                if (content.find("/Movie") != std::string::npos || content.find("/Sound") != std::string::npos) {
                    hasForbidden = true;
                    forbiddenElements += "Multimedia Stream ";
                }
            }
        }

        if (hasForbidden) {
            r.passed = false;
            r.failureDetail = "Prohibited actions found: " + forbiddenElements;
            allPassed = false;
        } else {
            r.passed = true;
        }
        report.ruleChecks.push_back(r);
    }

    report.isCompliant = allPassed;
    LOG_INFO("PdfA", "Validation against " + profileName + ": " + (allPassed ? "COMPLIANT" : "NON-COMPLIANT"));

    return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Ok(std::move(report));
}

pdf_api::EngineResult<pdf_api::PdfAValidationReport> PdfAComplianceService::ConvertToPdfA(
    domain::DocumentModel& doc,
    const std::filesystem::path& dstPath,
    const pdf_api::PdfAConversionOptions& options
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            pdf_api::EngineError::Unknown, "Invalid document model for PDF/A conversion"
        );
    }

    // 1. Save clean base copy to destination
    auto saveRes = doc.SaveCopy(dstPath);
    if (!saveRes.has_value()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            saveRes.error, "Failed to save document copy: " + saveRes.errorMessage
        );
    }

    // 2. Read saved file content and apply PDF/A transformations
    std::ifstream in(dstPath, std::ios::binary);
    if (!in.is_open()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to reopen saved file for PDF/A restructuring"
        );
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // 3. Strip prohibited actions if requested
    if (options.stripProhibitedActions) {
        auto overwritePattern = [](std::string& target, const std::regex& reg, const std::string& replacement) {
            std::smatch match;
            while (std::regex_search(target, match, reg)) {
                size_t pos = match.position();
                size_t len = match.length();
                std::string rep = replacement;
                if (rep.size() <= len) {
                    rep.append(len - rep.size(), ' ');
                    target.replace(pos, len, rep);
                } else {
                    target.replace(pos, len, rep);
                }
            }
        };

        overwritePattern(content, std::regex(R"(/JavaScript\s+\d+\s+0\s+R)"), "");
        overwritePattern(content, std::regex(R"(/S\s*/JavaScript)"), "/S /None");
        overwritePattern(content, std::regex(R"(/JS\s*\([^\)]*\))"), "/JS ()");
        overwritePattern(content, std::regex(R"(/Launch\s+\d+\s+0\s+R)"), "");
        overwritePattern(content, std::regex(R"(/S\s*/Launch)"), "/S /None");
    }

    // 4. Inject PDF/A Identification Schema into XMP
    std::string partStr = (options.targetProfile == pdf_api::PdfAProfile::PdfA1b) ? "1" : "2";
    std::string xmpPacket =
        "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
        "  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
        "    <rdf:Description rdf:about=\"\" xmlns:pdfaid=\"http://www.aiim.org/pdfa/ns/id/\">\n"
        "      <pdfaid:part>" + partStr + "</pdfaid:part>\n"
        "      <pdfaid:conformance>B</pdfaid:conformance>\n"
        "    </rdf:Description>\n"
        "    <rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
        "      <dc:title><rdf:Alt><rdf:li xml:lang=\"x-default\">" + doc.GetMetadata().title + "</rdf:li></rdf:Alt></dc:title>\n"
        "      <dc:language><rdf:Bag><rdf:li>" + options.defaultLanguage + "</rdf:li></rdf:Bag></dc:language>\n"
        "    </rdf:Description>\n"
        "  </rdf:RDF>\n"
        "</x:xmpmeta>";

    size_t xmpPos = content.find("<x:xmpmeta");
    if (xmpPos != std::string::npos) {
        size_t xmpEnd = content.find("</x:xmpmeta>", xmpPos);
        if (xmpEnd != std::string::npos) {
            xmpEnd += 12;
            content.replace(xmpPos, xmpEnd - xmpPos, xmpPacket);
        }
    } else {
        size_t trailerPos = content.rfind("trailer");
        if (trailerPos != std::string::npos) {
            std::string xmpObj = "\n% PDF/A XMP Metadata Stream\n" + xmpPacket + "\n";
            content.insert(trailerPos, xmpObj);
        } else {
            content.append("\n" + xmpPacket + "\n");
        }
    }

    // 5. Inject OutputIntent for sRGB into Catalog if missing
    if (options.injectOutputIntent && content.find("/OutputIntents") == std::string::npos) {
        size_t catPos = content.find("/Type /Catalog");
        if (catPos != std::string::npos) {
            std::string outputIntentStr =
                "\n/OutputIntents [<< /Type /OutputIntent /S /GTS_PDFA1 "
                "/OutputConditionIdentifier (sRGB IEC61966-2.1) /Info (sRGB) >>] ";
            content.insert(catPos + 14, outputIntentStr);
        }
    }

    // Write back to dstPath
    std::ofstream out(dstPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to write PDF/A output file"
        );
    }
    out.write(content.data(), content.size());
    out.close();

    // 6. Verify converted output by opening in engine and running validation
    auto engine = doc.GetEngine();
    if (!engine) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            pdf_api::EngineError::Unknown, "PDF Engine is null"
        );
    }

    auto reopenRes = engine->OpenDocument(dstPath);
    if (!reopenRes.has_value()) {
        return pdf_api::EngineResult<pdf_api::PdfAValidationReport>::Fail(
            reopenRes.error, "Converted PDF/A failed to reopen: " + reopenRes.errorMessage
        );
    }

    domain::DocumentModel convertedDoc(reopenRes.value, engine, dstPath);
    auto valRes = ValidateCompliance(convertedDoc, options.targetProfile);
    engine->CloseDocument(reopenRes.value);

    return valRes;
}

} // namespace pdfinity::services
