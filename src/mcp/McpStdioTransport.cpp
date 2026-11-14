// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "McpStdioTransport.hpp"
#include "McpServer.hpp"
#include "application/Logger.hpp"

#include <string>
#include <iostream>

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif

namespace pdfinity::mcp {

void McpStdioTransport::RequestStop() {
    s_stopRequested = true;
}

int McpStdioTransport::RunStdioServer() {
    s_stopRequested = false;

    // Fast I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    LOG_INFO("McpStdio", "Starting PDFinity MCP Stdio Server loop...");

    std::string line;
    while (!s_stopRequested && std::getline(std::cin, line)) {
        // Strip trailing CR if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        std::string response = McpServer::Instance().ProcessMessage(line);
        if (!response.empty()) {
            std::cout << response << "\n";
            std::cout.flush();
        }
    }

    LOG_INFO("McpStdio", "PDFinity MCP Stdio Server loop terminated (EOF or stop requested).");
    McpServer::Instance().Disconnect();
    return 0;
}

} // namespace pdfinity::mcp
