// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "McpTypes.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <functional>

namespace pdfinity::mcp {

class McpServer {
public:
    static McpServer& Instance();

    // Process a single JSON-RPC 2.0 message and return JSON-RPC 2.0 response
    std::string ProcessMessage(const std::string& jsonRpcRequest);

    // Session lifecycle
    void Disconnect();
    void ResetSession();

    McpSessionState GetSessionState() const;

private:
    McpServer();

    std::string HandleInitialize(const QJsonValue& id, const QJsonObject& params);
    std::string HandleToolsList(const QJsonValue& id, const QJsonObject& params);
    std::string HandleToolsCall(const QJsonValue& id, const QJsonObject& params);
    std::string HandleResourcesList(const QJsonValue& id, const QJsonObject& params);
    std::string HandleResourcesRead(const QJsonValue& id, const QJsonObject& params);

    std::string FormatResponse(const QJsonValue& id, const QJsonObject& result);
    std::string FormatError(const QJsonValue& id, int code, const std::string& message);

    mutable std::mutex m_mutex;
    McpSessionState m_state;
};

} // namespace pdfinity::mcp
