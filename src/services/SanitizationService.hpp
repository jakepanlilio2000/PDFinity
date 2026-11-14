// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <filesystem>
#include <vector>
#include <string>

namespace pdfinity::services {

class SanitizationService {
public:
    static pdf_api::EngineResult<void> SanitizeFile(
        const std::filesystem::path& targetPdfPath,
        const pdf_api::SanitizationOptions& options
    );

    static pdf_api::EngineResult<void> ScrubMetadataAndRebuild(
        const std::filesystem::path& srcPdfPath,
        const std::filesystem::path& dstPdfPath,
        const pdf_api::SanitizationOptions& options
    );
};

} // namespace pdfinity::services
