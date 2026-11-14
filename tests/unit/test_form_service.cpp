// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "services/FormService.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"

using namespace pdfinity;
using namespace pdfinity::services;

class TestFormService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInputSanitization();
    void testInputLengthLimit();
    void testJsonExportAndImport();
    void testMalformedJsonHandling();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestFormService::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestFormService::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestFormService::testInputSanitization() {
    std::string sanitized;
    std::string err;

    // Normal text with whitespace
    std::string normal = "Valid Address Line 1\nLine 2\tSuite 400\r\n";
    QVERIFY(FormService::ValidateTextInput(normal, sanitized, err));
    QCOMPARE(QString::fromStdString(sanitized), QString::fromStdString(normal));

    // Input containing illegal control characters and null bytes
    using namespace std::string_literals;
    std::string hostile = "User\0Name\aWith\033Escapes"s;
    QVERIFY(FormService::ValidateTextInput(hostile, sanitized, err));
    // Null byte (\0), Bell (\a), and Escape (\033) must be stripped
    QCOMPARE(QString::fromStdString(sanitized), QString("UserNameWithEscapes"));
}

void TestFormService::testInputLengthLimit() {
    std::string sanitized;
    std::string err;

    // 64 KB is allowed
    std::string valid64k(65536, 'A');
    QVERIFY(FormService::ValidateTextInput(valid64k, sanitized, err));

    // 64 KB + 1 byte must be rejected
    std::string over64k(65537, 'B');
    QVERIFY(!FormService::ValidateTextInput(over64k, sanitized, err));
    QVERIFY(!err.empty());
}

void TestFormService::testJsonExportAndImport() {
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    FormService service;
    std::string jsonStr = service.ExportFormDataToJson(doc);
    QVERIFY(!jsonStr.empty());
    QVERIFY(jsonStr.find("\"FullName\"") != std::string::npos);
    QVERIFY(jsonStr.find("\"SubscribeNewsletter\"") != std::string::npos);
    QVERIFY(jsonStr.find("\"Country\"") != std::string::npos);

    // Import modified JSON
    std::string modifiedJson = R"({
        "fields": [
            {
                "name": "FullName",
                "type": "TextField",
                "value": "Imported Commander"
            },
            {
                "name": "SubscribeNewsletter",
                "type": "CheckBox",
                "isChecked": false
            },
            {
                "name": "Country",
                "type": "ComboBox",
                "value": "Japan"
            }
        ]
    })";

    auto importRes = service.ImportFormDataFromJson(doc, modifiedJson);
    QVERIFY(importRes.has_value());
    QCOMPARE(importRes.value, 3);

    // Verify fields updated in model
    auto fieldsRes = doc.GetAllFormFields();
    QVERIFY(fieldsRes.has_value());
    for (const auto& f : fieldsRes.value) {
        if (f.name == "FullName") {
            QCOMPARE(QString::fromStdString(f.value), QString("Imported Commander"));
        } else if (f.name == "SubscribeNewsletter") {
            QVERIFY(!f.isChecked);
        } else if (f.name == "Country") {
            QCOMPARE(QString::fromStdString(f.value), QString("Japan"));
        }
    }
}

void TestFormService::testMalformedJsonHandling() {
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    FormService service;

    // Malformed JSON syntax
    auto res1 = service.ImportFormDataFromJson(doc, "{ not a valid json: [");
    QVERIFY(!res1.has_value());
    QCOMPARE(res1.error, pdf_api::EngineError::InvalidFormat);

    // JSON without "fields" array
    auto res2 = service.ImportFormDataFromJson(doc, R"({ "otherKey": 123 })");
    QVERIFY(!res2.has_value());
    QCOMPARE(res2.error, pdf_api::EngineError::InvalidFormat);
}

QTEST_MAIN(TestFormService)
#include "test_form_service.moc"
