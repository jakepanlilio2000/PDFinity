// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentEncryptionService.hpp"
#include "application/Logger.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#endif

#include <fstream>
#include <sstream>
#include <iomanip>
#include <regex>

namespace pdfinity::services {

namespace {

std::string HexEncode(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::vector<uint8_t> GenerateRandomBytes(size_t count) {
    std::vector<uint8_t> bytes(count, 0);
#if defined(_WIN32)
    HCRYPTPROV hProv = 0;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, static_cast<DWORD>(count), bytes.data());
        CryptReleaseContext(hProv, 0);
    }
#else
    for (size_t i = 0; i < count; ++i) bytes[i] = static_cast<uint8_t>(rand() % 256);
#endif
    return bytes;
}

} // namespace

int32_t DocumentEncryptionService::ComputePermissionsMask(const pdf_api::DocumentPermissions& perms) {
    // ISO 32000-1 Section 7.6.3.2 Table 22
    // Bits 7, 8, 13-32 must be 1.
    uint32_t mask = 0xFFFFF0C0u;

    if (perms.canPrint) mask |= (1u << 2);              // Bit 3
    if (perms.canModifyContents) mask |= (1u << 3);     // Bit 4
    if (perms.canCopyText) mask |= (1u << 4);           // Bit 5
    if (perms.canAnnotate) mask |= (1u << 5);           // Bit 6
    if (perms.canFillForms) mask |= (1u << 8);          // Bit 9
    if (perms.canExtractAccessibility) mask |= (1u << 9);// Bit 10
    if (perms.canAssemble) mask |= (1u << 10);          // Bit 11
    if (perms.canPrintHighQuality) mask |= (1u << 11);  // Bit 12

    return static_cast<int32_t>(mask);
}

pdf_api::DocumentPermissions DocumentEncryptionService::ParsePermissionsMask(int32_t mask) {
    pdf_api::DocumentPermissions perms;
    uint32_t uMask = static_cast<uint32_t>(mask);
    perms.isEncrypted = true;
    perms.canPrint = (uMask & (1u << 2)) != 0;
    perms.canModifyContents = (uMask & (1u << 3)) != 0;
    perms.canCopyText = (uMask & (1u << 4)) != 0;
    perms.canAnnotate = (uMask & (1u << 5)) != 0;
    perms.canFillForms = (uMask & (1u << 8)) != 0;
    perms.canExtractAccessibility = (uMask & (1u << 9)) != 0;
    perms.canAssemble = (uMask & (1u << 10)) != 0;
    perms.canPrintHighQuality = (uMask & (1u << 11)) != 0;
    return perms;
}

pdf_api::EngineResult<void> DocumentEncryptionService::EncryptDocument(
    const std::filesystem::path& srcPdfPath,
    const std::filesystem::path& dstPdfPath,
    const pdf_api::DocumentEncryptionConfig& config
) {
    std::ifstream file(srcPdfPath, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not open source PDF: " + srcPdfPath.string()
        );
    }
    std::vector<uint8_t> fileBytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (fileBytes.empty()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "Source PDF is empty");
    }

    std::string pdfStr(fileBytes.begin(), fileBytes.end());

    // 1. Scan for highest object number
    int maxObjNum = 0;
    std::regex objRegex(R"((\d+)\s+0\s+obj)");
    auto words_begin = std::sregex_iterator(pdfStr.begin(), pdfStr.end(), objRegex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        int num = std::stoi((*i)[1].str());
        if (num > maxObjNum) maxObjNum = num;
    }
    if (maxObjNum == 0) maxObjNum = 10;

    // 2. Scan for Root object number
    int rootObjNum = 1;
    std::regex rootRegex(R"(/Root\s+(\d+)\s+0\s+R)");
    std::smatch rootMatch;
    if (std::regex_search(pdfStr, rootMatch, rootRegex)) {
        rootObjNum = std::stoi(rootMatch[1].str());
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

    int encryptObjNum = maxObjNum + 1;
    int32_t permsMask = ComputePermissionsMask(config.permissions);

    // Cryptographic salts
    auto oSalt = GenerateRandomBytes(32);
    auto uSalt = GenerateRandomBytes(32);
    std::string oHex = HexEncode(oSalt.data(), oSalt.size());
    std::string uHex = HexEncode(uSalt.data(), uSalt.size());

    std::ostringstream oss;
    if (fileBytes.back() != '\n' && fileBytes.back() != '\r') {
        oss << "\r\n";
    }

    size_t incStartOffset = fileBytes.size() + oss.str().size();

    // Write Encrypt dictionary object
    size_t encObjOffset = incStartOffset;
    oss << encryptObjNum << " 0 obj\r\n"
        << "<<\r\n"
        << "/Filter /Standard\r\n"
        << "/V 5\r\n"
        << "/R 6\r\n"
        << "/Length 256\r\n"
        << "/CF << /StdCF << /CFM /AESV3 /Length 32 /AuthEvent /DocOpen >> >>\r\n"
        << "/StmF /StdCF\r\n"
        << "/StrF /StdCF\r\n"
        << "/P " << permsMask << "\r\n"
        << "/O <" << oHex << ">\r\n"
        << "/U <" << uHex << ">\r\n"
        << ">>\r\nendobj\r\n";

    // Write incremental xref
    size_t newXrefOffset = incStartOffset + oss.str().size() - (encObjOffset - incStartOffset);
    oss << "xref\r\n"
        << encryptObjNum << " 1\r\n";

    char buf[32];
    snprintf(buf, sizeof(buf), "%010zu 00000 n \r\n", encObjOffset);
    oss << buf;

    // Trailer
    oss << "trailer\r\n"
        << "<<\r\n"
        << "/Size " << (encryptObjNum + 1) << "\r\n"
        << "/Prev " << oldStartXref << "\r\n"
        << "/Root " << rootObjNum << " 0 R\r\n"
        << "/Encrypt " << encryptObjNum << " 0 R\r\n"
        << ">>\r\n"
        << "startxref\r\n"
        << newXrefOffset << "\r\n"
        << "%%EOF\r\n";

    std::string incStr = oss.str();
    std::vector<uint8_t> completeDoc = fileBytes;
    completeDoc.insert(completeDoc.end(), incStr.begin(), incStr.end());

    std::filesystem::path tempDst = dstPdfPath;
    tempDst += ".tmp";
    std::ofstream out(tempDst, std::ios::binary);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not open destination file: " + dstPdfPath.string()
        );
    }
    out.write(reinterpret_cast<const char*>(completeDoc.data()), completeDoc.size());
    out.close();

    std::error_code ec;
    std::filesystem::rename(tempDst, dstPdfPath, ec);
    if (ec) {
        std::filesystem::remove(tempDst, ec);
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::Unknown, "Failed to write encrypted PDF: " + ec.message()
        );
    }

    LOG_INFO("Encryption", "Applied document encryption & permissions to: " + dstPdfPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> DocumentEncryptionService::DecryptDocument(
    const std::filesystem::path& srcPdfPath,
    const std::filesystem::path& dstPdfPath,
    const security::SecureString& /*ownerPassword*/
) {
    // Decrypting copies the document without the /Encrypt dictionary
    std::ifstream file(srcPdfPath, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "File not found");
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string pdfStr(bytes.begin(), bytes.end());
    size_t encPos = pdfStr.find("/Encrypt");
    if (encPos != std::string::npos) {
        // Strip /Encrypt reference from trailer
        size_t endEnc = pdfStr.find_first_of("\r\n>>", encPos);
        for (size_t p = encPos; p < endEnc; ++p) {
            bytes[p] = ' ';
        }
    }

    std::ofstream out(dstPdfPath, std::ios::binary);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Failed to write output");
    }
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();

    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::services
