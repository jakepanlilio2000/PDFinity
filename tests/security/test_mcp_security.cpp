// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryDir>
#include <filesystem>
#include "mcp/McpServer.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "mcp/McpToolCatalog.hpp"
#include "application/AppContext.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace pdfinity;

class TestMcpSecurity : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testMcpOffByDefault();
    void testPathTraversalBlocked();
    void testApprovedRootEnforcement();
    void testDestructiveOperationsConsentGate();
    void testSourceImmutabilityViolationBlocked();
    void testSecretsRedactionInPolicy();
    void testMaxRequestSizeExceeded();
    void testDisconnectAndSessionReset();

private:
    std::filesystem::path m_fixturesDir;
};

void TestMcpSecurity::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
    app::AppContext::Instance().Initialize();
}

void TestMcpSecurity::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestMcpSecurity::init() {
    // Reset policy and server state between tests
    auto& sec = mcp::McpSecurityPolicy::Instance();
    mcp::McpSessionConfig defaultCfg;
    defaultCfg.enabled = false;
    defaultCfg.consentPolicy = mcp::McpConsentPolicy::AskForDestructive;
    defaultCfg.allowDestructive = false;
    defaultCfg.maxRequestSizeBytes = 10 * 1024 * 1024;
    sec.SetConfig(defaultCfg);
    sec.ClearApprovedRoots();
    sec.ClearAuditLogs();
    mcp::McpServer::Instance().ResetSession();
}

void TestMcpSecurity::testMcpOffByDefault() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    QVERIFY(!sec.IsEnabled());

    // Calling tools/list when disabled should return error -32001
    std::string req = R"({"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}})";
    std::string respStr = mcp::McpServer::Instance().ProcessMessage(req);
    QVERIFY(!respStr.empty());

    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(respStr));
    QVERIFY(doc.isObject());
    auto obj = doc.object();
    QVERIFY(obj.contains("error"));
    QCOMPARE(obj["error"].toObject()["code"].toInt(), -32001);
    QVERIFY(obj["error"].toObject()["message"].toString().contains("disabled"));

    // tools/call when disabled should also return -32001
    std::string callReq = R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"pdf_inspect","arguments":{"path":"some.pdf"}}})";
    std::string callResp = mcp::McpServer::Instance().ProcessMessage(callReq);
    auto callDoc = QJsonDocument::fromJson(QByteArray::fromStdString(callResp));
    QCOMPARE(callDoc.object()["error"].toObject()["code"].toInt(), -32001);
}

void TestMcpSecurity::testPathTraversalBlocked() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto rootPath = std::filesystem::path(tempDir.path().toStdString());
    sec.AddApprovedRoot(rootPath);

    // Initialize session
    mcp::McpServer::Instance().ProcessMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})");

    // Attempt path traversal with ../../../../
    std::string traversalPath = (rootPath / ".." / ".." / "Windows" / "System32" / "cmd.exe").string();
    QJsonObject args;
    args["path"] = QString::fromStdString(traversalPath);
    QJsonObject callParams;
    callParams["name"] = "pdf_inspect";
    callParams["arguments"] = args;
    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 2;
    req["method"] = "tools/call";
    req["params"] = callParams;

    std::string respStr = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(respStr));
    auto res = doc.object()["result"].toObject();

    // Must be returned as tool execution error (isError = true)
    QVERIFY(res["isError"].toBool());
    auto contentText = res["content"].toArray()[0].toObject()["text"].toString();
    QVERIFY(contentText.contains("outside all approved directory roots"));
}

void TestMcpSecurity::testApprovedRootEnforcement() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto approvedDir = std::filesystem::path(tempDir.path().toStdString()) / "approved";
    auto outsideDir = std::filesystem::path(tempDir.path().toStdString()) / "outside";
    std::filesystem::create_directories(approvedDir);
    std::filesystem::create_directories(outsideDir);

    sec.AddApprovedRoot(approvedDir);

    // Copy fixture into both directories
    auto insidePdf = approvedDir / "doc.pdf";
    auto outsidePdf = outsideDir / "secret.pdf";
    std::filesystem::copy_file(m_fixturesDir / "ordinary.pdf", insidePdf);
    std::filesystem::copy_file(m_fixturesDir / "ordinary.pdf", outsidePdf);

    mcp::McpServer::Instance().ProcessMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})");

    // 1. Inside approved root: Should succeed
    {
        QJsonObject args;
        args["path"] = QString::fromStdString(insidePdf.string());
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["id"] = 10;
        req["method"] = "tools/call";
        QJsonObject params;
        params["name"] = "pdf_inspect";
        params["arguments"] = args;
        req["params"] = params;

        std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
        auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
        QVERIFY(!doc.object()["result"].toObject()["isError"].toBool());
    }

    // 2. Outside approved root: Must fail
    {
        QJsonObject args;
        args["path"] = QString::fromStdString(outsidePdf.string());
        QJsonObject req;
        req["jsonrpc"] = "2.0";
        req["id"] = 11;
        req["method"] = "tools/call";
        QJsonObject params;
        params["name"] = "pdf_inspect";
        params["arguments"] = args;
        req["params"] = params;

        std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
        auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
        QVERIFY(doc.object()["result"].toObject()["isError"].toBool());
        QVERIFY(doc.object()["result"].toObject()["content"].toArray()[0].toObject()["text"].toString().contains("outside all approved directory roots"));
    }
}

void TestMcpSecurity::testDestructiveOperationsConsentGate() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);
    sec.SetAllowDestructive(false); // Destructive operations NOT allowed

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dir = std::filesystem::path(tempDir.path().toStdString());
    sec.AddApprovedRoot(dir);

    auto inPdf = dir / "input.pdf";
    auto outPdf = dir / "output_flattened.pdf";
    std::filesystem::copy_file(m_fixturesDir / "acroform_sample.pdf", inPdf);

    mcp::McpServer::Instance().ProcessMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})");

    QJsonObject args;
    args["inputPath"] = QString::fromStdString(inPdf.string());
    args["outputPath"] = QString::fromStdString(outPdf.string());
    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 20;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_flatten_forms";
    params["arguments"] = args;
    req["params"] = params;

    // 1. Without allowDestructive: MUST be blocked
    std::string resp1 = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc1 = QJsonDocument::fromJson(QByteArray::fromStdString(resp1));
    auto res1 = doc1.object()["result"].toObject();
    QVERIFY(res1["isError"].toBool());
    QVERIFY(res1["content"].toArray()[0].toObject()["text"].toString().contains("requires explicit operator consent"));

    // 2. With allowDestructive = true: MUST succeed
    sec.SetAllowDestructive(true);
    std::string resp2 = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc2 = QJsonDocument::fromJson(QByteArray::fromStdString(resp2));
    auto res2 = doc2.object()["result"].toObject();
    QVERIFY(!res2["isError"].toBool());
    QVERIFY(std::filesystem::exists(outPdf));
}

void TestMcpSecurity::testSourceImmutabilityViolationBlocked() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);
    sec.SetAllowDestructive(true);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dir = std::filesystem::path(tempDir.path().toStdString());
    sec.AddApprovedRoot(dir);

    auto sourcePdf = dir / "original.pdf";
    std::filesystem::copy_file(m_fixturesDir / "ordinary.pdf", sourcePdf);
    auto origSize = std::filesystem::file_size(sourcePdf);

    mcp::McpServer::Instance().ProcessMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05"}})");

    // Attempt to pass inputPath == outputPath
    QJsonObject args;
    args["inputPath"] = QString::fromStdString(sourcePdf.string());
    args["outputPath"] = QString::fromStdString(sourcePdf.string());
    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 30;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_optimize";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();

    QVERIFY(res["isError"].toBool());
    QVERIFY(res["content"].toArray()[0].toObject()["text"].toString().contains("Security Violation: Destination cannot overwrite the source"));
    // Original file must remain untouched
    QCOMPARE(std::filesystem::file_size(sourcePdf), origSize);
}

void TestMcpSecurity::testSecretsRedactionInPolicy() {
    QJsonObject in;
    in["username"] = "admin";
    in["password"] = "SuperSecret123!";
    in["privateKey"] = "MIIEvgIBADANBgkqhkiG9w0BAQEFAASC...";
    in["normalParam"] = "value";

    auto redacted = mcp::McpSecurityPolicy::RedactSecrets(in);
    QCOMPARE(redacted["username"].toString(), QString("admin"));
    QCOMPARE(redacted["normalParam"].toString(), QString("value"));
    QCOMPARE(redacted["password"].toString(), QString("******** [REDACTED]"));
    QCOMPARE(redacted["privateKey"].toString(), QString("******** [REDACTED]"));
}

void TestMcpSecurity::testMaxRequestSizeExceeded() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);

    // Set artificially small limit of 50 bytes
    mcp::McpSessionConfig cfg = sec.GetConfig();
    cfg.maxRequestSizeBytes = 50;
    sec.SetConfig(cfg);

    std::string longRequest = R"({"jsonrpc":"2.0","id":1,"method":"ping","params":{"padding":"this is a very long padding string that exceeds 50 bytes"}})";
    std::string resp = mcp::McpServer::Instance().ProcessMessage(longRequest);

    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    QVERIFY(doc.object().contains("error"));
    QCOMPARE(doc.object()["error"].toObject()["code"].toInt(), -32003);
    QVERIFY(doc.object()["error"].toObject()["message"].toString().contains("exceeds configured size limit"));
}

void TestMcpSecurity::testDisconnectAndSessionReset() {
    auto& server = mcp::McpServer::Instance();
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);

    server.ProcessMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","clientInfo":{"name":"ClaudeCode","version":"1.2"}}})");
    auto state = server.GetSessionState();
    QVERIFY(state.isConnected);
    QCOMPARE(QString::fromStdString(state.clientName), QString("ClaudeCode"));

    server.Disconnect();
    auto disconnectedState = server.GetSessionState();
    QVERIFY(!disconnectedState.isConnected);
}

QTEST_MAIN(TestMcpSecurity)
#include "test_mcp_security.moc"
