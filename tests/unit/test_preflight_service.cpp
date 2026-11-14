// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <filesystem>
#include "services/PreflightService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "security/SecureString.hpp"

using namespace pdfinity;

class TestPreflightService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPreflightOnOrdinaryDocument();
    void testPreflightEncryptionDetection();
    void testPreflightDisclaimerAndHeuristicDistinction();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestPreflightService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestPreflightService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestPreflightService::testPreflightOnOrdinaryDocument() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    services::PreflightOptions opt;
    opt.requireAllFontsEmbedded = false; // Warnings only for standard doc
    auto res = services::PreflightService::Instance().RunPreflight(doc, opt);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(!report.summary.empty());
    QVERIFY(!report.disclaimer.empty());
}

void TestPreflightService::testPreflightEncryptionDetection() {
    auto srcPdf = m_fixturesDir / "encrypted_user_pass.pdf";
    // Authenticate with user password "testpass123"
    auto openRes = m_engine->OpenDocument(srcPdf, security::SecureString("testpass123"));
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::PreflightService::Instance().RunPreflight(doc);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    bool foundEncryptionFinding = false;
    for (const auto& f : report.findings) {
        if (f.category == pdf_api::PreflightCategory::Encryption) {
            foundEncryptionFinding = true;
            QVERIFY(!f.remediation.empty());
        }
    }
    QVERIFY(foundEncryptionFinding);
}

void TestPreflightService::testPreflightDisclaimerAndHeuristicDistinction() {
    auto srcPdf = m_fixturesDir / "image_heavy.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::PreflightService::Instance().RunPreflight(doc);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    // Verification: Reports distinguish standards validation from best-effort inspection!
    QVERIFY(report.disclaimer.find("Passing automated preflight does not constitute a formal warranty") != std::string::npos);

    bool hasHeuristic = false;
    for (const auto& f : report.findings) {
        if (f.isHeuristic) hasHeuristic = true;
    }
    QVERIFY(hasHeuristic);
}

QTEST_MAIN(TestPreflightService)
#include "test_preflight_service.moc"
