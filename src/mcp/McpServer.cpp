// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpServer.hpp"
#include "McpSecurityPolicy.hpp"
#include "McpToolCatalog.hpp"
#include "application/Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace pdfinity::mcp {

McpServer& McpServer::Instance() {
    static McpServer instance;
    return instance;
}

McpServer::McpServer() {
    ResetSession();
}

void McpServer::ResetSession() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = McpSessionState{};
    m_state.negotiatedProtocolVersion = MCP_PROTOCOL_VERSION;
}

void McpServer::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state.isConnected) {
        LOG_INFO("McpServer", "Client disconnected from MCP session: " + m_state.clientName);
        McpSecurityPolicy::Instance().RecordAuditLog(m_state.clientName, "disconnect", "-", "SUCCESS", "Client disconnected");
    }
    m_state.isConnected = false;
    m_state.isInitialized = false;
}

McpSessionState McpServer::GetSessionState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

std::string McpServer::FormatResponse(const QJsonValue& id, const QJsonObject& result) {
    QJsonObject resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["result"] = result;
    return QString::fromUtf8(QJsonDocument(resp).toJson(QJsonDocument::Compact)).toStdString();
}

std::string McpServer::FormatError(const QJsonValue& id, int code, const std::string& message) {
    QJsonObject err;
    err["code"] = code;
    err["message"] = QString::fromStdString(message);

    QJsonObject resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["error"] = err;
    return QString::fromUtf8(QJsonDocument(resp).toJson(QJsonDocument::Compact)).toStdString();
}

std::string McpServer::ProcessMessage(const std::string& jsonRpcRequest) {
    auto& policy = McpSecurityPolicy::Instance();

    // 1. Check message size against denial of service
    if (!policy.CheckRequestSize(jsonRpcRequest.length())) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_state.securityViolations++;
        }
        policy.RecordAuditLog("client", "ProcessMessage", "-", "BLOCKED", "Request size exceeds limit (" + std::to_string(jsonRpcRequest.length()) + " bytes)");
        return FormatError(QJsonValue(), -32003, "Request payload exceeds configured size limit");
    }

    // 2. Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(jsonRpcRequest.data(), static_cast<int>(jsonRpcRequest.length())), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        policy.RecordAuditLog("client", "ProcessMessage", "-", "ERROR", "JSON parse error: " + parseError.errorString().toStdString());
        return FormatError(QJsonValue(), -32700, "Parse error: " + parseError.errorString().toStdString());
    }

    QJsonObject root = doc.object();
    QJsonValue id = root.value("id");

    if (root.value("jsonrpc").toString() != "2.0") {
        return FormatError(id, -32600, "Invalid Request: 'jsonrpc' must be '2.0'");
    }

    QString method = root.value("method").toString();
    if (method.isEmpty()) {
        return FormatError(id, -32600, "Invalid Request: 'method' field is required");
    }

    QJsonObject params = root.value("params").toObject();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.requestsHandled++;
    }

    // Handle notifications (no response required if id is null or undefined)
    if (method == "notifications/initialized") {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.isInitialized = true;
        policy.RecordAuditLog(m_state.clientName, method.toStdString(), "-", "SUCCESS", "Client confirmed initialization");
        return "";
    }

    if (method == "ping") {
        return FormatResponse(id, QJsonObject{});
    }

    if (method == "initialize") {
        return HandleInitialize(id, params);
    }

    // All subsequent methods require MCP to be enabled
    if (!policy.IsEnabled()) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_state.securityViolations++;
        }
        policy.RecordAuditLog(m_state.clientName, method.toStdString(), "-", "DENIED", "MCP server is disabled by operator");
        return FormatError(id, -32001, "MCP server is disabled by operator policy. Enable it in PDFinity Settings or pass --mcp-server.");
    }

    if (method == "tools/list") {
        return HandleToolsList(id, params);
    } else if (method == "tools/call") {
        return HandleToolsCall(id, params);
    } else if (method == "resources/list") {
        return HandleResourcesList(id, params);
    } else if (method == "resources/read") {
        return HandleResourcesRead(id, params);
    }

    return FormatError(id, -32601, "Method not found: " + method.toStdString());
}

std::string McpServer::HandleInitialize(const QJsonValue& id, const QJsonObject& params) {
    auto& policy = McpSecurityPolicy::Instance();
    std::string clientProto = params.value("protocolVersion").toString().toStdString();
    QJsonObject clientInfo = params.value("clientInfo").toObject();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.isConnected = true;
        m_state.clientName = clientInfo.value("name").toString("UnknownClient").toStdString();
        m_state.clientVersion = clientInfo.value("version").toString("1.0").toStdString();
        m_state.negotiatedProtocolVersion = MCP_PROTOCOL_VERSION;
    }

    policy.RecordAuditLog(m_state.clientName, "initialize", "-", "SUCCESS",
                          "Client connected with protocol: " + clientProto);

    QJsonObject result;
    result["protocolVersion"] = MCP_PROTOCOL_VERSION;

    QJsonObject caps;
    caps["tools"] = QJsonObject{{"listChanged", false}};
    caps["resources"] = QJsonObject{{"subscribe", false}, {"listChanged", false}};
    result["capabilities"] = caps;

    QJsonObject sInfo;
    sInfo["name"] = MCP_SERVER_NAME;
    sInfo["version"] = MCP_SERVER_VERSION;
    result["serverInfo"] = sInfo;

    return FormatResponse(id, result);
}

std::string McpServer::HandleToolsList(const QJsonValue& id, const QJsonObject& /*params*/) {
    auto tools = McpToolCatalog::Instance().GetAllTools();

    QJsonArray toolsArray;
    for (const auto& t : tools) {
        QJsonObject toolObj;
        toolObj["name"] = QString::fromStdString(t.name);
        toolObj["description"] = QString::fromStdString(t.description);
        toolObj["inputSchema"] = t.inputSchema;
        toolsArray.append(toolObj);
    }

    QJsonObject result;
    result["tools"] = toolsArray;

    McpSecurityPolicy::Instance().RecordAuditLog(m_state.clientName, "tools/list", "-", "SUCCESS",
                                                "Returned " + std::to_string(tools.size()) + " tools");
    return FormatResponse(id, result);
}

std::string McpServer::HandleToolsCall(const QJsonValue& id, const QJsonObject& params) {
    std::string toolName = params.value("name").toString().toStdString();
    QJsonObject args = params.value("arguments").toObject();

    if (toolName.empty()) {
        return FormatError(id, -32602, "Invalid params: 'name' is required for tools/call");
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.toolsExecuted++;
    }

    auto toolRes = McpToolCatalog::Instance().ExecuteTool(toolName, args);

    QJsonObject result;
    result["isError"] = toolRes.isError;

    QJsonArray contentArray;
    QJsonObject contentItem;
    contentItem["type"] = "text";
    contentItem["text"] = QString::fromStdString(toolRes.textContent);
    contentArray.append(contentItem);

    result["content"] = contentArray;

    if (toolRes.isError) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.securityViolations++;
    }

    return FormatResponse(id, result);
}

std::string McpServer::HandleResourcesList(const QJsonValue& id, const QJsonObject& /*params*/) {
    auto roots = McpSecurityPolicy::Instance().GetApprovedRoots();

    QJsonArray resArray;
    for (size_t i = 0; i < roots.size(); ++i) {
        QJsonObject resObj;
        resObj["uri"] = QString("file:///%1").arg(QString::fromStdString(roots[i].string()).replace('\\', '/'));
        resObj["name"] = QString("Approved Root #%1").arg(i + 1);
        resObj["mimeType"] = "inode/directory";
        resArray.append(resObj);
    }

    QJsonObject result;
    result["resources"] = resArray;
    return FormatResponse(id, result);
}

std::string McpServer::HandleResourcesRead(const QJsonValue& id, const QJsonObject& params) {
    QString uri = params.value("uri").toString();
    if (uri.isEmpty()) {
        return FormatError(id, -32602, "Invalid params: 'uri' is required");
    }

    // Validate URI begins with file:///
    if (!uri.startsWith("file:///")) {
        return FormatError(id, -32001, "Only file:/// resources are supported");
    }

    QString localPathStr = uri.mid(8); // Strip file:///
    std::filesystem::path localPath = localPathStr.toStdString();

    auto check = McpSecurityPolicy::Instance().ValidatePath(localPath, true);
    if (!check.allowed) {
        return FormatError(id, -32001, check.errorMessage);
    }

    QJsonObject result;
    QJsonArray contents;
    QJsonObject item;
    item["uri"] = uri;
    item["mimeType"] = "application/pdf";
    item["text"] = QString("PDF file: %1 (Size: %2 bytes)")
                      .arg(QString::fromStdString(localPath.filename().string()))
                      .arg(std::filesystem::file_size(localPath));
    contents.append(item);
    result["contents"] = contents;

    return FormatResponse(id, result);
}

} // namespace pdfinity::mcp
