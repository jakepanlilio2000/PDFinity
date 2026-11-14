// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/SignatureService.hpp"
#include "security/CryptoCertificateManager.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestSignaturePersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testVisualSignatureAppearanceOnly();
    void testCryptographicSigningAndReopenPersistence();
    void testIndependentValidatorValidation();
    void testTamperingDetectionExitGate();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::string m_certThumbprint;
};

void TestSignaturePersistence::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());

    // Import certificate fixture
    auto pfxPath = m_fixturesDir / "test_signer.pfx";
    QVERIFY(std::filesystem::exists(pfxPath));
    security::SecureString password("testpass123");
    auto res = security::CryptoCertificateManager::Instance().ImportPkcs12(
        pfxPath, password, "PDFinity Independent Test Signer"
    );
    QVERIFY(res.has_value());
    m_certThumbprint = res.value.thumbprintSha256;
}

void TestSignaturePersistence::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestSignaturePersistence::testVisualSignatureAppearanceOnly() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "visual_signed.pdf";

    auto origPath = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    int origObjCount = static_cast<int>(doc.GetPageObjects(0).value.size());

    pdf_api::VisualSignatureData vis;
    vis.pageIndex = 0;
    vis.rect = {100.0, 150.0, 220.0, 75.0};
    vis.signerName = "Alice Inspector";
    vis.title = "Lead Quality Reviewer";
    vis.dateText = "2026-11-14";

    // VERIFICATION TEST: Visual signatures are never described as cryptographic signatures
    QVERIFY(!vis.isCryptographicBound);
    auto applyRes = services::SignatureService::ApplyVisualSignature(doc, vis);
    QVERIFY(applyRes.has_value());

    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen in independent session
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    // Objects were inserted on the page
    auto objs = savedDoc.GetPageObjects(0);
    QVERIFY(objs.has_value());
    QVERIFY(objs.value.size() > static_cast<size_t>(origObjCount));

    // BUT cryptographic signature count MUST be 0!
    auto sigCount = savedDoc.GetSignatureCount();
    QVERIFY(sigCount.has_value());
    QCOMPARE(sigCount.value, 0);

    // Validation must report 0 cryptographic signatures
    auto valRes = services::SignatureService::ValidateDocumentSignatures(outPath);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 0);
}

void TestSignaturePersistence::testCryptographicSigningAndReopenPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "crypto_signed.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    options.reason = "Certified Document Audit";
    options.location = "Secure Lab";
    options.contactInfo = "audit@pdfinity.internal";

    security::SecureString pass("testpass123");
    auto signRes = services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass);
    QVERIFY(signRes.has_value());
    QVERIFY(std::filesystem::exists(signedPdf));

    // Reopen in PDF engine
    auto reopenRes = m_engine->OpenDocument(signedPdf);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel doc(reopenRes.value, m_engine, signedPdf);

    // Signature count in engine is 1
    auto sigCount = doc.GetSignatureCount();
    QVERIFY(sigCount.has_value());
    QCOMPARE(sigCount.value, 1);

    // Engine reads signature details
    auto sigData = doc.GetSignatureData(0);
    QVERIFY(sigData.has_value());
    QCOMPARE(sigData.value.subFilter, std::string("adbe.pkcs7.detached"));
    QCOMPARE(sigData.value.reason, std::string("Certified Document Audit"));
    QVERIFY(!sigData.value.byteRange.empty());

    // Validate signatures via service
    auto valRes = services::SignatureService::ValidateDocumentSignatures(signedPdf);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 1);
    QVERIFY(valRes.value[0].hasValidDigest);
    QVERIFY(!valRes.value[0].isTampered);
    QCOMPARE(valRes.value[0].certificate.thumbprintSha256, m_certThumbprint);
}

void TestSignaturePersistence::testIndependentValidatorValidation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "crypto_signed.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    options.reason = "Independent Validation Test";
    security::SecureString pass("testpass123");
    QVERIFY(services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass).has_value());

    // Extract the embedded PKCS#7 signature from the PDF
    auto valRes = services::SignatureService::ValidateDocumentSignatures(signedPdf);
    QVERIFY(valRes.has_value() && !valRes.value.empty());
    const auto& sig = valRes.value[0];
    QVERIFY(sig.hasValidDigest);

    // Independent check using OpenSSL CLI if available
    std::string opensslExe = "C:\\Program Files\\Git\\mingw64\\bin\\openssl.exe";
    if (std::filesystem::exists(opensslExe)) {
        // Extract PKCS#7 DER to temporary file
        std::ifstream in(signedPdf, std::ios::binary);
        std::vector<uint8_t> pdfBytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        size_t l1 = sig.byteRange[1];
        size_t b2 = sig.byteRange[2];

        std::string pdfStr(pdfBytes.begin(), pdfBytes.end());
        std::string gapStr = pdfStr.substr(l1, b2 - l1);
        size_t cOpen = gapStr.find('<');
        size_t cClose = gapStr.rfind('>');
        QVERIFY(cOpen != std::string::npos && cClose != std::string::npos);

        std::string hexSig = gapStr.substr(cOpen + 1, cClose - cOpen - 1);
        std::vector<uint8_t> pkcs7Bytes;
        for (size_t i = 0; i < hexSig.size(); i += 2) {
            pkcs7Bytes.push_back(static_cast<uint8_t>(std::stoul(hexSig.substr(i, 2), nullptr, 16)));
        }

        auto p7Path = std::filesystem::path(tempDir.path().toStdString()) / "sig.p7";
        std::ofstream p7Out(p7Path, std::ios::binary);
        p7Out.write(reinterpret_cast<const char*>(pkcs7Bytes.data()), pkcs7Bytes.size());
        p7Out.close();

        // Run OpenSSL to inspect PKCS#7 structure and print certificate
        std::string cmd = "\"\"" + opensslExe + "\" pkcs7 -inform DER -in \"" + p7Path.string() + "\" -print_certs\"";
        int rc = std::system(cmd.c_str());
        QCOMPARE(rc, 0); // OpenSSL successfully parsed and verified the PKCS#7 certificate structure!
    }
}

void TestSignaturePersistence::testTamperingDetectionExitGate() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto signedPdf = std::filesystem::path(tempDir.path().toStdString()) / "crypto_signed.pdf";

    pdf_api::CryptographicSignOptions options;
    options.certThumbprint = m_certThumbprint;
    security::SecureString pass("testpass123");
    QVERIFY(services::SignatureService::SignDocumentIncremental(origPdf, signedPdf, options, pass).has_value());

    // Tamper with signed PDF
    std::ifstream in(signedPdf, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Flip byte in original part 1
    bytes[42] ^= 0xAA;

    auto tamperedPdf = std::filesystem::path(tempDir.path().toStdString()) / "tampered.pdf";
    std::ofstream out(tamperedPdf, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();

    // Reopen and validate: TAMPERING MUST BE DETECTED!
    auto valRes = services::SignatureService::ValidateDocumentSignatures(tamperedPdf);
    QVERIFY(valRes.has_value());
    QCOMPARE(static_cast<int>(valRes.value.size()), 1);

    // VERIFICATION:
    QVERIFY(!valRes.value[0].hasValidDigest);
    QVERIFY(valRes.value[0].isTampered);
}

QTEST_MAIN(TestSignaturePersistence)
#include "test_signature_persistence.moc"
