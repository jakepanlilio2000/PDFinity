// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include "app/BatchCliHandler.hpp"
#include "application/AppContext.hpp"

using namespace pdfinity;

class TestBatchCli : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testIsBatchCommandDetection();
    void testBatchProfilesCommand();
    void testBatchHistoryCommand();
    void testBatchProcessMissingArgumentsValidation();
    void testBatchProcessEndToEndSuccess();

private:
    std::filesystem::path m_fixturesDir;
};

void TestBatchCli::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
    app::AppContext::Instance().Initialize();
}

void TestBatchCli::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestBatchCli::testIsBatchCommandDetection() {
    const char* argv1[] = { "PDFinity.exe", "batch" };
    QVERIFY(app::BatchCliHandler::IsBatchCommand(2, const_cast<char**>(argv1)));

    const char* argv2[] = { "PDFinity.exe", "--batch" };
    QVERIFY(app::BatchCliHandler::IsBatchCommand(2, const_cast<char**>(argv2)));

    const char* argv3[] = { "PDFinity.exe", "batch-profiles" };
    QVERIFY(app::BatchCliHandler::IsBatchCommand(2, const_cast<char**>(argv3)));

    const char* argv4[] = { "PDFinity.exe", "batch-history" };
    QVERIFY(app::BatchCliHandler::IsBatchCommand(2, const_cast<char**>(argv4)));

    const char* argvOther[] = { "PDFinity.exe", "document.pdf" };
    QVERIFY(!app::BatchCliHandler::IsBatchCommand(2, const_cast<char**>(argvOther)));
}

void TestBatchCli::testBatchProfilesCommand() {
    const char* argv[] = { "PDFinity.exe", "batch-profiles", "--list" };
    int code = app::BatchCliHandler::Execute(3, const_cast<char**>(argv));
    QCOMPARE(code, 0);
}

void TestBatchCli::testBatchHistoryCommand() {
    const char* argv[] = { "PDFinity.exe", "batch-history", "--list" };
    int code = app::BatchCliHandler::Execute(3, const_cast<char**>(argv));
    QCOMPARE(code, 0);
}

void TestBatchCli::testBatchProcessMissingArgumentsValidation() {
    // Missing all args
    const char* argv1[] = { "PDFinity.exe", "batch" };
    int code1 = app::BatchCliHandler::Execute(2, const_cast<char**>(argv1));
    QCOMPARE(code1, 1);

    // Missing output
    const char* argv2[] = { "PDFinity.exe", "batch", "--profile", "builtin_web_optimize", "--input", "test.pdf" };
    int code2 = app::BatchCliHandler::Execute(6, const_cast<char**>(argv2));
    QCOMPARE(code2, 1);

    // Non-existent profile
    const char* argv3[] = { "PDFinity.exe", "batch", "--profile", "unknown_profile_xyz", "--input", "test.pdf", "--output", "out" };
    int code3 = app::BatchCliHandler::Execute(8, const_cast<char**>(argv3));
    QCOMPARE(code3, 1);
}

void TestBatchCli::testBatchProcessEndToEndSuccess() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString()) / "cli_out";
    auto reportFile = std::filesystem::path(tempDir.path().toStdString()) / "cli_report.json";
    auto inputPdf = (m_fixturesDir / "ordinary.pdf").string();

    std::vector<std::string> args = {
        "PDFinity.exe",
        "batch",
        "--profile", "builtin_web_optimize",
        "--input", inputPdf,
        "--output", outDir.string(),
        "--collision", "overwrite",
        "--concurrency", "2",
        "--report", reportFile.string(),
        "--json"
    };

    std::vector<char*> argv;
    for (auto& s : args) {
        argv.push_back(s.data());
    }

    int code = app::BatchCliHandler::Execute(static_cast<int>(argv.size()), argv.data());
    QCOMPARE(code, 0);

    QVERIFY(std::filesystem::exists(outDir));
    QVERIFY(std::filesystem::exists(reportFile));
    QVERIFY(std::filesystem::file_size(reportFile) > 20);
}

QTEST_MAIN(TestBatchCli)
#include "test_batch_cli.moc"
