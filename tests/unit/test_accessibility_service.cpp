// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <filesystem>
#include "services/AccessibilityService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;

class TestAccessibilityService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInspectUntaggedDocument();
    void testActionableRemediationAndWCAGClauses();
    void testLegalDisclaimerPreserved();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestAccessibilityService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
}

void TestAccessibilityService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestAccessibilityService::testInspectUntaggedDocument() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::AccessibilityService::Instance().InspectAccessibility(doc);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(!report.isTagged);

    bool foundTaggedFinding = false;
    for (const auto& f : report.findings) {
        if (f.ruleId.find("WCAG-4.1.2") != std::string::npos || f.ruleId.find("PDF/UA") != std::string::npos) {
            foundTaggedFinding = true;
            QVERIFY(!f.remediation.empty());
        }
    }
    QVERIFY(foundTaggedFinding);
}

void TestAccessibilityService::testActionableRemediationAndWCAGClauses() {
    auto srcPdf = m_fixturesDir / "image_heavy.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::AccessibilityService::Instance().InspectAccessibility(doc);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    QVERIFY(!report.findings.empty());

    for (const auto& f : report.findings) {
        QVERIFY(!f.ruleId.empty());
        QVERIFY(!f.issue.empty());
        QVERIFY(!f.remediation.empty());
    }
}

void TestAccessibilityService::testLegalDisclaimerPreserved() {
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto openRes = m_engine->OpenDocument(srcPdf);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, srcPdf);

    auto res = services::AccessibilityService::Instance().InspectAccessibility(doc);
    QVERIFY(res.has_value());

    const auto& report = res.value;
    // Verification: Never claim legal compliance based only on heuristic checks!
    QVERIFY(report.legalDisclaimer.find("Accessibility Evaluation Notice") != std::string::npos);
    QVERIFY(report.legalDisclaimer.find("Passing automated checks does not confer a formal legal warranty") != std::string::npos);
}

QTEST_MAIN(TestAccessibilityService)
#include "test_accessibility_service.moc"
