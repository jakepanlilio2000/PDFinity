// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include "pdf_api/PdfTypes.hpp"
#include "security/SecureString.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace pdfinity::services {

class SignatureService {
public:
    // Apply visual-only signature appearance (stamped graphical appearance, no cryptographic binding)
    static pdf_api::EngineResult<void> ApplyVisualSignature(
        domain::DocumentModel& doc,
        pdf_api::VisualSignatureData visData
    );

    // Cryptographically sign PDF using ISO 32000 incremental update and PKCS#7 detached signature
    static pdf_api::EngineResult<void> SignDocumentIncremental(
        const std::filesystem::path& srcPdfPath,
        const std::filesystem::path& dstPdfPath,
        const pdf_api::CryptographicSignOptions& options,
        const security::SecureString& password
    );

    // Validate all cryptographic digital signatures in a PDF file
    static pdf_api::EngineResult<std::vector<pdf_api::SignatureValidationResult>> ValidateDocumentSignatures(
        const std::filesystem::path& pdfPath
    );

    // Generate human-readable signature & security report (Markdown / Text)
    static std::string GenerateSecurityReport(
        const std::filesystem::path& pdfPath,
        const std::vector<pdf_api::SignatureValidationResult>& results
    );

    // Disclose validation trust limitations (offline / OCSP / CRL)
    static std::string GetTrustLimitationDisclosure();
};

} // namespace pdfinity::services
