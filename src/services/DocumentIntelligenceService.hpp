// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace pdfinity::services {

class DocumentIntelligenceService {
public:
    static DocumentIntelligenceService& Instance();

    // Generate comprehensive document intelligence report
    pdf_api::EngineResult<pdf_api::DocumentIntelligenceReport> AnalyzeDocument(
        domain::DocumentModel& doc
    );

    // Update document metadata in Info dict and XMP stream
    pdf_api::EngineResult<void> UpdateMetadata(
        domain::DocumentModel& doc,
        const std::filesystem::path& dstPath,
        const pdf_api::DocumentMetadata& metadata,
        const std::string& customXmp = ""
    );

    // Extract an embedded attachment by index to disk
    pdf_api::EngineResult<void> ExtractAttachment(
        domain::DocumentModel& doc,
        int attachmentIndex,
        const std::filesystem::path& destinationPath
    );

    // Embed an external file into the document as an attachment
    pdf_api::EngineResult<void> EmbedAttachment(
        domain::DocumentModel& doc,
        const std::filesystem::path& fileToAttach,
        const std::string& description = ""
    );

private:
    DocumentIntelligenceService() = default;
};

} // namespace pdfinity::services
