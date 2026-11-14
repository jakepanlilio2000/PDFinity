// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include <fstream>
#include "services/BatchProcessingService.hpp"
#include "services/BatchProfileManager.hpp"
#include "services/BatchJobHistory.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestBatchService : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testResolveOutputPathAndCollision();
    void testCollectInputFiles();
    void testExecuteBatchWorkflowMultiFile();
    void testErrorIsolationPerFile();
    void testSafeResumeExistingOutputs();
    void testSummaryReportExport();
    void testSourceImmutabilityProtection();
    void testFlattenFormsBatchOperation();
    void testMemoryLimitEnforcement();
    void testUnsupportedOperationFailsExplicitly();

private:
    std::filesystem::path m_fixturesDir;
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

void TestBatchService::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));

    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    QVERIFY(m_engine->Initialize());
    app::AppContext::Instance().Initialize();
}

void TestBatchService::cleanupTestCase() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

void TestBatchService::testResolveOutputPathAndCollision() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());
    auto inPath = m_fixturesDir / "ordinary.pdf";

    // 1. Initial resolution when target does not exist
    auto p1 = services::BatchProcessingService::ResolveOutputPath(
        inPath, outDir, "{basename}_processed.pdf", domain::CollisionPolicy::AutoRename
    );
    QCOMPARE(QString::fromStdString(p1.filename().string()), QString("ordinary_processed.pdf"));

    // Create dummy existing file at p1
    {
        std::ofstream out(p1);
        out << "%PDF-dummy";
    }
    QVERIFY(std::filesystem::exists(p1));

    // 2. AutoRename should produce ordinary_processed_1.pdf
    auto p2 = services::BatchProcessingService::ResolveOutputPath(
        inPath, outDir, "{basename}_processed.pdf", domain::CollisionPolicy::AutoRename
    );
    QCOMPARE(QString::fromStdString(p2.filename().string()), QString("ordinary_processed_1.pdf"));

    // 3. Overwrite should keep original target
    auto p3 = services::BatchProcessingService::ResolveOutputPath(
        inPath, outDir, "{basename}_processed.pdf", domain::CollisionPolicy::Overwrite
    );
    QCOMPARE(QString::fromStdString(p3.filename().string()), QString("ordinary_processed.pdf"));
}

void TestBatchService::testCollectInputFiles() {
    auto files = services::BatchProcessingService::CollectInputFiles({m_fixturesDir}, false);
    QVERIFY(!files.empty());
    for (const auto& f : files) {
        QCOMPARE(QString::fromStdString(f.extension().string()).toLower(), QString(".pdf"));
    }
}

void TestBatchService::testExecuteBatchWorkflowMultiFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto profileOpt = services::BatchProfileManager::Instance().FindProfile("builtin_web_optimize");
    QVERIFY(profileOpt.has_value());

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Multi-File Web Optimization";
    cfg.profile = profileOpt.value();
    cfg.inputFiles = {
        m_fixturesDir / "ordinary.pdf",
        m_fixturesDir / "two_column_text.pdf"
    };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_opt.pdf";
    cfg.collisionPolicy = domain::CollisionPolicy::Overwrite;
    cfg.maxConcurrency = 2;

    int progressHits = 0;
    auto progressCb = [&](int /*cur*/, int /*tot*/, const domain::BatchFileItem& /*item*/) {
        progressHits++;
    };

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg, progressCb);
    QCOMPARE(summary.state, domain::BatchJobState::Succeeded);
    QCOMPARE(summary.totalFiles, 2);
    QCOMPARE(summary.succeededFiles, 2);
    QCOMPARE(summary.failedFiles, 0);
    QVERIFY(progressHits > 0);

    // Verify output files exist and are readable by engine
    for (const auto& item : summary.fileItems) {
        QVERIFY(std::filesystem::exists(item.outputPath));
        QVERIFY(item.outputSizeBytes > 0);
        auto testOpen = m_engine->OpenDocument(item.outputPath);
        QVERIFY(testOpen.has_value());
        m_engine->CloseDocument(testOpen.value);
    }
}

void TestBatchService::testErrorIsolationPerFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto profileOpt = services::BatchProfileManager::Instance().FindProfile("builtin_web_optimize");
    QVERIFY(profileOpt.has_value());

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Error Isolation";
    cfg.profile = profileOpt.value();
    cfg.inputFiles = {
        m_fixturesDir / "ordinary.pdf",
        m_fixturesDir / "malformed_corrupt_header.pdf", // Should fail safely without crashing
        m_fixturesDir / "two_column_text.pdf"
    };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_out.pdf";
    cfg.collisionPolicy = domain::CollisionPolicy::Overwrite;
    cfg.maxRetries = 0;

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    QCOMPARE(summary.totalFiles, 3);
    QCOMPARE(summary.succeededFiles, 2);
    QCOMPARE(summary.failedFiles, 1);

    // Find the malformed file result
    bool foundFailedItem = false;
    for (const auto& item : summary.fileItems) {
        if (item.inputPath.filename().string() == "malformed_corrupt_header.pdf") {
            foundFailedItem = true;
            QCOMPARE(item.status, domain::BatchFileStatus::Error);
            QVERIFY(!item.errorMessage.empty());
            QVERIFY(!item.logs.empty());
        } else {
            QCOMPARE(item.status, domain::BatchFileStatus::Success);
        }
    }
    QVERIFY(foundFailedItem);
}

void TestBatchService::testSafeResumeExistingOutputs() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    auto profileOpt = services::BatchProfileManager::Instance().FindProfile("builtin_web_optimize");
    QVERIFY(profileOpt.has_value());

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Safe Resume";
    cfg.profile = profileOpt.value();
    cfg.inputFiles = { m_fixturesDir / "ordinary.pdf" };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_resumed.pdf";
    cfg.collisionPolicy = domain::CollisionPolicy::Overwrite;
    cfg.resumeExistingOutputs = true;

    // Run 1: Should execute and succeed
    auto summary1 = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    QCOMPARE(summary1.succeededFiles, 1);
    QCOMPARE(summary1.skippedFiles, 0);

    // Run 2: With resumeExistingOutputs = true, should detect valid output and skip
    auto summary2 = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    QCOMPARE(summary2.succeededFiles, 0);
    QCOMPARE(summary2.skippedFiles, 1);
    QCOMPARE(summary2.fileItems[0].status, domain::BatchFileStatus::Skipped);
}

void TestBatchService::testSummaryReportExport() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto reportHtml = std::filesystem::path(tempDir.path().toStdString()) / "summary_report.html";
    auto reportJson = std::filesystem::path(tempDir.path().toStdString()) / "summary_report.json";

    domain::BatchJobSummary summary;
    summary.jobId = "job_test_report_123";
    summary.jobName = "Test Summary Report Job";
    summary.profileName = "PDF/A Archival";
    summary.state = domain::BatchJobState::Succeeded;
    summary.totalFiles = 2;
    summary.succeededFiles = 2;
    summary.totalInputBytes = 2048;
    summary.totalOutputBytes = 1024;
    summary.totalDurationMs = 1500;

    domain::BatchFileItem item;
    item.inputPath = "fileA.pdf";
    item.outputPath = "fileA_processed.pdf";
    item.status = domain::BatchFileStatus::Success;
    item.durationMs = 750;
    summary.fileItems.push_back(item);

    QVERIFY(services::BatchJobHistory::ExportSummaryToHtml(summary, reportHtml));
    QVERIFY(std::filesystem::exists(reportHtml));
    QVERIFY(std::filesystem::file_size(reportHtml) > 100);

    QVERIFY(services::BatchJobHistory::ExportSummaryToJson(summary, reportJson));
    QVERIFY(std::filesystem::exists(reportJson));
    QVERIFY(std::filesystem::file_size(reportJson) > 50);
}

void TestBatchService::testSourceImmutabilityProtection() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dir = std::filesystem::path(tempDir.path().toStdString());
    auto sourcePath = dir / "test_doc.pdf";

    // Copy fixture to temporary sourcePath
    std::filesystem::copy_file(m_fixturesDir / "ordinary.pdf", sourcePath);
    auto originalSize = std::filesystem::file_size(sourcePath);

    // 1. ResolveOutputPath when target is configured to same directory and name
    auto resolved = services::BatchProcessingService::ResolveOutputPath(
        sourcePath, dir, "{basename}.pdf", domain::CollisionPolicy::Overwrite
    );
    // Should NOT be sourcePath; must append safe suffix
    QVERIFY(resolved != sourcePath);
    QVERIFY(QString::fromStdString(resolved.filename().string()).contains("_processed"));

    // 2. Direct violation test: Attempt to run a batch job where output is explicitly set to sourcePath
    domain::BatchProfile prof;
    prof.id = "test_opt";
    prof.name = "Test Opt";
    domain::BatchOperation op;
    op.type = domain::BatchOperationType::Optimize;
    prof.operations.push_back(op);

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Source Immutability";
    cfg.profile = prof;
    cfg.inputFiles = { sourcePath };
    cfg.outputDirectory = dir;
    cfg.filenameTemplate = "{basename}.pdf";
    cfg.collisionPolicy = domain::CollisionPolicy::Overwrite;

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    // File at original path must be preserved and unchanged
    QVERIFY(std::filesystem::exists(sourcePath));
    QCOMPARE(std::filesystem::file_size(sourcePath), originalSize);
}

void TestBatchService::testFlattenFormsBatchOperation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    domain::BatchProfile prof;
    prof.id = "test_flatten";
    prof.name = "Test Form Flattening";
    domain::BatchOperation op;
    op.type = domain::BatchOperationType::FlattenForms;
    op.name = "Flatten AcroForms";
    op.parameters["forPrint"] = "false";
    prof.operations.push_back(op);

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Flattening Batch";
    cfg.profile = prof;
    cfg.inputFiles = { m_fixturesDir / "acroform_sample.pdf" };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_flattened.pdf";
    cfg.collisionPolicy = domain::CollisionPolicy::Overwrite;

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    QCOMPARE(summary.state, domain::BatchJobState::Succeeded);
    QCOMPARE(summary.succeededFiles, 1);

    auto outFile = outDir / "acroform_sample_flattened.pdf";
    QVERIFY(std::filesystem::exists(outFile));
    QVERIFY(std::filesystem::file_size(outFile) > 0);

    // Verify output document can be opened by engine
    auto testOpen = m_engine->OpenDocument(outFile);
    QVERIFY(testOpen.has_value());
    m_engine->CloseDocument(testOpen.value);
}

void TestBatchService::testMemoryLimitEnforcement() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    domain::BatchProfile prof;
    prof.id = "test_mem";
    prof.name = "Test Memory Limit";
    domain::BatchOperation op;
    op.type = domain::BatchOperationType::Optimize;
    prof.operations.push_back(op);

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Memory Budget Job";
    cfg.profile = prof;
    cfg.inputFiles = { m_fixturesDir / "ordinary.pdf" };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_out.pdf";
    cfg.maxFileSizeBytes = 500; // 500 bytes limit -> ordinary.pdf (912 bytes) exceeds limit

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    // Should fail file with memory limit exceeded
    QCOMPARE(summary.failedFiles, 1);
    QCOMPARE(summary.fileItems[0].status, domain::BatchFileStatus::Error);
    QVERIFY(QString::fromStdString(summary.fileItems[0].errorMessage).contains("exceeds memory/size limit"));
}

void TestBatchService::testUnsupportedOperationFailsExplicitly() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());

    domain::BatchProfile prof;
    prof.id = "test_unsupported";
    prof.name = "Test Unsupported Op";
    domain::BatchOperation op;
    op.type = domain::BatchOperationType::WatermarkStamp; // Currently unsupported
    op.name = "Watermark Stamp";
    prof.operations.push_back(op);

    domain::BatchJobConfig cfg;
    cfg.jobName = "Test Unsupported Job";
    cfg.profile = prof;
    cfg.inputFiles = { m_fixturesDir / "ordinary.pdf" };
    cfg.outputDirectory = outDir;
    cfg.filenameTemplate = "{basename}_wm.pdf";
    cfg.maxRetries = 0;

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(cfg);
    // Must NOT fake success
    QCOMPARE(summary.failedFiles, 1);
    QCOMPARE(summary.fileItems[0].status, domain::BatchFileStatus::Error);
    QVERIFY(QString::fromStdString(summary.fileItems[0].errorMessage).contains("not supported"));
}

QTEST_MAIN(TestBatchService)
#include "test_batch_service.moc"
