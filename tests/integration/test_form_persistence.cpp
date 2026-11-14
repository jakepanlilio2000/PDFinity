// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>

using namespace pdfinity;

class TestFormPersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDetectAcroForm();
    void testInspectFormFields();
    void testFillAndSaveCopyPersistence();
    void testFormResetToDefaults();
    void testFormFlattening();
    void testDetectXfaForm();

private:
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::filesystem::path m_fixturesDir;
};

void TestFormPersistence::initTestCase() {
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestFormPersistence::cleanupTestCase() {
    m_engine->Shutdown();
    m_engine.reset();
}

void TestFormPersistence::testDetectAcroForm() {
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto typeRes = doc.GetFormType();
    QVERIFY(typeRes.has_value());
    QCOMPARE(typeRes.value, pdf_api::FormType::AcroForm);
    QVERIFY(doc.HasForm());
    QVERIFY(!doc.HasXfa());
}

void TestFormPersistence::testInspectFormFields() {
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto fieldsRes = doc.GetAllFormFields();
    QVERIFY(fieldsRes.has_value());
    QVERIFY(fieldsRes.value.size() >= 4);

    bool foundName = false;
    bool foundCheck = false;
    bool foundCountry = false;

    for (const auto& f : fieldsRes.value) {
        if (f.name == "FullName") {
            foundName = true;
            QCOMPARE(f.type, pdf_api::FormFieldType::TextField);
            QCOMPARE(QString::fromStdString(f.value), QString("Jane Doe"));
            QCOMPARE(QString::fromStdString(f.defaultValue), QString("Default Name"));
        } else if (f.name == "SubscribeNewsletter") {
            foundCheck = true;
            QCOMPARE(f.type, pdf_api::FormFieldType::CheckBox);
            QVERIFY(f.isChecked);
        } else if (f.name == "Country") {
            foundCountry = true;
            QCOMPARE(f.type, pdf_api::FormFieldType::ComboBox);
            QCOMPARE(QString::fromStdString(f.value), QString("Canada"));
            QVERIFY(f.options.size() >= 5);
        }
    }

    QVERIFY(foundName);
    QVERIFY(foundCheck);
    QVERIFY(foundCountry);
}

void TestFormPersistence::testFillAndSaveCopyPersistence() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "filled_form.pdf";

    auto origPath = m_fixturesDir / "acroform_sample.pdf";
    uint64_t origSize = std::filesystem::file_size(origPath);

    // Open original
    auto openRes = m_engine->OpenDocument(origPath);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, origPath);

    // Find indices
    auto fieldsRes = doc.GetAllFormFields();
    QVERIFY(fieldsRes.has_value());

    for (const auto& f : fieldsRes.value) {
        if (f.name == "FullName") {
            QVERIFY(doc.SetFormFieldValue(f.pageIndex, f.annotIndex, "Dr. Elizabeth Bowman").has_value());
        } else if (f.name == "SubscribeNewsletter") {
            QVERIFY(doc.SetFormFieldChecked(f.pageIndex, f.annotIndex, false).has_value());
        } else if (f.name == "Country") {
            // Find option index for "Germany"
            int optIdx = -1;
            for (size_t o = 0; o < f.options.size(); ++o) {
                if (f.options[o] == "Germany") {
                    optIdx = static_cast<int>(o);
                    break;
                }
            }
            QVERIFY(optIdx >= 0);
            QVERIFY(doc.SetFormFieldOption(f.pageIndex, f.annotIndex, optIdx).has_value());
        }
    }

    // Save copy
    QVERIFY(doc.SaveCopy(outPath).has_value());
    QVERIFY(std::filesystem::exists(outPath));

    // Immutability check: original file must not be modified
    uint64_t origSizeAfter = std::filesystem::file_size(origPath);
    QCOMPARE(origSize, origSizeAfter);

    // Reopen in clean engine instance
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel savedDoc(reopenRes.value, m_engine, outPath);

    auto savedFieldsRes = savedDoc.GetAllFormFields();
    QVERIFY(savedFieldsRes.has_value());

    bool verifiedName = false;
    bool verifiedCheck = false;
    bool verifiedCountry = false;

    for (const auto& f : savedFieldsRes.value) {
        if (f.name == "FullName") {
            QCOMPARE(QString::fromStdString(f.value), QString("Dr. Elizabeth Bowman"));
            verifiedName = true;
        } else if (f.name == "SubscribeNewsletter") {
            QVERIFY(!f.isChecked);
            verifiedCheck = true;
        } else if (f.name == "Country") {
            QCOMPARE(QString::fromStdString(f.value), QString("Germany"));
            verifiedCountry = true;
        }
    }

    QVERIFY(verifiedName);
    QVERIFY(verifiedCheck);
    QVERIFY(verifiedCountry);
}

void TestFormPersistence::testFormResetToDefaults() {
    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    auto fieldsRes = doc.GetAllFormFields();
    QVERIFY(fieldsRes.has_value());

    // Modify text field
    for (const auto& f : fieldsRes.value) {
        if (f.name == "FullName") {
            doc.SetFormFieldValue(f.pageIndex, f.annotIndex, "Temporary Overwrite");
        }
    }

    // Reset form
    QVERIFY(doc.ResetFormFields().has_value());

    // Verify restored to default
    auto resetFields = doc.GetAllFormFields();
    QVERIFY(resetFields.has_value());
    for (const auto& f : resetFields.value) {
        if (f.name == "FullName") {
            QCOMPARE(QString::fromStdString(f.value), QString("Default Name"));
        }
    }
}

void TestFormPersistence::testFormFlattening() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outPath = std::filesystem::path(tempDir.path().toStdString()) / "flattened_form.pdf";

    auto path = m_fixturesDir / "acroform_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    // Verify fields exist initially
    auto initialFields = doc.GetAllFormFields();
    QVERIFY(initialFields.has_value() && !initialFields.value.empty());

    // Flatten document
    QVERIFY(doc.FlattenDocument().has_value());
    QVERIFY(doc.SaveCopy(outPath).has_value());

    // Reopen flattened document
    auto reopenRes = m_engine->OpenDocument(outPath);
    QVERIFY(reopenRes.has_value());
    domain::DocumentModel flatDoc(reopenRes.value, m_engine, outPath);

    // After flattening, interactive fields must be converted to static content
    auto flatFields = flatDoc.GetAllFormFields();
    QVERIFY(flatFields.has_value());
    QCOMPARE(flatFields.value.size(), 0);

    // Render test: flattened page must render cleanly
    pdf_api::RenderOptions opts;
    opts.scale = 1.0;
    opts.dpi = 72;
    auto renderRes = flatDoc.RenderPage(0, opts);
    QVERIFY(renderRes.has_value());
    QVERIFY(!renderRes.value.pixels.empty());
}

void TestFormPersistence::testDetectXfaForm() {
    auto path = m_fixturesDir / "xfa_sample.pdf";
    auto openRes = m_engine->OpenDocument(path);
    QVERIFY(openRes.has_value());
    domain::DocumentModel doc(openRes.value, m_engine, path);

    QVERIFY(doc.HasXfa());
    auto typeRes = doc.GetFormType();
    QVERIFY(typeRes.has_value());
    QVERIFY(typeRes.value == pdf_api::FormType::XfaFull || typeRes.value == pdf_api::FormType::XfaForeground);
}

QTEST_MAIN(TestFormPersistence)
#include "test_form_persistence.moc"
