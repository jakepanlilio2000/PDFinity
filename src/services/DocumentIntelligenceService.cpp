// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentIntelligenceService.hpp"
#include "application/Logger.hpp"

#include <fstream>
#include <sstream>
#include <regex>

namespace pdfinity::services {

DocumentIntelligenceService& DocumentIntelligenceService::Instance() {
    static DocumentIntelligenceService instance;
    return instance;
}

pdf_api::EngineResult<pdf_api::DocumentIntelligenceReport> DocumentIntelligenceService::AnalyzeDocument(
    domain::DocumentModel& doc
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<pdf_api::DocumentIntelligenceReport>::Fail(
            pdf_api::EngineError::Unknown, "Invalid document model"
        );
    }

    pdf_api::DocumentIntelligenceReport report;
    report.metadata = doc.GetMetadata();

    // Raw XMP
    auto xmpRes = doc.GetRawXmpMetadata();
    if (xmpRes.has_value()) {
        report.rawXmpPacket = xmpRes.value;
    }

    // Fonts
    auto fontsRes = doc.GetDocumentFonts();
    if (fontsRes.has_value()) {
        report.fonts = fontsRes.value;
    }

    // Images
    auto imgRes = doc.GetDocumentImages();
    if (imgRes.has_value()) {
        report.images = imgRes.value;
    }

    // Page Boxes
    auto boxRes = doc.GetPageBoxes();
    if (boxRes.has_value()) {
        report.pageBoxes = boxRes.value;
    }

    // Attachments
    auto attRes = doc.GetDocumentAttachments();
    if (attRes.has_value()) {
        report.attachments = attRes.value;
    }

    // Bookmarks
    auto bmRes = doc.GetBookmarks();
    if (bmRes.has_value()) {
        report.bookmarks = bmRes.value;
    }

    // Tagged PDF
    auto tagRes = doc.IsDocumentTagged();
    report.isTaggedPdf = tagRes.has_value() && tagRes.value;

    // Detect Layers (OCGs) & Language from file content if path exists
    if (!doc.GetPath().empty() && std::filesystem::exists(doc.GetPath())) {
        std::ifstream in(doc.GetPath(), std::ios::binary);
        if (in.is_open()) {
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();

            if (content.find("/OCProperties") != std::string::npos || content.find("/OCGs") != std::string::npos) {
                report.hasLayers = true;
                // Parse layer names if present: /Name (Layer 1)
                std::regex nameRegex(R"(/OCG[\s\S]*?/Name\s*\(([^)]+)\))");
                std::smatch match;
                std::string::const_iterator searchStart(content.cbegin());
                while (std::regex_search(searchStart, content.cend(), match, nameRegex)) {
                    report.layerNames.push_back(match[1].str());
                    searchStart = match.suffix().first;
                }
                if (report.layerNames.empty()) {
                    report.layerNames.push_back("Default Layer");
                }
            }

            // Document language: /Lang (en-US)
            std::regex langRegex(R"(/Lang\s*\(([^)]+)\))");
            std::smatch lMatch;
            if (std::regex_search(content, lMatch, langRegex)) {
                report.language = lMatch[1].str();
            }
        }
    }

    if (report.language.empty()) {
        report.language = "Unspecified";
    }

    LOG_INFO("Intelligence", "Document analyzed: " + std::to_string(report.fonts.size()) + " font(s), " +
             std::to_string(report.images.size()) + " image(s), " +
             std::to_string(report.attachments.size()) + " attachment(s)");

    return pdf_api::EngineResult<pdf_api::DocumentIntelligenceReport>::Ok(std::move(report));
}

pdf_api::EngineResult<void> DocumentIntelligenceService::UpdateMetadata(
    domain::DocumentModel& doc,
    const std::filesystem::path& dstPath,
    const pdf_api::DocumentMetadata& metadata,
    const std::string& customXmp
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document model");
    }

    // Save copy first
    auto saveRes = doc.SaveCopy(dstPath);
    if (!saveRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(saveRes.error, "Failed to save document copy: " + saveRes.errorMessage);
    }

    // Read saved copy and update Info dictionary & XMP stream in-place
    std::ifstream in(dstPath, std::ios::binary);
    if (!in.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to reopen saved file for metadata injection");
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    auto updateOrAppendField = [&](const std::string& key, const std::string& val) {
        std::regex fieldRegex("/" + key + R"(\s*\([^\)]*\))");
        std::string replacement = "/" + key + " (" + val + ")";
        if (std::regex_search(content, fieldRegex)) {
            // Replace existing
            std::smatch m;
            if (std::regex_search(content, m, fieldRegex)) {
                size_t pos = m.position();
                size_t len = m.length();
                if (replacement.size() <= len) {
                    replacement.append(len - replacement.size(), ' ');
                    content.replace(pos, len, replacement);
                } else {
                    content.replace(pos, len, replacement);
                }
            }
        }
    };

    if (!metadata.title.empty()) updateOrAppendField("Title", metadata.title);
    if (!metadata.author.empty()) updateOrAppendField("Author", metadata.author);
    if (!metadata.subject.empty()) updateOrAppendField("Subject", metadata.subject);
    if (!metadata.keywords.empty()) updateOrAppendField("Keywords", metadata.keywords);
    if (!metadata.creator.empty()) updateOrAppendField("Creator", metadata.creator);
    if (!metadata.producer.empty()) updateOrAppendField("Producer", metadata.producer);

    // Update XMP stream if custom XMP provided or rebuild standard Dublin Core block
    std::string xmpToInject = customXmp;
    if (xmpToInject.empty()) {
        std::ostringstream ss;
        ss << "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">\n"
           << "  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
           << "    <rdf:Description rdf:about=\"\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
           << "      <dc:title><rdf:Alt><rdf:li xml:lang=\"x-default\">" << metadata.title << "</rdf:li></rdf:Alt></dc:title>\n"
           << "      <dc:creator><rdf:Seq><rdf:li>" << metadata.author << "</rdf:li></rdf:Seq></dc:creator>\n"
           << "      <dc:description><rdf:Alt><rdf:li xml:lang=\"x-default\">" << metadata.subject << "</rdf:li></rdf:Alt></dc:description>\n"
           << "    </rdf:Description>\n"
           << "  </rdf:RDF>\n"
           << "</x:xmpmeta>";
        xmpToInject = ss.str();
    }

    size_t xmpPos = content.find("<x:xmpmeta");
    if (xmpPos != std::string::npos) {
        size_t xmpEnd = content.find("</x:xmpmeta>", xmpPos);
        if (xmpEnd != std::string::npos) {
            xmpEnd += 12;
            content.replace(xmpPos, xmpEnd - xmpPos, xmpToInject);
        }
    } else {
        size_t trailerPos = content.rfind("trailer");
        if (trailerPos != std::string::npos) {
            std::string xmpObj = "\n% XMP Metadata Stream\n" + xmpToInject + "\n";
            content.insert(trailerPos, xmpObj);
        } else {
            content.append("\n" + xmpToInject + "\n");
        }
    }

    std::ofstream out(dstPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to write updated metadata to file");
    }
    out.write(content.data(), content.size());
    out.close();

    LOG_INFO("Intelligence", "Document metadata updated: " + dstPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> DocumentIntelligenceService::ExtractAttachment(
    domain::DocumentModel& doc,
    int attachmentIndex,
    const std::filesystem::path& destinationPath
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document model");
    }

    auto dataRes = doc.ExtractAttachmentData(attachmentIndex);
    if (!dataRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(dataRes.error, "Failed to extract attachment: " + dataRes.errorMessage);
    }

    std::ofstream out(destinationPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to create output file: " + destinationPath.string());
    }

    out.write(reinterpret_cast<const char*>(dataRes.value.data()), dataRes.value.size());
    out.close();

    LOG_INFO("Intelligence", "Attachment extracted: " + destinationPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> DocumentIntelligenceService::EmbedAttachment(
    domain::DocumentModel& doc,
    const std::filesystem::path& fileToAttach,
    const std::string& description
) {
    if (!doc.IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document model");
    }

    if (!std::filesystem::exists(fileToAttach)) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Attachment source file not found: " + fileToAttach.string());
    }

    std::ifstream in(fileToAttach, std::ios::binary);
    if (!in.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Failed to open attachment source file");
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::string name = fileToAttach.filename().string();
    auto addRes = doc.AddAttachment(name, description, data);
    if (!addRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(addRes.error, "Failed to embed attachment: " + addRes.errorMessage);
    }

    LOG_INFO("Intelligence", "Embedded attachment '" + name + "' into document");
    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::services
