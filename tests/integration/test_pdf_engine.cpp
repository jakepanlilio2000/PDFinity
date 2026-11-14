// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "services/DocumentLifecycleService.hpp"
#include "security/SecureString.hpp"

using namespace pdfinity;

class TestPdfEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testOpenOrdinaryDocument();
    void testRenderPage();
    void testUnicodeMetadata();
    void testImageHeavyDocument();
    void testEncryptedDocumentLifecycle();
    void testMalformedFilesResilience();
    void testFileLimitsEnforcement();
    void testSaveCopyAtomic();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::unique_ptr<services::DocumentLifecycleService> m_service;
    std::filesystem::path m_fixturesDir;
};

void TestPdfEngine::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_service = std::make_unique<services::DocumentLifecycleService>(m_engine);

    // Locate fixtures directory relative to source tree
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestPdfEngine::cleanupTestCase() {
    m_service.reset();
    m_engine->Shutdown();
    m_engine.reset();
}

void TestPdfEngine::testOpenOrdinaryDocument() {
    auto path = m_fixturesDir / "ordinary.pdf";
    auto res = m_service->OpenDocument(path);
    QVERIFY(res.has_value());

    auto& doc = res.value;
    QVERIFY(doc != nullptr);
    QVERIFY(doc->IsValid());
    QCOMPARE(doc->GetPageCount(), 2);

    // Inspect page sizes
    auto size1 = doc->GetPageSize(0);
    QVERIFY(size1.has_value());
    QCOMPARE(static_cast<int>(size1.value.width), 612);
    QCOMPARE(static_cast<int>(size1.value.height), 792);

    auto size2 = doc->GetPageSize(1);
    QVERIFY(size2.has_value());
    QCOMPARE(static_cast<int>(size2.value.width), 595);
    QCOMPARE(static_cast<int>(size2.value.height), 842);

    // Inspect metadata
    const auto& meta = doc->GetMetadata();
    QCOMPARE(QString::fromStdString(meta.title), QString("PDFinity Ordinary Document"));
    QCOMPARE(QString::fromStdString(meta.author), QString("Lead C++ Architect"));
    QCOMPARE(QString::fromStdString(meta.subject), QString("Integration Testing"));

    // Inspect permissions
    const auto& perms = doc->GetPermissions();
    QCOMPARE(perms.isEncrypted, false);
    QCOMPARE(perms.canPrint, true);
    QCOMPARE(perms.canCopyText, true);
}

void TestPdfEngine::testRenderPage() {
    auto path = m_fixturesDir / "ordinary.pdf";
    auto res = m_service->OpenDocument(path);
    QVERIFY(res.has_value());

    pdf_api::RenderOptions opts;
    opts.dpi = 150;
    opts.scale = 1.0;

    auto renderRes = res.value->RenderPage(0, opts);
    QVERIFY(renderRes.has_value());

    const auto& img = renderRes.value;
    QVERIFY(img.width > 0);
    QVERIFY(img.height > 0);
    QVERIFY(img.stride >= img.width * 4);
    QCOMPARE(img.pixels.size(), static_cast<size_t>(img.stride * img.height));
}

void TestPdfEngine::testUnicodeMetadata() {
    auto path = m_fixturesDir / "unicode_metadata.pdf";
    auto res = m_service->OpenDocument(path);
    QVERIFY(res.has_value());

    const auto& meta = res.value->GetMetadata();
    QString title = QString::fromStdString(meta.title);
    QString author = QString::fromStdString(meta.author);

    QVERIFY(title.contains("PDFinity"));
    QVERIFY(title.contains(QString::fromUtf8("ドキュメント")));
    QVERIFY(author.contains(QString::fromUtf8("Александр Пушкин")));
}

void TestPdfEngine::testImageHeavyDocument() {
    auto path = m_fixturesDir / "image_heavy.pdf";
    auto res = m_service->OpenDocument(path);
    QVERIFY(res.has_value());

    pdf_api::RenderOptions opts;
    opts.dpi = 96;
    auto renderRes = res.value->RenderPage(0, opts);
    QVERIFY(renderRes.has_value());
    QVERIFY(renderRes.value.width > 0);
}

void TestPdfEngine::testEncryptedDocumentLifecycle() {
    auto path = m_fixturesDir / "encrypted_user_pass.pdf";

    // 1. Open without password -> Must fail with PasswordRequired
    auto resNoPwd = m_service->OpenDocument(path, security::SecureString{});
    QCOMPARE(resNoPwd.has_value(), false);
    QCOMPARE(resNoPwd.error, pdf_api::EngineError::PasswordRequired);

    // 2. Open with wrong password -> Must fail with IncorrectPassword
    security::SecureString wrongPwd("wrong_password_xyz");
    auto resWrongPwd = m_service->OpenDocument(path, wrongPwd);
    QCOMPARE(resWrongPwd.has_value(), false);
    QCOMPARE(resWrongPwd.error, pdf_api::EngineError::IncorrectPassword);

    // 3. Open with correct password -> Must succeed
    security::SecureString correctPwd("testpass123");
    auto resCorrect = m_service->OpenDocument(path, correctPwd);
    QVERIFY(resCorrect.has_value());
    QCOMPARE(resCorrect.value->GetPageCount(), 1);
    QCOMPARE(resCorrect.value->GetPermissions().isEncrypted, true);
}

void TestPdfEngine::testMalformedFilesResilience() {
    // 1. Corrupt header
    auto resHdr = m_service->OpenDocument(m_fixturesDir / "malformed_corrupt_header.pdf");
    QCOMPARE(resHdr.has_value(), false);

    // 2. Corrupt xref
    auto resXref = m_service->OpenDocument(m_fixturesDir / "malformed_corrupt_xref.pdf");
    QCOMPARE(resXref.has_value(), false);

    // 3. Empty file
    auto resEmpty = m_service->OpenDocument(m_fixturesDir / "malformed_empty.pdf");
    QCOMPARE(resEmpty.has_value(), false);

    // 4. Nonexistent file
    auto resNonexistent = m_service->OpenDocument(m_fixturesDir / "does_not_exist_98765.pdf");
    QCOMPARE(resNonexistent.has_value(), false);
    QCOMPARE(resNonexistent.error, pdf_api::EngineError::FileNotFound);
}

void TestPdfEngine::testFileLimitsEnforcement() {
    auto path = m_fixturesDir / "ordinary.pdf";

    // Set artificially low file size limit (50 bytes)
    m_engine->SetLimits(50, 10000);
    auto resOversized = m_service->OpenDocument(path);
    QCOMPARE(resOversized.has_value(), false);
    QCOMPARE(resOversized.error, pdf_api::EngineError::FileTooLarge);

    // Restore limits
    m_engine->SetLimits(500 * 1024 * 1024, 10000);
    auto resValid = m_service->OpenDocument(path);
    QVERIFY(resValid.has_value());
}

void TestPdfEngine::testSaveCopyAtomic() {
    auto path = m_fixturesDir / "ordinary.pdf";
    auto res = m_service->OpenDocument(path);
    QVERIFY(res.has_value());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    std::filesystem::path copyPath = tempDir.path().toStdString() + "/saved_copy.pdf";

    auto saveRes = m_service->SaveCopy(*res.value, copyPath);
    QVERIFY(saveRes.has_value());
    QVERIFY(std::filesystem::exists(copyPath));
    QVERIFY(std::filesystem::file_size(copyPath) > 0);

    // Open saved copy and verify integrity
    auto reopened = m_service->OpenDocument(copyPath);
    QVERIFY(reopened.has_value());
    QCOMPARE(reopened.value->GetPageCount(), 2);
}

QTEST_MAIN(TestPdfEngine)
#include "test_pdf_engine.moc"
#include <algorithm>
