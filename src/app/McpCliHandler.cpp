// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpCliHandler.hpp"
#include "mcp/McpServer.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "mcp/McpStdioTransport.hpp"
#include "application/Logger.hpp"

#include <iostream>
#include <filesystem>
#include <algorithm>

namespace pdfinity::app {

bool McpCliHandler::IsMcpCommand(int argc, char* argv[]) {
    if (argc < 2) return false;
    std::string cmd = argv[1];
    return (cmd == "mcp-server" || cmd == "--mcp-server" || cmd == "--mcp" || cmd == "mcp");
}

void McpCliHandler::PrintMcpHelp() {
    std::cout << "PDFinity - Model Context Protocol (MCP) Stdio Server\n\n"
              << "Usage: PDFinity mcp-server [options]\n\n"
              << "Options:\n"
              << "  --approved-roots <path...>   List of allowed directory roots for document access (Required)\n"
              << "  --allow-destructive          Permit destructive operations (redaction, flattening) without interactive confirmations\n"
              << "  --consent-policy <policy>    Enforce policy: AskForDestructive (default), TrustApprovedRootsOnly, AlwaysAsk, DenyAll\n"
              << "  --max-request-size <N>       Maximum request payload size in bytes (default: 10485760 / 10MB)\n"
              << "  -h, --help                   Display this help message and exit\n\n"
              << "Description:\n"
              << "  Launches the official JSON-RPC 2.0 stdio transport (MCP version 2024-11-05).\n"
              << "  Used by local client applications and integration tooling for document tool execution.\n";
}

int McpCliHandler::Execute(int argc, char* argv[]) {
    std::vector<std::filesystem::path> approvedRoots;
    bool allowDestructive = false;
    mcp::McpConsentPolicy policy = mcp::McpConsentPolicy::AskForDestructive;
    size_t maxSize = 10 * 1024 * 1024;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintMcpHelp();
            return 0;
        } else if (arg == "--approved-roots") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
                approvedRoots.emplace_back(argv[i]);
            }
        } else if (arg == "--allow-destructive") {
            allowDestructive = true;
        } else if (arg == "--consent-policy") {
            if (++i < argc) {
                policy = mcp::StringToConsentPolicy(argv[i]);
            }
        } else if (arg == "--max-request-size") {
            if (++i < argc) {
                try { maxSize = std::stoull(argv[i]); } catch (...) {}
            }
        }
    }

    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(true); // Explicitly activated by mcp-server subcommand
    sec.SetAllowDestructive(allowDestructive);
    sec.SetConsentPolicy(policy);

    sec.ClearApprovedRoots();
    for (const auto& r : approvedRoots) {
        sec.AddApprovedRoot(r);
    }

    LOG_INFO("McpCli", "Configured MCP stdio server with " + std::to_string(approvedRoots.size()) +
             " approved roots. Destructive allowed: " + (allowDestructive ? "YES" : "NO"));

    return mcp::McpStdioTransport::RunStdioServer();
}

} // namespace pdfinity::app
