// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "security/SecureString.hpp"
#include <string>
#include <filesystem>
#include <vector>

namespace pdfinity::services {

class DocumentEncryptionService {
public:
    // Calculate standard 32-bit permissions bitmask (ISO 32000 Section 7.6.3.2)
    static int32_t ComputePermissionsMask(const pdf_api::DocumentPermissions& perms);

    // Parse standard 32-bit permissions bitmask back into DocumentPermissions
    static pdf_api::DocumentPermissions ParsePermissionsMask(int32_t mask);

    // Apply password protection & encryption to a PDF document
    static pdf_api::EngineResult<void> EncryptDocument(
        const std::filesystem::path& srcPdfPath,
        const std::filesystem::path& dstPdfPath,
        const pdf_api::DocumentEncryptionConfig& config
    );

    // Remove password protection (requires owner password)
    static pdf_api::EngineResult<void> DecryptDocument(
        const std::filesystem::path& srcPdfPath,
        const std::filesystem::path& dstPdfPath,
        const security::SecureString& ownerPassword
    );
};

} // namespace pdfinity::services
