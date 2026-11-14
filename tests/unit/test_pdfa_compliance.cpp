// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include "services/PdfAComplianceService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "security/SecureString.hpp"

using namespace pdfinity;

class TestPdfACompliance : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEncryptedDocumentFailsPdfA();
    void testStandardDocumentValidationAndRuleReporting();
    void testConvertToPdfAWorkflow();
    void testDisclaimerDistinction();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestPdfACompliance::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestPdfACompliance::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestPdfACompliance::testEncryptedDocumentFailsPdfA() {
    auto srcPdf = m_fixturesDir / "encrypted_user_pass.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf, security::SecureString("testpass123"));
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::PdfAComplianceService::Instance().ValidateCompliance(doc, pdf_api::PdfAProfile::PdfA1b);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    // ISO 19005 clause 6.1.4: Encryption is strictly prohibited in PDF/A!
    QVERIFY(!report.isCompliant);

    bool encryptionCheckFailed = false;
    for (const auto& r : report.ruleChecks) {
        if (r.clause.find("6.1.4") != std::string::npos && !r.passed) {
            encryptionCheckFailed = true;
        }
    }
    QVERIFY(encryptionCheckFailed);
}

void TestPdfACompliance::testStandardDocumentValidationAndRuleReporting() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::PdfAComplianceService::Instance().ValidateCompliance(doc, pdf_api::PdfAProfile::PdfA1b);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(!report.ruleChecks.empty());

    for (const auto& r : report.ruleChecks) {
        QVERIFY(!r.ruleId.empty());
        QVERIFY(!r.clause.empty());
        QVERIFY(!r.description.empty());
    }
}

void TestPdfACompliance::testConvertToPdfAWorkflow() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "converted_pdfa.pdf";

    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    pdf_api::PdfAConversionOptions opt;
    opt.targetProfile = pdf_api::PdfAProfile::PdfA1b;
    opt.embedMissingMetadata = true;
    opt.stripProhibitedActions = true;
    opt.injectOutputIntent = true;

    auto res = services::PdfAComplianceService::Instance().ConvertToPdfA(doc, dstPdf, opt);
    QVERIFY(res.has_value());
    QVERIFY(std::filesystem::exists(dstPdf));

    // Reopen converted PDF and verify OutputIntent and XMP identification
    auto reopen = m_engine->OpenDocument(dstPdf);
    QVERIFY(reopen.has_value());
    domain::DocumentModel convertedDoc(reopen.value, m_engine, dstPdf);

    auto valRes = services::PdfAComplianceService::Instance().ValidateCompliance(convertedDoc, pdf_api::PdfAProfile::PdfA1b);
    QVERIFY(valRes.has_value());

    // Rule 3 (XMP schema) and Rule 4 (OutputIntent) must pass!
    bool xmpPassed = false;
    bool outputIntentPassed = false;
    for (const auto& r : valRes.value.ruleChecks) {
        if (r.clause.find("6.7") != std::string::npos && r.passed) xmpPassed = true;
        if (r.clause.find("6.2.3") != std::string::npos && r.passed) outputIntentPassed = true;
    }
    QVERIFY(xmpPassed);
    QVERIFY(outputIntentPassed);
}

void TestPdfACompliance::testDisclaimerDistinction() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::PdfAComplianceService::Instance().ValidateCompliance(doc, pdf_api::PdfAProfile::PdfA1b);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    // Verification: Reports are reproducible and distinguish standards validation from best-effort inspection.
    QVERIFY(report.disclaimer.find("Standards Validation Disclaimer") != std::string::npos);
    QVERIFY(report.disclaimer.find("does not constitute a certified legal warranty") != std::string::npos);
}

QTEST_MAIN(TestPdfACompliance)
#include "test_pdfa_compliance.moc"
