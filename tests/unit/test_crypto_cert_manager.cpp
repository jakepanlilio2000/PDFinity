// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <filesystem>
#include "security/CryptoCertificateManager.hpp"

using namespace pdfinity;

class TestCryptoCertManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testImportPkcs12Valid();
    void testImportPkcs12InvalidPassword();
    void testVaultListingAndRetrieval();
    void testDetachedSignatureAndVerification();
    void testTamperedDataSignatureVerification();

private:
    std::filesystem::path m_fixturesDir;
    std::string m_importedThumbprint;
};

void TestCryptoCertManager::initTestCase() {
#if !defined(_WIN32)
    QSKIP("CryptoCertificateManager requires Windows CryptoAPI and DPAPI vault.");
#endif
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestCryptoCertManager::testImportPkcs12Valid() {
    auto pfxPath = m_fixturesDir / "test_signer.pfx";
    QVERIFY2(std::filesystem::exists(pfxPath), "test_signer.pfx fixture missing");

    security::SecureString password("testpass123");
    auto res = security::CryptoCertificateManager::Instance().ImportPkcs12(
        pfxPath, password, "PDFinity Test Signer"
    );

    QVERIFY(res.has_value());
    const auto& info = res.value;
    QCOMPARE(info.subject, std::string("PDFinity Test Signer"));
    QVERIFY(!info.serialNumber.empty());
    QVERIFY(!info.thumbprintSha256.empty());
    QVERIFY(!info.isExpired);
    QVERIFY(!info.validFrom.empty());
    QVERIFY(!info.validTo.empty());

    m_importedThumbprint = info.thumbprintSha256;
}

void TestCryptoCertManager::testImportPkcs12InvalidPassword() {
    auto pfxPath = m_fixturesDir / "test_signer.pfx";
    security::SecureString wrongPassword("wrong_password_123");
    auto res = security::CryptoCertificateManager::Instance().ImportPkcs12(
        pfxPath, wrongPassword
    );

    QVERIFY(!res.has_value());
    QCOMPARE(res.error, pdf_api::EngineError::IncorrectPassword);
}

void TestCryptoCertManager::testVaultListingAndRetrieval() {
    QVERIFY(!m_importedThumbprint.empty());

    auto certs = security::CryptoCertificateManager::Instance().GetInstalledCertificates();
    QVERIFY(!certs.empty());

    bool found = false;
    for (const auto& c : certs) {
        if (c.thumbprintSha256 == m_importedThumbprint) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    auto getRes = security::CryptoCertificateManager::Instance().GetCertificateByThumbprint(m_importedThumbprint);
    QVERIFY(getRes.has_value());
    QCOMPARE(getRes.value.thumbprintSha256, m_importedThumbprint);
}

void TestCryptoCertManager::testDetachedSignatureAndVerification() {
    QVERIFY(!m_importedThumbprint.empty());

    std::string testText = "Authentic PDF Content Stream To Be Signed";
    std::vector<uint8_t> data(testText.begin(), testText.end());

    security::SecureString pass("testpass123");
    auto signRes = security::CryptoCertificateManager::Instance().CreateDetachedSignature(
        m_importedThumbprint, pass, data
    );

    QVERIFY(signRes.has_value());
    QVERIFY(!signRes.value.empty());

    // Verify signature against authentic data
    auto verifyRes = security::CryptoCertificateManager::Instance().VerifyDetachedSignature(
        data, signRes.value
    );

    QVERIFY(verifyRes.has_value());
    QVERIFY(verifyRes.value.hasValidDigest);
    QVERIFY(!verifyRes.value.isTampered);
    QCOMPARE(verifyRes.value.certificate.thumbprintSha256, m_importedThumbprint);
}

void TestCryptoCertManager::testTamperedDataSignatureVerification() {
    QVERIFY(!m_importedThumbprint.empty());

    std::string originalText = "Original Untampered Data";
    std::vector<uint8_t> originalData(originalText.begin(), originalText.end());

    security::SecureString pass("testpass123");
    auto signRes = security::CryptoCertificateManager::Instance().CreateDetachedSignature(
        m_importedThumbprint, pass, originalData
    );
    QVERIFY(signRes.has_value());

    // Tamper with data by changing 1 byte
    std::string tamperedText = "Original Vntampered Data"; // 'U' changed to 'V'
    std::vector<uint8_t> tamperedData(tamperedText.begin(), tamperedText.end());

    auto verifyRes = security::CryptoCertificateManager::Instance().VerifyDetachedSignature(
        tamperedData, signRes.value
    );

    QVERIFY(verifyRes.has_value());
    QVERIFY(!verifyRes.value.hasValidDigest);
    QVERIFY(verifyRes.value.isTampered);
}

QTEST_MAIN(TestCryptoCertManager)
#include "test_crypto_cert_manager.moc"
