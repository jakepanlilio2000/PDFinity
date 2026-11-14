// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/DocumentEncryptionService.hpp"

using namespace pdfinity;

class TestEncryptionService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testPermissionsMaskRoundtrip();
    void testEncryptDocumentAppliesPermissions();
    void testDecryptDocumentStripsEncrypt();

private:
    std::filesystem::path m_fixturesDir;
};

void TestEncryptionService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestEncryptionService::testPermissionsMaskRoundtrip() {
    pdf_api::DocumentPermissions perms;
    perms.canPrint = true;
    perms.canModifyContents = false;
    perms.canCopyText = false;
    perms.canAnnotate = true;
    perms.canFillForms = true;
    perms.canExtractAccessibility = true;
    perms.canAssemble = false;
    perms.canPrintHighQuality = false;

    int32_t mask = services::DocumentEncryptionService::ComputePermissionsMask(perms);
    auto parsed = services::DocumentEncryptionService::ParsePermissionsMask(mask);

    QCOMPARE(parsed.canPrint, perms.canPrint);
    QCOMPARE(parsed.canModifyContents, perms.canModifyContents);
    QCOMPARE(parsed.canCopyText, perms.canCopyText);
    QCOMPARE(parsed.canAnnotate, perms.canAnnotate);
    QCOMPARE(parsed.canFillForms, perms.canFillForms);
    QCOMPARE(parsed.canExtractAccessibility, perms.canExtractAccessibility);
    QCOMPARE(parsed.canAssemble, perms.canAssemble);
    QCOMPARE(parsed.canPrintHighQuality, perms.canPrintHighQuality);
}

void TestEncryptionService::testEncryptDocumentAppliesPermissions() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto encPdf = std::filesystem::path(tempDir.path().toStdString()) / "encrypted_doc.pdf";

    pdf_api::DocumentEncryptionConfig config;
    config.userPassword = "user123";
    config.ownerPassword = "owner456";
    config.permissions.canPrint = true;
    config.permissions.canCopyText = false;
    config.permissions.canModifyContents = false;

    auto encRes = services::DocumentEncryptionService::EncryptDocument(origPdf, encPdf, config);
    QVERIFY(encRes.has_value());
    QVERIFY(std::filesystem::exists(encPdf));

    // Inspect file for /Encrypt dictionary
    std::ifstream in(encPdf, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(content.find("/Encrypt") != std::string::npos);
    QVERIFY(content.find("/Filter /Standard") != std::string::npos);
    QVERIFY(content.find("/V 5") != std::string::npos);
}

void TestEncryptionService::testDecryptDocumentStripsEncrypt() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto origPdf = m_fixturesDir / "ordinary.pdf";
    auto encPdf = std::filesystem::path(tempDir.path().toStdString()) / "encrypted_doc.pdf";
    auto decPdf = std::filesystem::path(tempDir.path().toStdString()) / "decrypted_doc.pdf";

    pdf_api::DocumentEncryptionConfig config;
    config.userPassword = "user123";
    config.ownerPassword = "owner456";
    QVERIFY(services::DocumentEncryptionService::EncryptDocument(origPdf, encPdf, config).has_value());

    security::SecureString ownerPass("owner456");
    auto decRes = services::DocumentEncryptionService::DecryptDocument(encPdf, decPdf, ownerPass);
    QVERIFY(decRes.has_value());
    QVERIFY(std::filesystem::exists(decPdf));

    std::ifstream in(decPdf, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(content.find("/Encrypt") == std::string::npos);
}

QTEST_MAIN(TestEncryptionService)
#include "test_encryption_service.moc"
