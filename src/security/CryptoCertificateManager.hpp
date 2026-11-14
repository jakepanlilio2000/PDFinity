// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "security/SecureString.hpp"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <mutex>

namespace pdfinity::security {

class CryptoCertificateManager {
public:
    static CryptoCertificateManager& Instance();

    // Import PKCS#12 (.p12 / .pfx) certificate with secure password
    pdf_api::EngineResult<pdf_api::CertificateInfo> ImportPkcs12(
        const std::filesystem::path& pfxPath,
        const SecureString& password,
        const std::string& friendlyName = ""
    );

    // List all imported certificates in the secure vault
    std::vector<pdf_api::CertificateInfo> GetInstalledCertificates();

    // Retrieve certificate info by SHA-256 thumbprint
    pdf_api::EngineResult<pdf_api::CertificateInfo> GetCertificateByThumbprint(
        const std::string& thumbprint
    );

    // Delete certificate from secure vault
    bool DeleteCertificate(const std::string& thumbprint);

    // Sign data producing a DER-encoded detached PKCS#7 signature (adbe.pkcs7.detached)
    pdf_api::EngineResult<std::vector<uint8_t>> CreateDetachedSignature(
        const std::string& thumbprint,
        const SecureString& password,
        const std::vector<uint8_t>& dataToSign
    );

    // Verify a detached PKCS#7 signature against raw signed data
    pdf_api::EngineResult<pdf_api::SignatureValidationResult> VerifyDetachedSignature(
        const std::vector<uint8_t>& signedData,
        const std::vector<uint8_t>& pkcs7Der
    );

    // Secure DPAPI Vault storage paths
    [[nodiscard]] std::filesystem::path GetVaultDirectory() const;

private:
    CryptoCertificateManager();
    ~CryptoCertificateManager();

    // Encrypt & decrypt private key/PFX bytes using OS-appropriate Windows DPAPI
    std::vector<uint8_t> ProtectData(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> UnprotectData(const std::vector<uint8_t>& ciphertext);

    std::mutex m_mutex;
    std::filesystem::path m_vaultDir;
};

} // namespace pdfinity::security
