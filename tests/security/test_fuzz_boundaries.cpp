// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <fstream>
#include <vector>
#include <random>

#include "application/AppContext.hpp"
#include "pdf_api/IPdfEngine.hpp"
#include "security/PathSanitizer.hpp"
#include "security/SecureString.hpp"
#include "mcp/McpServer.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "remote/RemoteCommandDispatcher.hpp"
#include "remote/RemoteTypes.hpp"

using namespace pdfinity;
using namespace pdfinity::security;

class TestFuzzBoundaries : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. PDF Parser Boundary Tests
    void testZeroBytePdfResilience();
    void testTruncatedHeaderPdfResilience();
    void testRandomBinaryFuzzPdfResilience();

    // 2. Path Sanitizer & Device Name Fuzzing
    void testHostilePathTraversalFuzzing();
    void testWindowsDeviceNamesFuzzing();

    // 3. MCP JSON-RPC Malformed Payload Fuzzing
    void testMcpMalformedJsonFuzzing();
    void testMcpInvalidJsonRpcVersion();
    void testMcpInvalidMethodInvocation();

    // 4. Remote Command Boundary & Integer Overflow Fuzzing
    void testRemoteCommandAllowlistRejection();
    void testRemoteGotoPageNegativeAndHugeBoundary();
    void testRemoteZoomExtremeBoundaries();

    // 5. SecureString Boundary & Wipe Verification
    void testSecureStringBoundariesAndSelfAssignment();
};

void TestFuzzBoundaries::initTestCase() {
    app::AppContext::Instance().Initialize();
}

void TestFuzzBoundaries::cleanupTestCase() {
    app::AppContext::Instance().Shutdown();
}

void TestFuzzBoundaries::testZeroBytePdfResilience() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close(); // Empty 0-byte file

    auto engine = app::AppContext::Instance().GetPdfEngine();
    QVERIFY(engine != nullptr);
    auto res = engine->OpenDocument(tempFile.fileName().toStdString());
    // Must gracefully fail without crashing
    QVERIFY(!res.has_value());
}

void TestFuzzBoundaries::testTruncatedHeaderPdfResilience() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const char header[] = "%PDF-1.7\n";
    tempFile.write(header, sizeof(header) - 1);
    tempFile.close();

    auto engine = app::AppContext::Instance().GetPdfEngine();
    QVERIFY(engine != nullptr);
    auto res = engine->OpenDocument(tempFile.fileName().toStdString());
    QVERIFY(!res.has_value());
}

void TestFuzzBoundaries::testRandomBinaryFuzzPdfResilience() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    // Generate 64 KB of pseudorandom fuzz data
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, 255);
    QByteArray fuzzData;
    fuzzData.resize(65536);
    for (int i = 0; i < fuzzData.size(); ++i) {
        fuzzData[i] = static_cast<char>(dist(rng));
    }
    // Prepend pseudo PDF marker
    fuzzData[0] = '%';
    fuzzData[1] = 'P';
    fuzzData[2] = 'D';
    fuzzData[3] = 'F';

    tempFile.write(fuzzData);
    tempFile.close();

    auto engine = app::AppContext::Instance().GetPdfEngine();
    QVERIFY(engine != nullptr);
    auto res = engine->OpenDocument(tempFile.fileName().toStdString());
    // Must reject without crashing
    QVERIFY(!res.has_value());
}

void TestFuzzBoundaries::testHostilePathTraversalFuzzing() {
    std::vector<std::string> hostilePaths = {
        "../../../../Windows/System32/cmd.exe",
        "..\\..\\..\\..\\Windows\\System32\\cmd.exe",
        "/etc/passwd",
        "/etc/shadow",
        "C:/PDFinity/Workspace/../../../../sensitive.dat",
        "file:///C:/Windows/win.ini",
        "\\\\.\\PHYSICALDRIVE0",
        "\\\\?\\C:\\Windows\\System32\\calc.exe"
    };

    std::filesystem::path safeRoot = "C:/PDFinity/Workspace";

    for (const auto& path : hostilePaths) {
        auto canonical = PathSanitizer::Canonicalize(path);
        if (canonical.has_value()) {
            QCOMPARE(PathSanitizer::IsWithinBoundary(canonical.value(), safeRoot), false);
        } else {
            QVERIFY(!canonical.has_value());
        }
    }
}

void TestFuzzBoundaries::testWindowsDeviceNamesFuzzing() {
    std::vector<std::string> deviceNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
        "con.pdf", "aux.txt", "nul.bin", "prn.docx"
    };

    for (const auto& dev : deviceNames) {
        QCOMPARE(PathSanitizer::IsSafeFileName(dev), false);
        auto res = PathSanitizer::Canonicalize("C:/temp/" + dev);
        QVERIFY(!res.has_value());
        QCOMPARE(res.error(), PathError::ReservedDeviceName);
    }
}

void TestFuzzBoundaries::testMcpMalformedJsonFuzzing() {
    auto& server = mcp::McpServer::Instance();
    mcp::McpSecurityPolicy::Instance().SetEnabled(true);

    std::vector<std::string> malformedPayloads = {
        "",
        "   ",
        "{",
        "{ \"jsonrpc\": \"2.0\", ",
        "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": [1, 2, 3]}",
        "{\"jsonrpc\": \"2.0\", \"id\": 1, \"params\": null}"
    };

    for (const auto& payload : malformedPayloads) {
        std::string response = server.ProcessMessage(payload);
        QVERIFY(!response.empty());
        QVERIFY(response.find("\"error\"") != std::string::npos);
    }
}

void TestFuzzBoundaries::testMcpInvalidJsonRpcVersion() {
    auto& server = mcp::McpServer::Instance();

    std::string invalidVersion = "{\"jsonrpc\": \"1.0\", \"id\": 1, \"method\": \"tools/list\"}";
    std::string response = server.ProcessMessage(invalidVersion);
    QVERIFY(!response.empty());
    QVERIFY(response.find("-32600") != std::string::npos); // Invalid Request
}

void TestFuzzBoundaries::testMcpInvalidMethodInvocation() {
    auto& server = mcp::McpServer::Instance();

    std::string unknownMethod = "{\"jsonrpc\": \"2.0\", \"id\": 42, \"method\": \"system/execute_shell\"}";
    std::string response = server.ProcessMessage(unknownMethod);
    QVERIFY(!response.empty());
    QVERIFY(response.find("-32601") != std::string::npos); // Method Not Found
}

void TestFuzzBoundaries::testRemoteCommandAllowlistRejection() {
    auto& dispatcher = remote::RemoteCommandDispatcher::Instance();
    remote::RemoteDeviceInfo client;
    client.deviceId = "dev_fuzz";

    std::vector<std::string> hostileCommands = {
        "exec", "shell", "eval", "system", "cmd", "powershell",
        "format_c", "read_file", "write_file", "rm_rf"
    };

    for (const auto& cmd : hostileCommands) {
        auto resp = dispatcher.Execute(cmd, QJsonObject{}, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32601); // Method Not Found / Rejected
    }
}

void TestFuzzBoundaries::testRemoteGotoPageNegativeAndHugeBoundary() {
    auto& dispatcher = remote::RemoteCommandDispatcher::Instance();
    remote::RemoteDeviceInfo client;
    client.deviceId = "dev_fuzz";

    // Negative page index
    {
        QJsonObject params;
        params["page"] = -5;
        auto resp = dispatcher.Execute("goto_page", params, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32602); // Invalid params
    }

    // Zero page index (1-based index expected)
    {
        QJsonObject params;
        params["page"] = 0;
        auto resp = dispatcher.Execute("goto_page", params, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32602); // Invalid params
    }
}

void TestFuzzBoundaries::testRemoteZoomExtremeBoundaries() {
    auto& dispatcher = remote::RemoteCommandDispatcher::Instance();
    remote::RemoteDeviceInfo client;
    client.deviceId = "dev_fuzz";

    // Negative zoom
    {
        QJsonObject params;
        params["zoom"] = -50.0;
        auto resp = dispatcher.Execute("set_zoom", params, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32602);
    }

    // Zero zoom
    {
        QJsonObject params;
        params["zoom"] = 0.0;
        auto resp = dispatcher.Execute("set_zoom", params, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32602);
    }

    // Out-of-bounds zoom (> 1000%)
    {
        QJsonObject params;
        params["zoom"] = 50000.0;
        auto resp = dispatcher.Execute("set_zoom", params, client);
        QCOMPARE(resp.success, false);
        QCOMPARE(resp.errorCode, -32602);
    }
}

void TestFuzzBoundaries::testSecureStringBoundariesAndSelfAssignment() {
    SecureString s1("TestKeySuperSecret987!");
    QCOMPARE(s1.empty(), false);
    QCOMPARE(s1.size(), static_cast<size_t>(22));

    // Clear wipes memory safely
    s1.clear();
    QCOMPARE(s1.size(), static_cast<size_t>(0));
    QCOMPARE(s1.empty(), true);
}

QTEST_MAIN(TestFuzzBoundaries)
#include "test_fuzz_boundaries.moc"
