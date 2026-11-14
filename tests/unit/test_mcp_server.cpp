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

class TestMcpServer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testInitializeHandshake();
    void testPing();
    void testToolsListCatalog();
    void testPdfInspectTool();
    void testPdfExtractTextTool();
    void testPdfSearchTool();
    void testPdfPreflightValidateTool();
    void testPdfOptimizeTool();
    void testPdfConvertToPdfATool();
    void testPdfRedactAndSanitizeTool();
    void testResourcesListAndRead();
    void testJsonRpcErrorHandling();

private:
    std::filesystem::path m_fixturesDir;
};

void TestMcpServer::initTestCase() {
    m_fixturesDir = std::filesystem::path(PDFINITY_SOURCE_DIR) / "tests" / "fixtures";
    QVERIFY(std::filesystem::exists(m_fixturesDir));
    app::AppContext::Instance().Initialize();
}

void TestMcpServer::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestMcpServer::init() {
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true);
    sec.SetAllowDestructive(true);
    sec.ClearApprovedRoots();
    sec.AddApprovedRoot(m_fixturesDir);
    mcp::McpServer::Instance().ResetSession();
}

void TestMcpServer::testInitializeHandshake() {
    std::string req = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {
                "name": "TestMcpClient",
                "version": "2.0"
            }
        }
    })";

    std::string respStr = mcp::McpServer::Instance().ProcessMessage(req);
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(respStr));
    QVERIFY(doc.isObject());
    auto root = doc.object();
    QCOMPARE(root["jsonrpc"].toString(), QString("2.0"));
    QCOMPARE(root["id"].toInt(), 1);

    auto result = root["result"].toObject();
    QCOMPARE(result["protocolVersion"].toString(), QString("2024-11-05"));
    QCOMPARE(result["serverInfo"].toObject()["name"].toString(), QString("PDFinity"));
    QVERIFY(result["capabilities"].toObject().contains("tools"));
    QVERIFY(result["capabilities"].toObject().contains("resources"));
}

void TestMcpServer::testPing() {
    std::string req = R"({"jsonrpc":"2.0","id":42,"method":"ping"})";
    std::string respStr = mcp::McpServer::Instance().ProcessMessage(req);
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(respStr));
    QCOMPARE(doc.object()["id"].toInt(), 42);
    QVERIFY(doc.object().contains("result"));
}

void TestMcpServer::testToolsListCatalog() {
    std::string req = R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})";
    std::string respStr = mcp::McpServer::Instance().ProcessMessage(req);
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(respStr));

    auto tools = doc.object()["result"].toObject()["tools"].toArray();
    QVERIFY(tools.size() >= 10);

    // Verify key tools are present
    QStringList toolNames;
    for (const auto& t : tools) {
        toolNames.append(t.toObject()["name"].toString());
        QVERIFY(!t.toObject()["description"].toString().isEmpty());
        QVERIFY(t.toObject().contains("inputSchema"));
    }

    QVERIFY(toolNames.contains("pdf_inspect"));
    QVERIFY(toolNames.contains("pdf_extract_text"));
    QVERIFY(toolNames.contains("pdf_search"));
    QVERIFY(toolNames.contains("pdf_preflight_validate"));
    QVERIFY(toolNames.contains("pdf_verify_signatures"));
    QVERIFY(toolNames.contains("pdf_optimize"));
    QVERIFY(toolNames.contains("pdf_convert_to_pdfa"));
    QVERIFY(toolNames.contains("pdf_flatten_forms"));
    QVERIFY(toolNames.contains("pdf_redact_and_sanitize"));
    QVERIFY(toolNames.contains("mcp_list_approved_roots"));
}

void TestMcpServer::testPdfInspectTool() {
    auto targetPdf = m_fixturesDir / "ordinary.pdf";
    QJsonObject args;
    args["path"] = QString::fromStdString(targetPdf.string());

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 3;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_inspect";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());

    auto text = res["content"].toArray()[0].toObject()["text"].toString();
    QVERIFY(text.contains("pageCount"));
    QVERIFY(text.contains("metadata"));
}

void TestMcpServer::testPdfExtractTextTool() {
    auto targetPdf = m_fixturesDir / "ordinary.pdf";
    QJsonObject args;
    args["path"] = QString::fromStdString(targetPdf.string());
    args["startPage"] = 1;
    args["endPage"] = 1;

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 4;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_extract_text";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());

    auto text = res["content"].toArray()[0].toObject()["text"].toString();
    QVERIFY(text.contains("Page 1"));
}

void TestMcpServer::testPdfSearchTool() {
    auto targetPdf = m_fixturesDir / "ordinary.pdf";
    QJsonObject args;
    args["path"] = QString::fromStdString(targetPdf.string());
    args["query"] = "PDFinity";

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 5;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_search";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());

    auto text = res["content"].toArray()[0].toObject()["text"].toString();
    QVERIFY(text.contains("query"));
    QVERIFY(text.contains("totalMatches"));
}

void TestMcpServer::testPdfPreflightValidateTool() {
    auto targetPdf = m_fixturesDir / "ordinary.pdf";
    QJsonObject args;
    args["path"] = QString::fromStdString(targetPdf.string());
    args["profile"] = "PdfA1b";

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 6;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_preflight_validate";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());

    auto text = res["content"].toArray()[0].toObject()["text"].toString();
    QVERIFY(text.contains("totalErrors") || text.contains("isPassed"));
}

void TestMcpServer::testPdfOptimizeTool() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());
    mcp::McpSecurityPolicy::Instance().AddApprovedRoot(outDir);

    auto inPdf = m_fixturesDir / "ordinary.pdf";
    auto outPdf = outDir / "ordinary_opt.pdf";

    QJsonObject args;
    args["inputPath"] = QString::fromStdString(inPdf.string());
    args["outputPath"] = QString::fromStdString(outPdf.string());
    args["preset"] = "Screen";

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 7;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_optimize";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());
    QVERIFY(std::filesystem::exists(outPdf));
    QVERIFY(std::filesystem::file_size(outPdf) > 0);
}

void TestMcpServer::testPdfConvertToPdfATool() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());
    mcp::McpSecurityPolicy::Instance().AddApprovedRoot(outDir);

    auto inPdf = m_fixturesDir / "ordinary.pdf";
    auto outPdf = outDir / "ordinary_pdfa.pdf";

    QJsonObject args;
    args["inputPath"] = QString::fromStdString(inPdf.string());
    args["outputPath"] = QString::fromStdString(outPdf.string());
    args["profile"] = "PdfA1b";

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 8;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_convert_to_pdfa";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());
    QVERIFY(std::filesystem::exists(outPdf));
}

void TestMcpServer::testPdfRedactAndSanitizeTool() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto outDir = std::filesystem::path(tempDir.path().toStdString());
    mcp::McpSecurityPolicy::Instance().AddApprovedRoot(outDir);

    auto inPdf = m_fixturesDir / "ordinary.pdf";
    auto outPdf = outDir / "ordinary_sanitized.pdf";

    QJsonObject args;
    args["inputPath"] = QString::fromStdString(inPdf.string());
    args["outputPath"] = QString::fromStdString(outPdf.string());
    args["scrubMetadata"] = true;

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = 9;
    req["method"] = "tools/call";
    QJsonObject params;
    params["name"] = "pdf_redact_and_sanitize";
    params["arguments"] = args;
    req["params"] = params;

    std::string resp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(req).toJson(QJsonDocument::Compact)).toStdString());
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(resp));
    auto res = doc.object()["result"].toObject();
    QVERIFY(!res["isError"].toBool());
    QVERIFY(std::filesystem::exists(outPdf));
}

void TestMcpServer::testResourcesListAndRead() {
    // 1. resources/list
    std::string listReq = R"({"jsonrpc":"2.0","id":100,"method":"resources/list"})";
    std::string listResp = mcp::McpServer::Instance().ProcessMessage(listReq);
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(listResp));
    auto resources = doc.object()["result"].toObject()["resources"].toArray();
    QVERIFY(!resources.isEmpty());

    // 2. resources/read
    auto targetPdf = m_fixturesDir / "ordinary.pdf";
    QString uri = QString("file:///%1").arg(QString::fromStdString(targetPdf.string()).replace('\\', '/'));

    QJsonObject readReq;
    readReq["jsonrpc"] = "2.0";
    readReq["id"] = 101;
    readReq["method"] = "resources/read";
    QJsonObject readParams;
    readParams["uri"] = uri;
    readReq["params"] = readParams;

    std::string readResp = mcp::McpServer::Instance().ProcessMessage(QString(QJsonDocument(readReq).toJson(QJsonDocument::Compact)).toStdString());
    auto readDoc = QJsonDocument::fromJson(QByteArray::fromStdString(readResp));
    QVERIFY(readDoc.object().contains("result"));
    auto contents = readDoc.object()["result"].toObject()["contents"].toArray();
    QCOMPARE(contents.size(), 1);
    QVERIFY(contents[0].toObject()["text"].toString().contains("ordinary.pdf"));
}

void TestMcpServer::testJsonRpcErrorHandling() {
    // 1. Invalid JSON
    std::string badJson = "{ invalid json string ";
    std::string r1 = mcp::McpServer::Instance().ProcessMessage(badJson);
    auto d1 = QJsonDocument::fromJson(QByteArray::fromStdString(r1));
    QCOMPARE(d1.object()["error"].toObject()["code"].toInt(), -32700);

    // 2. Missing method
    std::string noMethod = R"({"jsonrpc":"2.0","id":1})";
    std::string r2 = mcp::McpServer::Instance().ProcessMessage(noMethod);
    auto d2 = QJsonDocument::fromJson(QByteArray::fromStdString(r2));
    QCOMPARE(d2.object()["error"].toObject()["code"].toInt(), -32600);

    // 3. Unknown method
    std::string unknownMethod = R"({"jsonrpc":"2.0","id":2,"method":"non_existent_method"})";
    std::string r3 = mcp::McpServer::Instance().ProcessMessage(unknownMethod);
    auto d3 = QJsonDocument::fromJson(QByteArray::fromStdString(r3));
    QCOMPARE(d3.object()["error"].toObject()["code"].toInt(), -32601);

    // 4. Invalid params for tools/call (missing name)
    std::string noToolName = R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{}})";
    std::string r4 = mcp::McpServer::Instance().ProcessMessage(noToolName);
    auto d4 = QJsonDocument::fromJson(QByteArray::fromStdString(r4));
    QCOMPARE(d4.object()["error"].toObject()["code"].toInt(), -32602);
}

QTEST_MAIN(TestMcpServer)
#include "test_mcp_server.moc"
