// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "CryptoCertificateManager.hpp"
#include "application/Logger.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#include <dpapi.h>
#include <shlobj.h>
#endif

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace pdfinity::security {

namespace {

std::string HexEncode(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string ComputeSha256Thumbprint(const uint8_t* certData, size_t certLen) {
#if defined(_WIN32)
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            if (CryptHashData(hHash, certData, static_cast<DWORD>(certLen), 0)) {
                BYTE hashBytes[32];
                DWORD hashLen = sizeof(hashBytes);
                if (CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0)) {
                    CryptDestroyHash(hHash);
                    CryptReleaseContext(hProv, 0);
                    return HexEncode(hashBytes, hashLen);
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
#endif
    return "";
}

pdf_api::CertificateInfo ParseCertContext(PCCERT_CONTEXT pCert) {
    pdf_api::CertificateInfo info;
    if (!pCert) return info;

    // Subject
    char subjectBuf[512] = {0};
    CertGetNameStringA(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, subjectBuf, sizeof(subjectBuf));
    info.subject = std::string(subjectBuf);
    if (info.subject.empty()) {
        CertNameToStrA(pCert->dwCertEncodingType, &pCert->pCertInfo->Subject, CERT_X500_NAME_STR, subjectBuf, sizeof(subjectBuf));
        info.subject = std::string(subjectBuf);
    }

    // Issuer
    char issuerBuf[512] = {0};
    CertGetNameStringA(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, issuerBuf, sizeof(issuerBuf));
    info.issuer = std::string(issuerBuf);
    if (info.issuer.empty()) {
        CertNameToStrA(pCert->dwCertEncodingType, &pCert->pCertInfo->Issuer, CERT_X500_NAME_STR, issuerBuf, sizeof(issuerBuf));
        info.issuer = std::string(issuerBuf);
    }

    // Self-signed check
    info.isSelfSigned = (info.subject == info.issuer);

    // Serial number (reversed in CERT_INFO)
    std::ostringstream serOss;
    for (int i = static_cast<int>(pCert->pCertInfo->SerialNumber.cbData) - 1; i >= 0; --i) {
        serOss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
               << static_cast<int>(pCert->pCertInfo->SerialNumber.pbData[i]);
    }
    info.serialNumber = serOss.str();

    // Validity dates
    SYSTEMTIME stBefore{}, stAfter{}, stNow{};
    FileTimeToSystemTime(&pCert->pCertInfo->NotBefore, &stBefore);
    FileTimeToSystemTime(&pCert->pCertInfo->NotAfter, &stAfter);
    GetSystemTime(&stNow);

    char dateBuf[64];
    snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", stBefore.wYear, stBefore.wMonth, stBefore.wDay);
    info.validFrom = std::string(dateBuf);
    snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", stAfter.wYear, stAfter.wMonth, stAfter.wDay);
    info.validTo = std::string(dateBuf);

    FILETIME ftNow;
    SystemTimeToFileTime(&stNow, &ftNow);
    info.isExpired = (CompareFileTime(&ftNow, &pCert->pCertInfo->NotAfter) > 0);

    // SHA-256 Thumbprint
    info.thumbprintSha256 = ComputeSha256Thumbprint(pCert->pbCertEncoded, pCert->cbCertEncoded);

    return info;
}

} // namespace

CryptoCertificateManager& CryptoCertificateManager::Instance() {
    static CryptoCertificateManager instance;
    return instance;
}

CryptoCertificateManager::CryptoCertificateManager() {
    m_vaultDir = GetVaultDirectory();
    std::error_code ec;
    std::filesystem::create_directories(m_vaultDir, ec);
}

CryptoCertificateManager::~CryptoCertificateManager() = default;

std::filesystem::path CryptoCertificateManager::GetVaultDirectory() const {
#if defined(_WIN32)
    PWSTR localAppData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
        std::filesystem::path p(localAppData);
        CoTaskMemFree(localAppData);
        return p / "PDFinity" / "certs";
    }
#endif
    return std::filesystem::temp_directory_path() / "PDFinity" / "certs";
}

std::vector<uint8_t> CryptoCertificateManager::ProtectData(const std::vector<uint8_t>& plaintext) {
#if defined(_WIN32)
    if (plaintext.empty()) return {};
    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(plaintext.data());
    inBlob.cbData = static_cast<DWORD>(plaintext.size());
    DATA_BLOB outBlob{};
    if (CryptProtectData(&inBlob, L"PDFinityVaultKey", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        std::vector<uint8_t> cipher(outBlob.pbData, outBlob.pbData + outBlob.cbData);
        LocalFree(outBlob.pbData);
        return cipher;
    }
#endif
    return plaintext;
}

std::vector<uint8_t> CryptoCertificateManager::UnprotectData(const std::vector<uint8_t>& ciphertext) {
#if defined(_WIN32)
    if (ciphertext.empty()) return {};
    DATA_BLOB inBlob;
    inBlob.pbData = const_cast<BYTE*>(ciphertext.data());
    inBlob.cbData = static_cast<DWORD>(ciphertext.size());
    DATA_BLOB outBlob{};
    if (CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &outBlob)) {
        std::vector<uint8_t> plain(outBlob.pbData, outBlob.pbData + outBlob.cbData);
        LocalFree(outBlob.pbData);
        return plain;
    }
#endif
    return ciphertext;
}

pdf_api::EngineResult<pdf_api::CertificateInfo> CryptoCertificateManager::ImportPkcs12(
    const std::filesystem::path& pfxPath,
    const SecureString& password,
    const std::string& friendlyName
) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ifstream file(pfxPath, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
            pdf_api::EngineError::FileNotFound, "Could not open PKCS#12 file: " + pfxPath.string()
        );
    }
    std::vector<uint8_t> pfxData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (pfxData.empty()) {
        return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
            pdf_api::EngineError::InvalidFormat, "PKCS#12 file is empty"
        );
    }

#if defined(_WIN32)
    CRYPT_DATA_BLOB pfxBlob;
    pfxBlob.pbData = pfxData.data();
    pfxBlob.cbData = static_cast<DWORD>(pfxData.size());

    std::wstring widePassword;
    if (!password.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, password.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            widePassword.resize(wlen);
            MultiByteToWideChar(CP_UTF8, 0, password.c_str(), -1, widePassword.data(), wlen);
        }
    }

    HCERTSTORE hStore = PFXImportCertStore(
        &pfxBlob,
        widePassword.empty() ? nullptr : widePassword.c_str(),
        CRYPT_EXPORTABLE
    );
    if (!hStore) {
        DWORD err = GetLastError();
        return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
            pdf_api::EngineError::IncorrectPassword,
            "Failed to unlock PKCS#12 certificate. Invalid password or corrupted archive (Error " + std::to_string(err) + ")."
        );
    }

    PCCERT_CONTEXT pCert = CertEnumCertificatesInStore(hStore, nullptr);
    if (!pCert) {
        CertCloseStore(hStore, 0);
        return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
            pdf_api::EngineError::InvalidFormat, "No certificates found in PKCS#12 store"
        );
    }

    pdf_api::CertificateInfo info = ParseCertContext(pCert);
    if (!friendlyName.empty()) {
        info.subject = friendlyName;
    }

    // Save encrypted archive into secure vault (DPAPI-protected, no plaintext key on disk)
    std::vector<uint8_t> encryptedPfx = ProtectData(pfxData);
    std::filesystem::path certFilePath = m_vaultDir / (info.thumbprintSha256 + ".pfx.enc");
    std::ofstream out(certFilePath, std::ios::binary);
    if (out.is_open()) {
        out.write(reinterpret_cast<const char*>(encryptedPfx.data()), encryptedPfx.size());
        out.close();
    }

    // Store encrypted password metadata
    if (!password.empty()) {
        std::vector<uint8_t> passBytes(password.c_str(), password.c_str() + password.size());
        std::vector<uint8_t> encPass = ProtectData(passBytes);
        std::ofstream passOut(m_vaultDir / (info.thumbprintSha256 + ".pwd.enc"), std::ios::binary);
        if (passOut.is_open()) {
            passOut.write(reinterpret_cast<const char*>(encPass.data()), encPass.size());
            passOut.close();
        }
    }

    CertFreeCertificateContext(pCert);
    CertCloseStore(hStore, 0);

    LOG_INFO("Security", "PKCS#12 certificate successfully imported to secure vault: " + info.subject + " (" + info.thumbprintSha256 + ")");
    return pdf_api::EngineResult<pdf_api::CertificateInfo>::Ok(std::move(info));
#else
    return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
        pdf_api::EngineError::NotSupported, "PKCS#12 import not supported on this platform"
    );
#endif
}

std::vector<pdf_api::CertificateInfo> CryptoCertificateManager::GetInstalledCertificates() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<pdf_api::CertificateInfo> list;

    if (!std::filesystem::exists(m_vaultDir)) return list;

    for (const auto& entry : std::filesystem::directory_iterator(m_vaultDir)) {
        if (entry.path().extension() == ".enc" && entry.path().stem().extension() == ".pfx") {
            std::string stem = entry.path().stem().stem().string();
            // Read encrypted PFX
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file.is_open()) continue;
            std::vector<uint8_t> encData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            std::vector<uint8_t> pfxData = UnprotectData(encData);
            if (pfxData.empty()) continue;

            // Try reading password if cached
            std::wstring widePassword;
            std::filesystem::path pwdPath = m_vaultDir / (stem + ".pwd.enc");
            if (std::filesystem::exists(pwdPath)) {
                std::ifstream pFile(pwdPath, std::ios::binary);
                if (pFile.is_open()) {
                    std::vector<uint8_t> encPwd((std::istreambuf_iterator<char>(pFile)), std::istreambuf_iterator<char>());
                    pFile.close();
                    std::vector<uint8_t> pwd = UnprotectData(encPwd);
                    if (!pwd.empty()) {
                        std::string pStr(pwd.begin(), pwd.end());
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, pStr.c_str(), -1, nullptr, 0);
                        if (wlen > 0) {
                            widePassword.resize(wlen);
                            MultiByteToWideChar(CP_UTF8, 0, pStr.c_str(), -1, widePassword.data(), wlen);
                        }
                    }
                }
            }

#if defined(_WIN32)
            CRYPT_DATA_BLOB pfxBlob;
            pfxBlob.pbData = pfxData.data();
            pfxBlob.cbData = static_cast<DWORD>(pfxData.size());
            HCERTSTORE hStore = PFXImportCertStore(&pfxBlob, widePassword.empty() ? nullptr : widePassword.c_str(), 0);
            if (hStore) {
                PCCERT_CONTEXT pCert = CertEnumCertificatesInStore(hStore, nullptr);
                if (pCert) {
                    list.push_back(ParseCertContext(pCert));
                    CertFreeCertificateContext(pCert);
                }
                CertCloseStore(hStore, 0);
            }
#endif
        }
    }

    return list;
}

pdf_api::EngineResult<pdf_api::CertificateInfo> CryptoCertificateManager::GetCertificateByThumbprint(
    const std::string& thumbprint
) {
    auto certs = GetInstalledCertificates();
    for (const auto& c : certs) {
        if (c.thumbprintSha256 == thumbprint) {
            return pdf_api::EngineResult<pdf_api::CertificateInfo>::Ok(c);
        }
    }
    return pdf_api::EngineResult<pdf_api::CertificateInfo>::Fail(
        pdf_api::EngineError::FileNotFound, "Certificate thumbprint not found in vault: " + thumbprint
    );
}

bool CryptoCertificateManager::DeleteCertificate(const std::string& thumbprint) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool ok = true;
    std::error_code ec;
    std::filesystem::remove(m_vaultDir / (thumbprint + ".pfx.enc"), ec);
    std::filesystem::remove(m_vaultDir / (thumbprint + ".pwd.enc"), ec);
    return ok;
}

pdf_api::EngineResult<std::vector<uint8_t>> CryptoCertificateManager::CreateDetachedSignature(
    const std::string& thumbprint,
    const SecureString& password,
    const std::vector<uint8_t>& dataToSign
) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::filesystem::path pfxEncPath = m_vaultDir / (thumbprint + ".pfx.enc");
    if (!std::filesystem::exists(pfxEncPath)) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::FileNotFound, "Certificate not found in vault: " + thumbprint
        );
    }

    std::ifstream file(pfxEncPath, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::FileNotFound, "Failed to read encrypted certificate vault"
        );
    }
    std::vector<uint8_t> encData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::vector<uint8_t> pfxData = UnprotectData(encData);
    if (pfxData.empty()) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::IncorrectPassword, "DPAPI vault decryption failed"
        );
    }

#if defined(_WIN32)
    CRYPT_DATA_BLOB pfxBlob;
    pfxBlob.pbData = pfxData.data();
    pfxBlob.cbData = static_cast<DWORD>(pfxData.size());

    std::wstring widePassword;
    if (!password.empty()) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, password.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            widePassword.resize(wlen);
            MultiByteToWideChar(CP_UTF8, 0, password.c_str(), -1, widePassword.data(), wlen);
        }
    } else {
        // Try reading stored password
        std::filesystem::path pwdPath = m_vaultDir / (thumbprint + ".pwd.enc");
        if (std::filesystem::exists(pwdPath)) {
            std::ifstream pFile(pwdPath, std::ios::binary);
            if (pFile.is_open()) {
                std::vector<uint8_t> encPwd((std::istreambuf_iterator<char>(pFile)), std::istreambuf_iterator<char>());
                pFile.close();
                std::vector<uint8_t> pwd = UnprotectData(encPwd);
                if (!pwd.empty()) {
                    std::string pStr(pwd.begin(), pwd.end());
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, pStr.c_str(), -1, nullptr, 0);
                    if (wlen > 0) {
                        widePassword.resize(wlen);
                        MultiByteToWideChar(CP_UTF8, 0, pStr.c_str(), -1, widePassword.data(), wlen);
                    }
                }
            }
        }
    }

    HCERTSTORE hStore = PFXImportCertStore(
        &pfxBlob,
        widePassword.empty() ? nullptr : widePassword.c_str(),
        CRYPT_EXPORTABLE
    );
    if (!hStore) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::IncorrectPassword, "Failed to unlock certificate with provided password"
        );
    }

    PCCERT_CONTEXT pCert = CertEnumCertificatesInStore(hStore, nullptr);
    if (!pCert) {
        CertCloseStore(hStore, 0);
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::InvalidFormat, "No certificates in PFX"
        );
    }

    CRYPT_SIGN_MESSAGE_PARA signPara{};
    signPara.cbSize = sizeof(signPara);
    signPara.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    signPara.pSigningCert = pCert;
    signPara.HashAlgorithm.pszObjId = const_cast<char*>(szOID_NIST_sha256);
    signPara.cMsgCert = 1;
    signPara.rgpMsgCert = &pCert;

    const BYTE* rgpbToBeSigned[] = { dataToSign.data() };
    DWORD rgcbToBeSigned[] = { static_cast<DWORD>(dataToSign.size()) };

    DWORD cbSignedBlob = 0;
    if (!CryptSignMessage(&signPara, TRUE /* detached */, 1, rgpbToBeSigned, rgcbToBeSigned, nullptr, &cbSignedBlob)) {
        DWORD err = GetLastError();
        CertFreeCertificateContext(pCert);
        CertCloseStore(hStore, 0);
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::RenderFailed, "CryptSignMessage sizing failed with error: " + std::to_string(err)
        );
    }

    std::vector<uint8_t> signedBlob(cbSignedBlob);
    if (!CryptSignMessage(&signPara, TRUE, 1, rgpbToBeSigned, rgcbToBeSigned, signedBlob.data(), &cbSignedBlob)) {
        DWORD err = GetLastError();
        CertFreeCertificateContext(pCert);
        CertCloseStore(hStore, 0);
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
            pdf_api::EngineError::RenderFailed, "CryptSignMessage execution failed with error: " + std::to_string(err)
        );
    }

    CertFreeCertificateContext(pCert);
    CertCloseStore(hStore, 0);

    return pdf_api::EngineResult<std::vector<uint8_t>>::Ok(std::move(signedBlob));
#else
    return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(
        pdf_api::EngineError::NotSupported, "Cryptographic signing not supported on this platform"
    );
#endif
}

pdf_api::EngineResult<pdf_api::SignatureValidationResult> CryptoCertificateManager::VerifyDetachedSignature(
    const std::vector<uint8_t>& signedData,
    const std::vector<uint8_t>& pkcs7Der
) {
    if (signedData.empty() || pkcs7Der.empty()) {
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Fail(
            pdf_api::EngineError::InvalidFormat, "Empty data or PKCS#7 buffer"
        );
    }

    pdf_api::SignatureValidationResult res;
    res.filter = "Adobe.PPKLite";
    res.subFilter = "adbe.pkcs7.detached";

#if defined(_WIN32)
    CRYPT_VERIFY_MESSAGE_PARA verifyPara{};
    verifyPara.cbSize = sizeof(verifyPara);
    verifyPara.dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    const BYTE* rgpbToBeSigned[] = { signedData.data() };
    DWORD rgcbToBeSigned[] = { static_cast<DWORD>(signedData.size()) };

    PCCERT_CONTEXT pSignerCert = nullptr;
    BOOL verified = CryptVerifyDetachedMessageSignature(
        &verifyPara,
        0,
        pkcs7Der.data(),
        static_cast<DWORD>(pkcs7Der.size()),
        1,
        rgpbToBeSigned,
        rgcbToBeSigned,
        &pSignerCert
    );

    if (verified && pSignerCert) {
        res.hasValidDigest = true;
        res.isTampered = false;
        res.certificate = ParseCertContext(pSignerCert);
        res.isCertValid = !res.certificate.isExpired;

        // Chain validation
        CERT_CHAIN_PARA chainPara{};
        chainPara.cbSize = sizeof(chainPara);
        PCCERT_CHAIN_CONTEXT pChainContext = nullptr;
        if (CertGetCertificateChain(nullptr, pSignerCert, nullptr, nullptr, &chainPara, 0, nullptr, &pChainContext)) {
            if (pChainContext->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR) {
                res.isChainTrusted = true;
            }
            CertFreeCertificateChain(pChainContext);
        }

        res.trustSummary = res.hasValidDigest 
            ? "Signature is mathematically authentic. Document byte range has not been tampered with."
            : "Digest verification failed. Content modified.";
        res.revocationStatus = "Offline validation: OCSP/CRL revocation checking was skipped (offline mode).";

        CertFreeCertificateContext(pSignerCert);
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Ok(std::move(res));
    } else {
        DWORD err = GetLastError();
        res.hasValidDigest = false;
        res.isTampered = true;
        res.trustSummary = "CRYPTOGRAPHIC VERIFICATION FAILED: Signed byte range does not match signature digest (Error " +
                           std::to_string(err) + ").";
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Ok(std::move(res));
    }
#else
    return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Fail(
        pdf_api::EngineError::NotSupported, "Cryptographic verification not supported on this platform"
    );
#endif
}

} // namespace pdfinity::security
