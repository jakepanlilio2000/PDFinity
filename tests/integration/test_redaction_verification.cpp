// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <QByteArray>
#include <QCryptographicHash>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "services/RedactionService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

namespace {

std::string ComputeFileSha256(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in.is_open()) return "";
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(QByteArray::fromRawData(reinterpret_cast<const char*>(bytes.data()),
                                       static_cast<qsizetype>(bytes.size())));
    return hash.result().toHex().toStdString();
}

} // namespace

class TestRedactionVerification : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAdversarialTextRedactionCannotBeRecovered();
    void testAdversarialImageRedaction();
    void testAdversarialMetadataSanitization();
    void testExitGateUnsupportedContentPreventsSuccess();
    void testSourcePreservationHashUnchanged();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestRedactionVerification::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestRedactionVerification::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestRedactionVerification::testAdversarialTextRedactionCannotBeRecovered() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "adversarial_redacted.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    // Mark 1: Cover "TOP_SECRET_CODENAME_ALPHA99" (y ~ 720)
    pdf_api::RedactionMark m1;
    m1.id = "redact-alpha99";
    m1.pageIndex = 0;
    m1.rect = {45.0, 710.0, 300.0, 25.0};
    m1.overlayText = "[REDACTED-ID]";
    service.AddRedactionMark(m1);

    // Mark 2: Cover "Employee SSN: 999-88-7777" (y ~ 680)
    pdf_api::RedactionMark m2;
    m2.id = "redact-ssn";
    m2.pageIndex = 0;
    m2.rect = {45.0, 670.0, 350.0, 25.0};
    m2.overlayText = "[REDACTED-PII]";
    service.AddRedactionMark(m2);

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf);
    QVERIFY(repRes.has_value());
    QVERIFY(repRes.value.isSuccess);

    // =========================================================================
    // MULTI-METHOD ADVERSARIAL VERIFICATION
    // =========================================================================

    // 1. Text Extraction Verification
    auto reopenRes = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel redactedDoc(reopenRes.value, m_engine, dstPdf);

    auto pageTextRes = redactedDoc.ExtractPageText(0);
    QVERIFY(pageTextRes.has_value());
    const std::string& extractedText = pageTextRes.value;

    // Sensitive strings MUST NOT exist!
    QVERIFY(extractedText.find("TOP_SECRET") == std::string::npos);
    QVERIFY(extractedText.find("ALPHA99") == std::string::npos);
    QVERIFY(extractedText.find("999-88-7777") == std::string::npos);

    // Unredacted public text MUST remain intact!
    QVERIFY(extractedText.find("Public Notice") != std::string::npos);
    QVERIFY(extractedText.find("unclassified general information") != std::string::npos);

    // 2. Text Search Verification
    auto s1 = redactedDoc.SearchPageText(0, "TOP_SECRET");
    QVERIFY(s1.has_value() && s1.value.empty());

    auto s2 = redactedDoc.SearchPageText(0, "999-88-7777");
    QVERIFY(s2.has_value() && s2.value.empty());

    auto sPublic = redactedDoc.SearchPageText(0, "Public Notice");
    QVERIFY(sPublic.has_value() && !sPublic.value.empty());

    // 3. Object Inspection Verification
    auto objsRes = redactedDoc.GetPageObjects(0);
    QVERIFY(objsRes.has_value());
    for (const auto& obj : objsRes.value) {
        if (obj.type == pdf_api::PageObjectType::Text) {
            QVERIFY(obj.text.find("TOP_SECRET") == std::string::npos);
            QVERIFY(obj.text.find("999-88-7777") == std::string::npos);
        }
    }

    // 4. Raw Binary Stream Inspection
    std::ifstream in(dstPdf, std::ios::binary);
    std::string rawBytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(rawBytes.find("TOP_SECRET_CODENAME_ALPHA99") == std::string::npos);
    QVERIFY(rawBytes.find("999-88-7777") == std::string::npos);

    // 5. Service-level verification call
    auto verifyRes = services::RedactionService::VerifyRedactedDocument(
        dstPdf, {"TOP_SECRET_CODENAME_ALPHA99", "999-88-7777"}
    );
    QVERIFY(verifyRes.has_value());
    QVERIFY(verifyRes.value);
}

void TestRedactionVerification::testAdversarialImageRedaction() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "scanned_page.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "redacted_image.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    // Cover the 16x16 image placed on scanned_page
    pdf_api::RedactionMark mImg;
    mImg.id = "redact-scan";
    mImg.pageIndex = 0;
    mImg.rect = {40.0, 40.0, 520.0, 720.0}; // covers entire scanned image
    service.AddRedactionMark(mImg);

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf);
    QVERIFY(repRes.has_value());
    QVERIFY(repRes.value.isSuccess);

    // Reopen and verify image object was destroyed
    auto reopenRes = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel redactedDoc(reopenRes.value, m_engine, dstPdf);

    auto objs = redactedDoc.GetPageObjects(0);
    QVERIFY(objs.has_value());
    bool hasImage = false;
    for (const auto& obj : objs.value) {
        if (obj.type == pdf_api::PageObjectType::Image) {
            hasImage = true;
            break;
        }
    }
    // Entire image was purged!
    QVERIFY(!hasImage);
}

void TestRedactionVerification::testAdversarialMetadataSanitization() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "sanitized_all.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    pdf_api::RedactionMark m;
    m.id = "m-dummy";
    m.pageIndex = 0;
    m.rect = {50.0, 710.0, 100.0, 20.0};
    service.AddRedactionMark(m);

    pdf_api::SanitizationOptions opt;
    opt.scrubInfoMetadata = true;
    opt.scrubXmpMetadata = true;

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf, opt);
    QVERIFY(repRes.has_value());

    // Inspect raw output
    std::ifstream in(dstPdf, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(bytes.find("Agent Zero") == std::string::npos);
    QVERIFY(bytes.find("Classified Operations Manual") == std::string::npos);
    QVERIFY(bytes.find("Top Secret Eyes Only") == std::string::npos);
}

void TestRedactionVerification::testExitGateUnsupportedContentPreventsSuccess() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "unsupported_out.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    // Mark on Page 2 covering the nested FormXObject
    pdf_api::RedactionMark mForm;
    mForm.id = "form-mark";
    mForm.pageIndex = 1;
    mForm.rect = {40.0, 580.0, 160.0, 60.0};
    service.AddRedactionMark(mForm);

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf);
    QVERIFY(repRes.has_value());

    // =========================================================================
    // VERIFICATION CRITERION: Any unsupported content prevents a success claim
    // and is disclosed!
    // =========================================================================
    const auto& report = repRes.value;
    QVERIFY(report.hasUnsupportedContent);
    QVERIFY(!report.isSuccess); // Success claim MUST BE PREVENTED!
    QVERIFY(!report.warnings.empty());

    // Verify warning mentions Form XObject
    bool foundFormWarning = false;
    for (const auto& w : report.warnings) {
        if (w.find("Form XObject") != std::string::npos) {
            foundFormWarning = true;
            break;
        }
    }
    QVERIFY(foundFormWarning);
}

void TestRedactionVerification::testSourcePreservationHashUnchanged() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "redacted_copy.pdf";

    std::string hashBefore = ComputeFileSha256(srcPdf);
    QVERIFY(!hashBefore.empty());

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto& service = services::RedactionService::Instance();
    service.ClearRedactionMarks();

    pdf_api::RedactionMark m;
    m.id = "m-save";
    m.pageIndex = 0;
    m.rect = {50.0, 710.0, 200.0, 30.0};
    service.AddRedactionMark(m);

    auto repRes = service.ApplyRedactionsAndSave(doc, srcPdf, dstPdf);
    QVERIFY(repRes.has_value());

    // Source document MUST remain completely unmodified
    std::string hashAfter = ComputeFileSha256(srcPdf);
    QCOMPARE(hashBefore, hashAfter);
}

QTEST_MAIN(TestRedactionVerification)
#include "test_redaction_verification.moc"
