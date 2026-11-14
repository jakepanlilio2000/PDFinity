// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "McpTypes.hpp"
#include "security/PathSanitizer.hpp"
#include <mutex>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

namespace pdfinity::mcp {

struct PolicyCheckResult {
    bool allowed{false};
    int errorCode{0}; // JSON-RPC error code
    std::string errorMessage;

    static PolicyCheckResult Allow() {
        return {true, 0, ""};
    }

    static PolicyCheckResult Deny(int code, const std::string& msg) {
        return {false, code, msg};
    }
};

class McpSecurityPolicy {
public:
    static McpSecurityPolicy& Instance();

    void SetConfig(const McpSessionConfig& config);
    McpSessionConfig GetConfig() const;

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void SetConsentPolicy(McpConsentPolicy policy);
    McpConsentPolicy GetConsentPolicy() const;

    void SetAllowDestructive(bool allow);
    bool IsAllowDestructive() const;

    void AddApprovedRoot(const std::filesystem::path& rootPath);
    void RemoveApprovedRoot(const std::filesystem::path& rootPath);
    void ClearApprovedRoots();
    std::vector<std::filesystem::path> GetApprovedRoots() const;

    // Path verification: checks against approved roots and path traversal attacks
    PolicyCheckResult ValidatePath(const std::filesystem::path& targetPath, bool mustExist = true) const;

    // Tool execution authorization: validates category, consent, and destructive gates
    PolicyCheckResult AuthorizeTool(const McpToolDefinition& tool, const QJsonObject& args) const;

    // Checks message size limit against denial of service
    bool CheckRequestSize(size_t byteCount) const;

    // Audit logging
    void RecordAuditLog(const std::string& client, const std::string& method,
                        const std::string& toolName, const std::string& status,
                        const std::string& details);
    std::vector<McpAuditLogEntry> GetAuditLogs() const;
    void ClearAuditLogs();

    // Redacts secret fields (passwords, private keys) from logging
    static QJsonObject RedactSecrets(const QJsonObject& input);

private:
    McpSecurityPolicy();

    mutable std::mutex m_mutex;
    McpSessionConfig m_config;
    std::vector<McpAuditLogEntry> m_auditLogs;
};

} // namespace pdfinity::mcp
