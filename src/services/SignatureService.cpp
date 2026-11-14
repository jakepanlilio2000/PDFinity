// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SignatureService.hpp"
#include "security/CryptoCertificateManager.hpp"
#include "application/Logger.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <ctime>
#include <cstring>

namespace pdfinity::services {

namespace {

std::string HexEncode(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> HexDecode(const std::string& hex) {
    std::vector<uint8_t> bytes;
    std::string clean;
    for (char c : hex) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            clean.push_back(c);
        }
    }
    if (clean.size() % 2 != 0) clean.push_back('0');
    bytes.reserve(clean.size() / 2);
    for (size_t i = 0; i < clean.size(); i += 2) {
        std::string byteString = clean.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string FormatPdfDateNow() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmNow{};
#if defined(_WIN32)
    gmtime_s(&tmNow, &tt);
#else
    gmtime_r(&tt, &tmNow);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "D:%Y%m%d%H%M%SZ", &tmNow);
    return std::string(buf);
}

} // namespace

pdf_api::EngineResult<void> SignatureService::ApplyVisualSignature(
    domain::DocumentModel& doc,
    pdf_api::VisualSignatureData visData
) {
    // A standalone visual signature is purely graphical; it does NOT provide cryptographic binding.
    visData.isCryptographicBound = false;
    return doc.ApplyVisualSignature(visData);
}

pdf_api::EngineResult<void> SignatureService::SignDocumentIncremental(
    const std::filesystem::path& srcPdfPath,
    const std::filesystem::path& dstPdfPath,
    const pdf_api::CryptographicSignOptions& options,
    const security::SecureString& password
) {
    std::ifstream in(srcPdfPath, std::ios::binary);
    if (!in.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not open source PDF: " + srcPdfPath.string()
        );
    }
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (fileBytes.empty()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "Source PDF is empty");
    }

    std::string pdfStr(fileBytes.begin(), fileBytes.end());

    // 1. Scan for the highest object number in the document
    int maxObjNum = 0;
    std::regex objRegex(R"((\d+)\s+0\s+obj)");
    auto words_begin = std::sregex_iterator(pdfStr.begin(), pdfStr.end(), objRegex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        int num = std::stoi((*i)[1].str());
        if (num > maxObjNum) maxObjNum = num;
    }
    if (maxObjNum == 0) maxObjNum = 10;

    // 2. Scan for Root Catalog object number and extract catalog dictionary
    int rootObjNum = 1;
    std::string pagesRef = "2 0 R";
    std::regex rootRegex(R"(/Root\s+(\d+)\s+0\s+R)");
    std::smatch rootMatch;
    if (std::regex_search(pdfStr, rootMatch, rootRegex)) {
        rootObjNum = std::stoi(rootMatch[1].str());
    }

    std::string catalogBody;
    std::regex rootObjPattern(std::to_string(rootObjNum) + R"(\s+0\s+obj\s*<<([\s\S]*?)>>\s*endobj)");
    std::smatch rootObjMatch;
    if (std::regex_search(pdfStr, rootObjMatch, rootObjPattern)) {
        catalogBody = rootObjMatch[1].str();
        // Remove existing /AcroForm if present
        std::regex acroPattern(R"(/AcroForm\s+(\d+\s+0\s+R|<<[\s\S]*?>>))");
        catalogBody = std::regex_replace(catalogBody, acroPattern, "");
    } else {
        std::regex pagesRegex(R"(/Pages\s+(\d+\s+0\s+R))");
        std::smatch pagesMatch;
        if (std::regex_search(pdfStr, pagesMatch, pagesRegex)) {
            pagesRef = pagesMatch[1].str();
        }
        catalogBody = "/Type /Catalog /Pages " + pagesRef;
    }

    // 3. Scan for previous startxref
    size_t oldStartXref = 0;
    size_t srefPos = pdfStr.rfind("startxref");
    if (srefPos != std::string::npos) {
        size_t eol = pdfStr.find_first_of("\r\n", srefPos + 9);
        if (eol != std::string::npos) {
            size_t numStart = pdfStr.find_first_not_of("\r\n \t", eol);
            if (numStart != std::string::npos) {
                oldStartXref = std::stoull(pdfStr.substr(numStart));
            }
        }
    }

    // Allocate new objects
    int sigDictObj = maxObjNum + 1;
    int sigAnnotObj = maxObjNum + 2;
    int acroFormObj = maxObjNum + 3;
    int updatedRootObj = maxObjNum + 4;

    std::string sigTime = FormatPdfDateNow();
    std::string reason = options.reason.empty() ? "Document Authenticity & Integrity" : options.reason;
    std::string location = options.location.empty() ? "Offline Secure Desktop" : options.location;

    // 4096 bytes = 8192 hex characters placeholder for PKCS#7 signature
    const size_t HEX_PLACEHOLDER_LEN = 8192;
    std::string hexPlaceholder(HEX_PLACEHOLDER_LEN, '0');

    // Build incremental update string
    std::ostringstream incOss;
    if (fileBytes.back() != '\n' && fileBytes.back() != '\r') {
        incOss << "\r\n";
    }

    size_t incStartOffset = fileBytes.size();

    auto getCurOffset = [&]() -> size_t {
        return fileBytes.size() + incOss.str().size();
    };

    // Object 1: Signature Value Dictionary
    size_t sigDictOffset = getCurOffset();
    incOss << sigDictObj << " 0 obj\r\n"
           << "<<\r\n"
           << "/Type /Sig\r\n"
           << "/Filter /Adobe.PPKLite\r\n"
           << "/SubFilter /adbe.pkcs7.detached\r\n"
           << "/ByteRange [ 0000000000 0000000000 0000000000 0000000000 ]\r\n"
           << "/Contents <" << hexPlaceholder << ">\r\n"
           << "/Reason (" << reason << ")\r\n"
           << "/M (" << sigTime << ")\r\n"
           << "/Location (" << location << ")\r\n";
    if (!options.contactInfo.empty()) {
        incOss << "/ContactInfo (" << options.contactInfo << ")\r\n";
    }
    incOss << ">>\r\nendobj\r\n";

    // Object 2: Signature Field Widget Annotation
    size_t sigAnnotOffset = getCurOffset();
    incOss << sigAnnotObj << " 0 obj\r\n"
           << "<<\r\n"
           << "/Type /Annot\r\n"
           << "/Subtype /Widget\r\n"
           << "/FT /Sig\r\n"
           << "/Rect [ 0 0 0 0 ]\r\n"
           << "/V " << sigDictObj << " 0 R\r\n"
           << "/T (Signature1)\r\n"
           << "/F 132\r\n"
           << ">>\r\nendobj\r\n";

    // Object 3: AcroForm Dictionary linking Sig field
    size_t acroFormOffset = getCurOffset();
    incOss << acroFormObj << " 0 obj\r\n"
           << "<<\r\n"
           << "/Fields [" << sigAnnotObj << " 0 R]\r\n"
           << "/SigFlags 3\r\n"
           << ">>\r\nendobj\r\n";

    // Object 4: Updated Root Catalog referencing AcroForm and preserving existing catalog entries
    size_t updatedRootOffset = getCurOffset();
    incOss << updatedRootObj << " 0 obj\r\n"
           << "<<"
           << catalogBody
           << "\r\n/AcroForm " << acroFormObj << " 0 R\r\n"
           << ">>\r\nendobj\r\n";

    // Incremental XRef Table
    size_t newXrefOffset = getCurOffset();
    incOss << "xref\r\n"
           << sigDictObj << " 4\r\n";

    auto writeXrefEntry = [&incOss](size_t offset) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%010zu 00000 n \r\n", offset);
        incOss << buf;
    };

    writeXrefEntry(sigDictOffset);
    writeXrefEntry(sigAnnotOffset);
    writeXrefEntry(acroFormOffset);
    writeXrefEntry(updatedRootOffset);

    // Trailer
    incOss << "trailer\r\n"
           << "<<\r\n"
           << "/Size " << (updatedRootObj + 1) << "\r\n"
           << "/Prev " << oldStartXref << "\r\n"
           << "/Root " << updatedRootObj << " 0 R\r\n"
           << ">>\r\n"
           << "startxref\r\n"
           << newXrefOffset << "\r\n"
           << "%%EOF\r\n";

    std::string incStr = incOss.str();
    std::vector<uint8_t> completeDoc = fileBytes;
    completeDoc.insert(completeDoc.end(), incStr.begin(), incStr.end());

    // Locate the exact offsets of Contents `<` and `>` in completeDoc
    std::string completeStr(completeDoc.begin(), completeDoc.end());
    size_t contentsTag = completeStr.find("/Contents <", incStartOffset);
    if (contentsTag == std::string::npos) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::RenderFailed, "Contents tag offset not found");
    }
    size_t contentsStart = contentsTag + 10; // points to '<'
    size_t contentsEnd = contentsStart + 1 + HEX_PLACEHOLDER_LEN; // points to '>'

    size_t totalLen = completeDoc.size();
    size_t len1 = contentsStart;
    size_t start2 = contentsEnd + 1;
    size_t len2 = totalLen - start2;

    // Format fixed-width ByteRange:
    char byteRangeExact[128];
    snprintf(byteRangeExact, sizeof(byteRangeExact), "/ByteRange [ %010zu %010zu %010zu %010zu ]",
             static_cast<size_t>(0), len1, start2, len2);

    size_t byteRangeTag = completeStr.find("/ByteRange [ ", incStartOffset);
    if (byteRangeTag == std::string::npos) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::RenderFailed, "ByteRange placeholder not found");
    }
    std::memcpy(completeDoc.data() + byteRangeTag, byteRangeExact, std::strlen(byteRangeExact));

    // Extract exactly part 1 and part 2 for signing
    std::vector<uint8_t> dataToSign;
    dataToSign.reserve(len1 + len2);
    dataToSign.insert(dataToSign.end(), completeDoc.begin(), completeDoc.begin() + len1);
    dataToSign.insert(dataToSign.end(), completeDoc.begin() + start2, completeDoc.begin() + start2 + len2);

    // Call CryptoCertificateManager to generate detached PKCS#7 signature
    auto sigRes = security::CryptoCertificateManager::Instance().CreateDetachedSignature(
        options.certThumbprint, password, dataToSign
    );
    if (!sigRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(sigRes.error, sigRes.errorMessage);
    }

    std::string pkcs7Hex = HexEncode(sigRes.value.data(), sigRes.value.size());
    if (pkcs7Hex.size() > HEX_PLACEHOLDER_LEN) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::RenderFailed, "PKCS#7 signature exceeds placeholder buffer size"
        );
    }

    // Overwrite `<...>` with actual hex signature and pad with '0'
    size_t hexWritePos = contentsStart + 1;
    std::memcpy(completeDoc.data() + hexWritePos, pkcs7Hex.data(), pkcs7Hex.size());
    size_t padLen = HEX_PLACEHOLDER_LEN - pkcs7Hex.size();
    if (padLen > 0) {
        std::memset(completeDoc.data() + hexWritePos + pkcs7Hex.size(), '0', padLen);
    }

    // Write final signed file atomically
    std::filesystem::path tempDst = dstPdfPath;
    tempDst += ".tmp";
    std::ofstream out(tempDst, std::ios::binary);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not write to destination: " + dstPdfPath.string()
        );
    }
    out.write(reinterpret_cast<const char*>(completeDoc.data()), completeDoc.size());
    out.close();

    std::error_code ec;
    std::filesystem::rename(tempDst, dstPdfPath, ec);
    if (ec) {
        std::filesystem::remove(tempDst, ec);
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::Unknown, "Failed to finalize signed document: " + ec.message()
        );
    }

    LOG_INFO("Signature", "Cryptographic PDF signature appended successfully via incremental update: " + dstPdfPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<std::vector<pdf_api::SignatureValidationResult>> SignatureService::ValidateDocumentSignatures(
    const std::filesystem::path& pdfPath
) {
    std::ifstream file(pdfPath, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<std::vector<pdf_api::SignatureValidationResult>>::Fail(
            pdf_api::EngineError::FileNotFound, "File not found: " + pdfPath.string()
        );
    }
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (fileBytes.empty()) {
        return pdf_api::EngineResult<std::vector<pdf_api::SignatureValidationResult>>::Fail(
            pdf_api::EngineError::InvalidFormat, "PDF file is empty"
        );
    }

    std::string pdfStr(fileBytes.begin(), fileBytes.end());
    std::vector<pdf_api::SignatureValidationResult> results;

    // Search for all /ByteRange [...] occurrences
    std::regex brRegex(R"(/ByteRange\s*\[\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*\])");
    auto words_begin = std::sregex_iterator(pdfStr.begin(), pdfStr.end(), brRegex);
    auto words_end = std::sregex_iterator();

    int sigIndex = 0;
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        size_t b1 = std::stoull(match[1].str());
        size_t l1 = std::stoull(match[2].str());
        size_t b2 = std::stoull(match[3].str());
        size_t l2 = std::stoull(match[4].str());

        pdf_api::SignatureValidationResult res;
        res.signatureIndex = sigIndex++;
        res.byteRange = {static_cast<int>(b1), static_cast<int>(l1), static_cast<int>(b2), static_cast<int>(l2)};

        // Validate byte range boundaries
        if (b1 != 0 || (b1 + l1) > fileBytes.size() || b2 > fileBytes.size() || (b2 + l2) > fileBytes.size()) {
            res.hasValidDigest = false;
            res.isTampered = true;
            res.trustSummary = "TAMPERED: ByteRange points outside file boundaries.";
            results.push_back(std::move(res));
            continue;
        }

        // Post-signing modification check: any bytes beyond signed range?
        if (fileBytes.size() > (b2 + l2)) {
            res.hasModificationsAfterSigning = true;
        }

        // Locate /Contents <...> between l1 and b2
        std::string gapStr = pdfStr.substr(l1, b2 - l1);
        size_t cOpen = gapStr.find('<');
        size_t cClose = gapStr.rfind('>');
        if (cOpen == std::string::npos || cClose == std::string::npos || cClose <= cOpen) {
            res.hasValidDigest = false;
            res.isTampered = true;
            res.trustSummary = "TAMPERED: Signature /Contents hex stream is corrupted or missing.";
            results.push_back(std::move(res));
            continue;
        }

        std::string hexSig = gapStr.substr(cOpen + 1, cClose - cOpen - 1);
        std::vector<uint8_t> pkcs7Der = HexDecode(hexSig);

        // Reconstruct signed data
        std::vector<uint8_t> signedData;
        signedData.reserve(l1 + l2);
        signedData.insert(signedData.end(), fileBytes.begin() + b1, fileBytes.begin() + b1 + l1);
        signedData.insert(signedData.end(), fileBytes.begin() + b2, fileBytes.begin() + b2 + l2);

        // Verify cryptographic detached signature
        auto verifyRes = security::CryptoCertificateManager::Instance().VerifyDetachedSignature(
            signedData, pkcs7Der
        );

        if (verifyRes.has_value()) {
            res.hasValidDigest = verifyRes.value.hasValidDigest;
            res.isTampered = verifyRes.value.isTampered;
            res.certificate = verifyRes.value.certificate;
            res.isCertValid = verifyRes.value.isCertValid;
            res.isChainTrusted = verifyRes.value.isChainTrusted;
            res.revocationStatus = verifyRes.value.revocationStatus;
            res.trustSummary = verifyRes.value.trustSummary;
        } else {
            res.hasValidDigest = false;
            res.isTampered = true;
            res.trustSummary = "VERIFICATION FAILED: " + verifyRes.errorMessage;
        }

        // Parse optional reason and signing time
        size_t dictStart = pdfStr.rfind("<<", match.position());
        size_t dictEnd = pdfStr.find(">>", match.position());
        if (dictStart != std::string::npos && dictEnd != std::string::npos) {
            std::string dictStr = pdfStr.substr(dictStart, dictEnd - dictStart);
            std::regex rRegex(R"(/Reason\s*\(([^)]*)\))");
            std::smatch rMatch;
            if (std::regex_search(dictStr, rMatch, rRegex)) {
                res.reason = rMatch[1].str();
            }
            std::regex mRegex(R"(/M\s*\(([^)]*)\))");
            std::smatch mMatch;
            if (std::regex_search(dictStr, mMatch, mRegex)) {
                res.signingTime = mMatch[1].str();
            }
            std::regex locRegex(R"(/Location\s*\(([^)]*)\))");
            std::smatch locMatch;
            if (std::regex_search(dictStr, locMatch, locRegex)) {
                res.location = locMatch[1].str();
            }
        }

        results.push_back(std::move(res));
    }

    return pdf_api::EngineResult<std::vector<pdf_api::SignatureValidationResult>>::Ok(std::move(results));
}

std::string SignatureService::GenerateSecurityReport(
    const std::filesystem::path& pdfPath,
    const std::vector<pdf_api::SignatureValidationResult>& results
) {
    std::ostringstream oss;
    oss << "# PDFinity Security and Digital Signature Audit Report\n\n"
        << "- **Document Path:** `" << pdfPath.string() << "`\n"
        << "- **Total Signatures Detected:** " << results.size() << "\n"
        << "- **Inspection Timestamp:** " << FormatPdfDateNow() << "\n\n"
        << "---\n\n";

    if (results.empty()) {
        oss << "### No Cryptographic Signatures Detected\n\n"
            << "This document contains no digital signature fields (`/Type /Sig`). Visual signatures or stamps present on pages are graphical appearances only and provide no cryptographic guarantee of document integrity.\n\n";
    } else {
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& sig = results[i];
            oss << "### Signature #" << (i + 1) << "\n\n"
                << "| Security Parameter | Audit Result |\n"
                << "| :--- | :--- |\n"
                << "| **Mathematical Integrity** | " << (sig.hasValidDigest ? "✅ **VALID (Digest Match)**" : "❌ **INVALID / TAMPERED**") << " |\n"
                << "| **Tamper Detection Status** | " << (sig.isTampered ? "⚠️ **DOCUMENT MODIFIED AFTER SIGNING**" : "✅ Intact (No modifications)") << " |\n"
                << "| **Post-Sign Changes** | " << (sig.hasModificationsAfterSigning ? "Incremental additions appended after signature byte range" : "None (Signature covers through EOF)") << " |\n"
                << "| **Signer Identity (Subject)** | " << (sig.certificate.subject.empty() ? "Unknown" : sig.certificate.subject) << " |\n"
                << "| **Certificate Issuer** | " << (sig.certificate.issuer.empty() ? "Unknown" : sig.certificate.issuer) << " |\n"
                << "| **Certificate Serial** | `" << sig.certificate.serialNumber << "` |\n"
                << "| **Validity Period** | " << sig.certificate.validFrom << " to " << sig.certificate.validTo << " |\n"
                << "| **Certificate Expiry** | " << (sig.certificate.isExpired ? "⚠️ Expired" : "Valid") << " |\n"
                << "| **SHA-256 Thumbprint** | `" << sig.certificate.thumbprintSha256 << "` |\n"
                << "| **Filter / SubFilter** | `" << sig.filter << "` / `" << sig.subFilter << "` |\n"
                << "| **Byte Range** | `[" << sig.byteRange[0] << ", " << sig.byteRange[1] << ", " << sig.byteRange[2] << ", " << sig.byteRange[3] << "]` |\n"
                << "| **Signing Reason** | " << (sig.reason.empty() ? "None specified" : sig.reason) << " |\n"
                << "| **Signing Time** | " << (sig.signingTime.empty() ? "Unknown" : sig.signingTime) << " |\n"
                << "| **Revocation Status** | " << sig.revocationStatus << " |\n\n";

            oss << "**Trust Assessment:** " << sig.trustSummary << "\n\n";
        }
    }

    oss << "### Trust & Offline Limitation Disclosure\n\n"
        << GetTrustLimitationDisclosure() << "\n";

    return oss.str();
}

std::string SignatureService::GetTrustLimitationDisclosure() {
    return "PDFinity operates offline-first. Signature validation mathematically verifies document byte integrity against the embedded X.509 certificate. However, because real-time OCSP/CRL revocation services require outbound network access, certificates that were revoked after signing cannot be identified without an active network connection and configured trust anchors.";
}

} // namespace pdfinity::services
