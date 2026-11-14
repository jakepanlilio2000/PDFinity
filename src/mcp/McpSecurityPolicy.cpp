// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpSecurityPolicy.hpp"
#include "application/Logger.hpp"
#include <QDateTime>
#include <algorithm>

namespace pdfinity::mcp {

McpSecurityPolicy& McpSecurityPolicy::Instance() {
    static McpSecurityPolicy instance;
    return instance;
}

McpSecurityPolicy::McpSecurityPolicy() {
    // Disabled by default
    m_config.enabled = false;
    m_config.consentPolicy = McpConsentPolicy::AskForDestructive;
    m_config.allowDestructive = false;
    m_config.maxRequestSizeBytes = 10 * 1024 * 1024;
}

void McpSecurityPolicy::SetConfig(const McpSessionConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

McpSessionConfig McpSecurityPolicy::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void McpSecurityPolicy::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.enabled = enabled;
    LOG_INFO("McpSecurity", std::string("MCP service status changed: ") + (enabled ? "ENABLED" : "DISABLED"));
}

bool McpSecurityPolicy::IsEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.enabled;
}

void McpSecurityPolicy::SetConsentPolicy(McpConsentPolicy policy) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.consentPolicy = policy;
}

McpConsentPolicy McpSecurityPolicy::GetConsentPolicy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.consentPolicy;
}

void McpSecurityPolicy::SetAllowDestructive(bool allow) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.allowDestructive = allow;
}

bool McpSecurityPolicy::IsAllowDestructive() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.allowDestructive;
}

void McpSecurityPolicy::AddApprovedRoot(const std::filesystem::path& rootPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto canon = security::PathSanitizer::Canonicalize(rootPath);
    if (canon.has_value()) {
        auto p = canon.value();
        if (std::find(m_config.approvedRoots.begin(), m_config.approvedRoots.end(), p) == m_config.approvedRoots.end()) {
            m_config.approvedRoots.push_back(p);
            LOG_INFO("McpSecurity", "Added approved root: " + p.string());
        }
    }
}

void McpSecurityPolicy::RemoveApprovedRoot(const std::filesystem::path& rootPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto canon = security::PathSanitizer::Canonicalize(rootPath);
    auto target = canon.has_value() ? canon.value() : rootPath;
    m_config.approvedRoots.erase(
        std::remove_if(m_config.approvedRoots.begin(), m_config.approvedRoots.end(),
                       [&target](const std::filesystem::path& p) { return p == target; }),
        m_config.approvedRoots.end()
    );
}

void McpSecurityPolicy::ClearApprovedRoots() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.approvedRoots.clear();
    LOG_INFO("McpSecurity", "All approved roots cleared.");
}

std::vector<std::filesystem::path> McpSecurityPolicy::GetApprovedRoots() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.approvedRoots;
}

PolicyCheckResult McpSecurityPolicy::ValidatePath(const std::filesystem::path& targetPath, bool mustExist) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_config.enabled) {
        return PolicyCheckResult::Deny(-32001, "MCP integration is disabled in application settings");
    }

    if (targetPath.empty()) {
        return PolicyCheckResult::Deny(-32602, "Target path parameter is empty");
    }

    // 1. Path sanitization and canonicalization
    auto canon = security::PathSanitizer::Canonicalize(targetPath);
    if (!canon.has_value()) {
        return PolicyCheckResult::Deny(-32001, "Invalid path: " + security::PathSanitizer::GetErrorMessage(canon.error()));
    }

    const auto& canonical = canon.value();

    // 2. Approved roots boundary check (enforce sandbox boundary first to prevent path probing)
    if (m_config.approvedRoots.empty()) {
        return PolicyCheckResult::Deny(-32001, "Filesystem access blocked: No approved roots configured for MCP");
    }

    bool insideApproved = false;
    for (const auto& root : m_config.approvedRoots) {
        if (security::PathSanitizer::IsWithinBoundary(canonical, root)) {
            insideApproved = true;
            break;
        }
    }

    if (!insideApproved) {
        return PolicyCheckResult::Deny(-32001, "Path traversal/Sandbox violation: Path '" + targetPath.string() +
                                              "' is outside all approved directory roots");
    }

    // 3. Existence check (within approved roots)
    if (mustExist && !std::filesystem::exists(canonical)) {
        return PolicyCheckResult::Deny(-32002, "File not found: " + canonical.string());
    }

    return PolicyCheckResult::Allow();
}

PolicyCheckResult McpSecurityPolicy::AuthorizeTool(const McpToolDefinition& tool, const QJsonObject& /*args*/) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_config.enabled) {
        return PolicyCheckResult::Deny(-32001, "MCP server is disabled by operator policy");
    }

    if (m_config.consentPolicy == McpConsentPolicy::DenyAll) {
        return PolicyCheckResult::Deny(-32001, "All MCP tool invocations are denied by policy (ConsentPolicy::DenyAll)");
    }

    // Destructive tool gating
    if (tool.category == McpToolCategory::Destructive) {
        if (!m_config.allowDestructive) {
            return PolicyCheckResult::Deny(-32001, "Destructive tool '" + tool.name +
                                                  "' is blocked: requires explicit operator consent or --allow-destructive flag");
        }
    }

    return PolicyCheckResult::Allow();
}

bool McpSecurityPolicy::CheckRequestSize(size_t byteCount) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return byteCount <= m_config.maxRequestSizeBytes;
}

void McpSecurityPolicy::RecordAuditLog(const std::string& client, const std::string& method,
                                      const std::string& toolName, const std::string& status,
                                      const std::string& details) {
    std::lock_guard<std::mutex> lock(m_mutex);
    McpAuditLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    entry.client = client.empty() ? "stdio_client" : client;
    entry.method = method;
    entry.toolName = toolName;
    entry.status = status;
    entry.details = details;

    m_auditLogs.push_back(entry);
    if (m_auditLogs.size() > 1000) {
        m_auditLogs.erase(m_auditLogs.begin(), m_auditLogs.begin() + 100);
    }

    if (status == "DENIED" || status == "BLOCKED") {
        LOG_WARN("McpSecurity", "[" + status + "] " + method + " (" + toolName + "): " + details);
    } else {
        LOG_INFO("McpSecurity", "[" + status + "] " + method + " (" + toolName + "): " + details);
    }
}

std::vector<McpAuditLogEntry> McpSecurityPolicy::GetAuditLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auditLogs;
}

void McpSecurityPolicy::ClearAuditLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_auditLogs.clear();
}

QJsonObject McpSecurityPolicy::RedactSecrets(const QJsonObject& input) {
    QJsonObject output = input;
    static const std::vector<QString> sensitiveKeys = {
        "password", "passphrase", "secret", "privateKey", "private_key", "pin", "key"
    };

    for (const auto& key : sensitiveKeys) {
        if (output.contains(key)) {
            output[key] = "******** [REDACTED]";
        }
    }
    return output;
}

} // namespace pdfinity::mcp
