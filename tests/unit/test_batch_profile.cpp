// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include "services/BatchProfileManager.hpp"

using namespace pdfinity;

class TestBatchProfile : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testBuiltInProfilesPresence();
    void testFindProfileByNameAndId();
    void testCreateSaveAndDeleteCustomProfile();
    void testProfileJsonExportAndImport();

private:
    std::filesystem::path m_fixturesDir;
};

void TestBatchProfile::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
}

void TestBatchProfile::cleanupTestCase() {
    services::BatchProfileManager::Instance().ResetToDefaults();
}

void TestBatchProfile::testBuiltInProfilesPresence() {
    auto& mgr = services::BatchProfileManager::Instance();
    auto builtIns = mgr.GetBuiltInProfiles();
    QVERIFY(builtIns.size() >= 5);

    bool hasPdfA = false;
    bool hasWebOpt = false;
    bool hasOcr = false;

    for (const auto& p : builtIns) {
        QVERIFY(p.isBuiltIn);
        QVERIFY(!p.id.empty());
        QVERIFY(!p.name.empty());
        QVERIFY(!p.description.empty());
        QVERIFY(!p.operations.empty());

        if (p.id == "builtin_pdfa_archival") hasPdfA = true;
        if (p.id == "builtin_web_optimize") hasWebOpt = true;
        if (p.id == "builtin_searchable_ocr") hasOcr = true;
    }

    QVERIFY(hasPdfA);
    QVERIFY(hasWebOpt);
    QVERIFY(hasOcr);
}

void TestBatchProfile::testFindProfileByNameAndId() {
    auto& mgr = services::BatchProfileManager::Instance();

    auto byId = mgr.FindProfile("builtin_web_optimize");
    QVERIFY(byId.has_value());
    QCOMPARE(QString::fromStdString(byId.value().id), QString("builtin_web_optimize"));

    auto byName = mgr.FindProfile(byId.value().name);
    QVERIFY(byName.has_value());
    QCOMPARE(QString::fromStdString(byName.value().id), QString("builtin_web_optimize"));

    auto byInvalid = mgr.FindProfile("non_existent_profile_xyz");
    QVERIFY(!byInvalid.has_value());
}

void TestBatchProfile::testCreateSaveAndDeleteCustomProfile() {
    auto& mgr = services::BatchProfileManager::Instance();

    domain::BatchProfile custom;
    custom.id = "custom_test_workflow";
    custom.name = "My Automated Legal Export";
    custom.description = "Convert to PDF/A and sanitize";
    custom.isBuiltIn = false;

    domain::BatchOperation op1;
    op1.type = domain::BatchOperationType::ConvertToPdfA;
    op1.name = "PDF/A-1b";
    op1.parameters = {{"profile", "PdfA1b"}};
    custom.operations.push_back(op1);

    domain::BatchOperation op2;
    op2.type = domain::BatchOperationType::Sanitize;
    op2.name = "Sanitize";
    op2.parameters = {{"profile", "Maximum"}};
    custom.operations.push_back(op2);

    QVERIFY(mgr.SaveCustomProfile(custom));

    // Verify retrieval
    auto retrieved = mgr.FindProfile("custom_test_workflow");
    QVERIFY(retrieved.has_value());
    QCOMPARE(QString::fromStdString(retrieved.value().name), QString("My Automated Legal Export"));
    QCOMPARE(retrieved.value().operations.size(), static_cast<size_t>(2));

    // Delete custom profile
    QVERIFY(mgr.DeleteCustomProfile("custom_test_workflow"));
    auto afterDelete = mgr.FindProfile("custom_test_workflow");
    QVERIFY(!afterDelete.has_value());
}

void TestBatchProfile::testProfileJsonExportAndImport() {
    auto& mgr = services::BatchProfileManager::Instance();
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto srcProfile = mgr.FindProfile("builtin_web_optimize");
    QVERIFY(srcProfile.has_value());

    auto jsonPath = std::filesystem::path(tempDir.path().toStdString()) / "web_opt_exported.json";
    QVERIFY(mgr.ExportProfileToJson(srcProfile.value(), jsonPath));
    QVERIFY(std::filesystem::exists(jsonPath));

    auto imported = mgr.ImportProfileFromJson(jsonPath);
    QVERIFY(imported.has_value());
    QCOMPARE(QString::fromStdString(imported.value().name), QString::fromStdString(srcProfile.value().name));
    QCOMPARE(imported.value().operations.size(), srcProfile.value().operations.size());

    // Clean up imported profile
    mgr.DeleteCustomProfile(imported.value().id);
}

QTEST_MAIN(TestBatchProfile)
#include "test_batch_profile.moc"
