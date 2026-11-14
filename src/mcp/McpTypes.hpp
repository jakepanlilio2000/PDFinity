// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <QJsonObject>
#include <QJsonArray>

namespace pdfinity::mcp {

inline constexpr const char* MCP_PROTOCOL_VERSION = "2024-11-05";
inline constexpr const char* MCP_SERVER_NAME = "PDFinity";
inline constexpr const char* MCP_SERVER_VERSION = "1.0.3";

enum class McpToolCategory {
    ReadOnly,           // Non-mutating inspection, extraction, search
    Write,              // Generates or converts new output documents
    Destructive,        // Permanently modifies, scrubs, flattens or redacts content
    SecuritySensitive,  // Digital signature verification, encryption inspection
    Admin               // Management, status, approved roots listing
};

enum class McpConsentPolicy {
    AlwaysAsk,             // Requires user confirmation for every tool invocation
    AskForDestructive,     // Allows read/write inside approved roots, requires confirmation for destructive
    TrustApprovedRootsOnly,// Allows read/write inside approved roots, forbids destructive unless explicitly permitted
    DenyAll                // Blocks all tool execution attempts
};

struct McpToolDefinition {
    std::string name;
    std::string description;
    McpToolCategory category{McpToolCategory::ReadOnly};
    QJsonObject inputSchema;
};

struct McpAuditLogEntry {
    std::string timestamp;
    std::string client;
    std::string method;
    std::string toolName;
    std::string status; // "SUCCESS", "DENIED", "ERROR", "BLOCKED"
    std::string details;
};

struct McpSessionConfig {
    bool enabled{false}; // Strictly disabled by default
    McpConsentPolicy consentPolicy{McpConsentPolicy::AskForDestructive};
    std::vector<std::filesystem::path> approvedRoots;
    bool allowDestructive{false};
    size_t maxRequestSizeBytes{10 * 1024 * 1024}; // 10 MB ceiling
};

struct McpSessionState {
    bool isConnected{false};
    bool isInitialized{false};
    std::string clientName;
    std::string clientVersion;
    std::string negotiatedProtocolVersion;
    uint64_t requestsHandled{0};
    uint64_t toolsExecuted{0};
    uint64_t securityViolations{0};
};

inline std::string ToolCategoryToString(McpToolCategory cat) {
    switch (cat) {
        case McpToolCategory::ReadOnly: return "ReadOnly";
        case McpToolCategory::Write: return "Write";
        case McpToolCategory::Destructive: return "Destructive";
        case McpToolCategory::SecuritySensitive: return "SecuritySensitive";
        case McpToolCategory::Admin: return "Admin";
    }
    return "Unknown";
}

inline std::string ConsentPolicyToString(McpConsentPolicy policy) {
    switch (policy) {
        case McpConsentPolicy::AlwaysAsk: return "AlwaysAsk";
        case McpConsentPolicy::AskForDestructive: return "AskForDestructive";
        case McpConsentPolicy::TrustApprovedRootsOnly: return "TrustApprovedRootsOnly";
        case McpConsentPolicy::DenyAll: return "DenyAll";
    }
    return "Unknown";
}

inline McpConsentPolicy StringToConsentPolicy(const std::string& str) {
    if (str == "AlwaysAsk") return McpConsentPolicy::AlwaysAsk;
    if (str == "TrustApprovedRootsOnly") return McpConsentPolicy::TrustApprovedRootsOnly;
    if (str == "DenyAll") return McpConsentPolicy::DenyAll;
    return McpConsentPolicy::AskForDestructive;
}

} // namespace pdfinity::mcp
