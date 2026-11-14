// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/SignatureService.hpp"
#include "security/CryptoCertificateManager.hpp"

using namespace pdfinity;

class TestSignatureService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testIncrementalSigningAndValidation();
    void testTamperDetectionOnModifiedPdf();
    void testPostSigningModificationDetection();
    void testSecurityReportGeneration();

private:
    std::filesystem::path m_fixturesDir;
    std::string m_certThumbprint;
};

void TestSignatureService::initTestCase() {
#if !defined(_WIN32)
    QSKIP("SignatureService PKCS#12 signing requires Windows CryptoAPI.");
#endif
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    // Ensure test certificate is imported
    auto pfxPath = m_fixturesDir / "test_signer.pfx";
    QVERIFY(std::filesystem::exists(pfxPath));
    security::SecureString password("testpass123");
    auto res = security::CryptoCertificateManager::Instance().ImportPkcs12(
        pfxPath, password, "PDFinity Test Signer"
    );
    QVERIFY(res.has_value());
    m_certThumbprint = res.value.thumbprintSha256;
}

void TestSignatureService::testIncrementalSigningAndValidation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "signed_doc.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    options.reason = "Official Approval";
    options.location = "Secure Workstation";
    options.contactInfo = "signer@pdfinity.internal";

    security::SecureString pass("testpass123");
    auto signRes = services::SignatureService::SignDocumentIncremental(
        origPdf, signedPdf, options, pass
    );
    QVERIFY(signRes.has_value());
    QVERIFY(std::filesystem::exists(signedPdf));
    QVERIFY(std::filesystem::file_size(signedPdf) > std::filesystem::file_size(origPdf));

    // Validate signatures in signed PDF
    auto valRes = services::SignatureService::ValidateDocumentSignatures(signedPdf);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 1);

    const auto& sig = valRes.value[0];
    QVERIFY(sig.hasValidDigest);
    QVERIFY(!sig.isTampered);
    QVERIFY(!sig.hasModificationsAfterSigning);
    QCOMPARE(sig.reason, std::string("Official Approval"));
    QCOMPARE(sig.certificate.thumbprintSha256, m_certThumbprint);
}

void TestSignatureService::testTamperDetectionOnModifiedPdf() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "signed_doc.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    security::SecureString pass("testpass123");
    QVERIFY(services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass).has_value());

    // Tamper with the signed PDF by flipping 1 byte within the original content stream
    auto tamperedPdf = std::filesystem::path(tempDir.path().toStdString()) / "tampered_doc.pdf";
    std::ifstream in(signedPdf, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Modify a byte near the start of the original document
    bytes[50] ^= 0xFF;

    std::ofstream out(tamperedPdf, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();

    // Validate tampered document
    auto valRes = services::SignatureService::ValidateDocumentSignatures(tamperedPdf);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 1);

    const auto& sig = valRes.value[0];
    QVERIFY(!sig.hasValidDigest);
    QVERIFY(sig.isTampered);
}

void TestSignatureService::testPostSigningModificationDetection() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "signed_doc.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    security::SecureString pass("testpass123");
    QVERIFY(services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass).has_value());

    // Append unauthorized bytes after EOF
    auto modifiedPdf = std::filesystem::path(tempDir.path().toStdString()) / "appended_doc.pdf";
    std::ifstream in(signedPdf, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::string extraBytes = "\r\n% Unauthorized Post-Signing Modification\r\n";
    bytes.insert(bytes.end(), extraBytes.begin(), extraBytes.end());

    std::ofstream out(modifiedPdf, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();

    auto valRes = services::SignatureService::ValidateDocumentSignatures(modifiedPdf);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 1);

    const auto& sig = valRes.value[0];
    QVERIFY(sig.hasModificationsAfterSigning);
}

void TestSignatureService::testSecurityReportGeneration() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "signed_doc.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    options.reason = "Audit Compliance";
    security::SecureString pass("testpass123");
    QVERIFY(services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass).has_value());

    auto valRes = services::SignatureService::ValidateDocumentSignatures(signedPdf);
    QVERIFY(valRes.has_value());

    std::string report = services::SignatureService::GenerateSecurityReport(signedPdf, valRes.value);
    QVERIFY(!report.empty());
    QVERIFY(report.find("PDFinity Security and Digital Signature Audit Report") != std::string::npos);
    QVERIFY(report.find("Mathematical Integrity") != std::string::npos);
    QVERIFY(report.find("Trust & Offline Limitation Disclosure") != std::string::npos);
}

QTEST_MAIN(TestSignatureService)
#include "test_signature_service.moc"
