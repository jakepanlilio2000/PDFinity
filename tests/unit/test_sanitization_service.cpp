// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/SanitizationService.hpp"

using namespace pdfinity;

class TestSanitizationService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testScrubInfoMetadata();
    void testScrubXmpMetadata();
    void testRemoveJavaScript();
    void testSanitizationProfiles();

private:
    std::filesystem::path m_fixturesDir;
};

void TestSanitizationService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestSanitizationService::testScrubInfoMetadata() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "redaction_adversarial.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "sanitized_info.pdf";

    pdf_api::SanitizationOptions opt;
    opt.scrubInfoMetadata = true;
    opt.scrubXmpMetadata = false;

    auto res = services::SanitizationService::ScrubMetadataAndRebuild(srcPdf, dstPdf, opt);
    QVERIFY(res.has_value());
    QVERIFY(std::filesystem::exists(dstPdf));

    // Verify raw content has scrubbed metadata
    std::ifstream in(dstPdf, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Original author was "Agent Zero"
    QVERIFY(bytes.find("Agent Zero") == std::string::npos);
    // Original title was "Classified Operations Manual"
    QVERIFY(bytes.find("Classified Operations Manual") == std::string::npos);
}

void TestSanitizationService::testScrubXmpMetadata() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto srcPdf = m_fixturesDir / "ordinary.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "sanitized_xmp.pdf";

    pdf_api::SanitizationOptions opt;
    opt.scrubInfoMetadata = true;
    opt.scrubXmpMetadata = true;

    auto res = services::SanitizationService::ScrubMetadataAndRebuild(srcPdf, dstPdf, opt);
    QVERIFY(res.has_value());

    std::ifstream in(dstPdf, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Verify no /Metadata stream reference
    QVERIFY(bytes.find("/Metadata ") == std::string::npos);
    QVERIFY(bytes.find("<x:xmpmeta") == std::string::npos);
}

void TestSanitizationService::testRemoveJavaScript() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Create synthetic PDF with JavaScript action
    std::string jsPdf =
        "%PDF-1.7\r\n"
        "1 0 obj << /Type /Catalog /Pages 2 0 R /OpenAction << /S /JavaScript /JS (app.alert('Malicious');) >> >> endobj\r\n"
        "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\r\n"
        "3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >> endobj\r\n"
        "xref\r\n0 4\r\n0000000000 65535 f \r\n0000000010 00000 n \r\n0000000100 00000 n \r\n0000000150 00000 n \r\n"
        "trailer << /Size 4 /Root 1 0 R >>\r\nstartxref\r\n220\r\n%%EOF\r\n";

    auto srcPdf = std::filesystem::path(tempDir.path().toStdString()) / "js_doc.pdf";
    auto dstPdf = std::filesystem::path(tempDir.path().toStdString()) / "sanitized_js.pdf";

    std::ofstream out(srcPdf, std::ios::binary);
    out.write(jsPdf.data(), jsPdf.size());
    out.close();

    pdf_api::SanitizationOptions opt;
    opt.removeJavaScript = true;

    auto res = services::SanitizationService::ScrubMetadataAndRebuild(srcPdf, dstPdf, opt);
    QVERIFY(res.has_value());

    std::ifstream in(dstPdf, std::ios::binary);
    std::string sanitized((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    QVERIFY(sanitized.find("/S /JavaScript") == std::string::npos);
    QVERIFY(sanitized.find("app.alert") == std::string::npos);
}

void TestSanitizationService::testSanitizationProfiles() {
    pdf_api::SanitizationOptions optStd;
    optStd.profile = pdf_api::SanitizationProfile::StandardSanitize;
    QVERIFY(optStd.scrubInfoMetadata);
    QVERIFY(optStd.removeAttachments);
    QVERIFY(optStd.removeJavaScript);
    QVERIFY(!optStd.removeHiddenLayers);

    pdf_api::SanitizationOptions optHigh;
    optHigh.profile = pdf_api::SanitizationProfile::HighAssuranceClean;
    optHigh.removeHiddenLayers = true;
    optHigh.removeAllAnnotations = true;
    QVERIFY(optHigh.removeHiddenLayers);
    QVERIFY(optHigh.removeAllAnnotations);
}

QTEST_MAIN(TestSanitizationService)
#include "test_sanitization_service.moc"
