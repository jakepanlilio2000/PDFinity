// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SanitizationService.hpp"
#include "application/Logger.hpp"

#include <fstream>
#include <regex>
#include <sstream>

namespace pdfinity::services {

pdf_api::EngineResult<void> SanitizationService::SanitizeFile(
    const std::filesystem::path& targetPdfPath,
    const pdf_api::SanitizationOptions& options
) {
    return ScrubMetadataAndRebuild(targetPdfPath, targetPdfPath, options);
}

pdf_api::EngineResult<void> SanitizationService::ScrubMetadataAndRebuild(
    const std::filesystem::path& srcPdfPath,
    const std::filesystem::path& dstPdfPath,
    const pdf_api::SanitizationOptions& options
) {
    if (!std::filesystem::exists(srcPdfPath)) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Source file not found: " + srcPdfPath.string()
        );
    }

    std::ifstream in(srcPdfPath, std::ios::binary);
    if (!in.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not open file: " + srcPdfPath.string()
        );
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (content.empty()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "PDF file is empty");
    }

    // Helper to overwrite regex matches in-place preserving byte length and xref table offsets!
    auto overwriteWithSameLength = [](std::string& str, const std::regex& re, const std::string& prefix) {
        std::vector<std::pair<size_t, size_t>> matches;
        auto words_begin = std::sregex_iterator(str.begin(), str.end(), re);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            matches.push_back({i->position(), i->length()});
        }
        for (const auto& [pos, len] : matches) {
            std::string rep = prefix;
            if (rep.size() < len) {
                rep.append(len - rep.size(), ' ');
            } else if (rep.size() > len) {
                rep = rep.substr(0, len);
            }
            std::memcpy(str.data() + pos, rep.data(), len);
        }
    };

    // 1. Scrub Document Information Dictionary (/Info) in-place
    if (options.scrubInfoMetadata) {
        // Scrub Title
        overwriteWithSameLength(content, std::regex(R"(/Title\s*\([^\)]*\))"), "/Title ()");
        overwriteWithSameLength(content, std::regex(R"(/Title\s*<[0-9a-fA-F]*>)"), "/Title ()");

        // Scrub Author
        overwriteWithSameLength(content, std::regex(R"(/Author\s*\([^\)]*\))"), "/Author ()");
        overwriteWithSameLength(content, std::regex(R"(/Author\s*<[0-9a-fA-F]*>)"), "/Author ()");

        // Scrub Subject
        overwriteWithSameLength(content, std::regex(R"(/Subject\s*\([^\)]*\))"), "/Subject ()");
        overwriteWithSameLength(content, std::regex(R"(/Subject\s*<[0-9a-fA-F]*>)"), "/Subject ()");

        // Scrub Keywords
        overwriteWithSameLength(content, std::regex(R"(/Keywords\s*\([^\)]*\))"), "/Keywords ()");
        overwriteWithSameLength(content, std::regex(R"(/Keywords\s*<[0-9a-fA-F]*>)"), "/Keywords ()");

        // Scrub Creator & Producer
        overwriteWithSameLength(content, std::regex(R"(/Creator\s*\([^\)]*\))"), "/Creator (PDFinity)");
        overwriteWithSameLength(content, std::regex(R"(/Producer\s*\([^\)]*\))"), "/Producer (PDFinity)");

        // Scrub CreationDate & ModDate
        overwriteWithSameLength(content, std::regex(R"(/CreationDate\s*\([^\)]*\))"), "/CreationDate ()");
        overwriteWithSameLength(content, std::regex(R"(/ModDate\s*\([^\)]*\))"), "/ModDate ()");
    }

    // 2. Scrub XMP Metadata Stream from Catalog (/Metadata) in-place
    if (options.scrubXmpMetadata) {
        // Overwrite /Metadata reference with spaces
        overwriteWithSameLength(content, std::regex(R"(/Metadata\s+\d+\s+0\s+R)"), "");

        // Zero out any remaining <x:xmpmeta blocks in streams
        size_t xmpPos = content.find("<x:xmpmeta");
        while (xmpPos != std::string::npos) {
            size_t xmpEnd = content.find("</x:xmpmeta>", xmpPos);
            if (xmpEnd != std::string::npos) {
                xmpEnd += 12; // length of </x:xmpmeta>
                std::fill(content.begin() + xmpPos, content.begin() + xmpEnd, ' ');
            }
            xmpPos = content.find("<x:xmpmeta", xmpPos + 1);
        }
    }

    // 3. Remove Embedded JavaScript in-place
    if (options.removeJavaScript) {
        overwriteWithSameLength(content, std::regex(R"(/JavaScript\s+\d+\s+0\s+R)"), "");
        overwriteWithSameLength(content, std::regex(R"(/S\s*/JavaScript)"), "/S /None");
        overwriteWithSameLength(content, std::regex(R"(/JS\s*\([^\)]*\))"), "/JS ()");
    }

    // 4. Remove Hidden Layers / Optional Content Groups (/OCProperties) in-place
    if (options.removeHiddenLayers) {
        overwriteWithSameLength(content, std::regex(R"(/OCProperties\s+(\d+\s+0\s+R|<<[\s\S]*?>>))"), "");
        overwriteWithSameLength(content, std::regex(R"(/OCGs\s*\[[^\]]*\])"), "");
    }

    // 5. Atomic write to destination
    std::filesystem::path tempPath = dstPdfPath;
    tempPath += ".tmp";

    std::ofstream out(tempPath, std::ios::binary);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to write sanitized temp file: " + tempPath.string()
        );
    }
    out.write(content.data(), content.size());
    out.close();

    std::error_code ec;
    std::filesystem::rename(tempPath, dstPdfPath, ec);
    if (ec) {
        std::filesystem::remove(dstPdfPath, ec);
        std::filesystem::rename(tempPath, dstPdfPath, ec);
        if (ec) {
            std::filesystem::remove(tempPath, ec);
            return pdf_api::EngineResult<void>::Fail(
                pdf_api::EngineError::SaveFailed, "Failed to replace sanitized output file: " + ec.message()
            );
        }
    }

    LOG_INFO("Sanitization", "Document sanitized successfully: " + dstPdfPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::services
