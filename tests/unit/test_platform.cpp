// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include "security/SecureString.hpp"
#include "security/PathSanitizer.hpp"
#include "security/LogRedactor.hpp"
#include "platform/PlatformFactory.hpp"

using namespace pdfinity::security;
using namespace pdfinity::platform;

class TestPlatformAndSecurity : public QObject {
    Q_OBJECT

private slots:
    void testSecureStringLifecycle();
    void testPathSanitizerValidPaths();
    void testPathSanitizerReservedNames();
    void testPathSanitizerBoundary();
    void testPathSanitizerSafeFileName();
    void testLogRedactor();
    void testPlatformAdapter();
};

void TestPlatformAndSecurity::testSecureStringLifecycle() {
    SecureString s1("SensitiveSecretValue123");
    QCOMPARE(s1.size(), static_cast<size_t>(23));
    QCOMPARE(s1.empty(), false);
    QCOMPARE(QString(s1.c_str()), QString("SensitiveSecretValue123"));

    // Copy
    SecureString s2 = s1;
    QCOMPARE(QString(s2.c_str()), QString("SensitiveSecretValue123"));

    // Move
    SecureString s3 = std::move(s1);
    QCOMPARE(QString(s3.c_str()), QString("SensitiveSecretValue123"));

    // Clear and wipe
    s2.clear();
    QCOMPARE(s2.size(), static_cast<size_t>(0));
    QCOMPARE(s2.empty(), true);
}

void TestPlatformAndSecurity::testPathSanitizerValidPaths() {
    auto resEmpty = PathSanitizer::Canonicalize("");
    QCOMPARE(resEmpty.has_value(), false);
    QCOMPARE(resEmpty.error(), PathError::EmptyPath);

    std::string nullPath = "foo";
    nullPath += '\0';
    nullPath += "bar.pdf";
    auto resNull = PathSanitizer::Canonicalize(nullPath);
    QCOMPARE(resNull.has_value(), false);
    QCOMPARE(resNull.error(), PathError::InvalidCharacters);

    auto valid = PathSanitizer::Canonicalize("C:/Windows");
    QCOMPARE(valid.has_value(), true);
}

void TestPlatformAndSecurity::testPathSanitizerReservedNames() {
    auto resCon = PathSanitizer::Canonicalize("C:/temp/CON.pdf");
    QCOMPARE(resCon.has_value(), false);
    QCOMPARE(resCon.error(), PathError::ReservedDeviceName);

    auto resNul = PathSanitizer::Canonicalize("C:/test/NUL.txt");
    QCOMPARE(resNul.has_value(), false);
    QCOMPARE(resNul.error(), PathError::ReservedDeviceName);

    auto resAux = PathSanitizer::Canonicalize("AUX");
    QCOMPARE(resAux.has_value(), false);
    QCOMPARE(resAux.error(), PathError::ReservedDeviceName);
}

void TestPlatformAndSecurity::testPathSanitizerBoundary() {
    std::filesystem::path root = "C:/PDFinity/Workspace";
    std::filesystem::path safeChild = "C:/PDFinity/Workspace/docs/sample.pdf";
    std::filesystem::path maliciousEscape = "C:/PDFinity/Workspace/../../Windows/System32/calc.exe";

    QCOMPARE(PathSanitizer::IsWithinBoundary(safeChild, root), true);
    QCOMPARE(PathSanitizer::IsWithinBoundary(maliciousEscape, root), false);
}

void TestPlatformAndSecurity::testPathSanitizerSafeFileName() {
    QCOMPARE(PathSanitizer::IsSafeFileName("invoice_2026.pdf"), true);
    QCOMPARE(PathSanitizer::IsSafeFileName("contract.final.pdf"), true);

    // Unsafe
    QCOMPARE(PathSanitizer::IsSafeFileName(""), false);
    QCOMPARE(PathSanitizer::IsSafeFileName(".."), false);
    QCOMPARE(PathSanitizer::IsSafeFileName("dir/file.pdf"), false);
    QCOMPARE(PathSanitizer::IsSafeFileName("dir\\file.pdf"), false);
    QCOMPARE(PathSanitizer::IsSafeFileName("stream:file.pdf"), false);
    QCOMPARE(PathSanitizer::IsSafeFileName("NUL.pdf"), false);
    QCOMPARE(PathSanitizer::IsSafeFileName("CON"), false);
}

void TestPlatformAndSecurity::testLogRedactor() {
    LogRedactor redactor;

    std::string sensitive = "User login with password=SecretPassword123! from terminal.";
    std::string redacted = redactor.Redact(sensitive);
    QVERIFY(redacted.find("SecretPassword123!") == std::string::npos);
    QVERIFY(redacted.find("password=********") != std::string::npos);

    // Custom pattern rule
    redactor.AddRedactionPattern("api_key_xyz", "[REDACTED_KEY]");
    std::string tokenLog = "Calling service with token: api_key_xyz.";
    std::string tokenRedacted = redactor.Redact(tokenLog);
    QVERIFY(tokenRedacted.find("api_key_xyz") == std::string::npos);
    QVERIFY(tokenRedacted.find("[REDACTED_KEY]") != std::string::npos);
}

void TestPlatformAndSecurity::testPlatformAdapter() {
    auto adapter = PlatformFactory::CreateAdapter();
    QVERIFY(adapter != nullptr);

    QVERIFY(!adapter->GetPlatformName().empty());
    QVERIFY(!adapter->GetPlatformVersion().empty());
    QVERIFY(!adapter->GetArchitecture().empty());

    QVERIFY(adapter->GetPrimaryScreenDpi() > 0.0);
    QVERIFY(!adapter->GetAppDataDirectory().empty());
    QVERIFY(!adapter->GetDocumentsDirectory().empty());
    // By default, unit tests run in standard mode unless portable.dat exists
    QCOMPARE(adapter->IsPortableMode(), false);

#if defined(_WIN32)
    QVERIFY(adapter->GetPlatformName().find("Windows") != std::string::npos);
#endif
}

QTEST_MAIN(TestPlatformAndSecurity)
#include "test_platform.moc"
