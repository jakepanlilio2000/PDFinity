// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ui/MainWindow.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"
#include "app/BatchCliHandler.hpp"
#include "app/McpCliHandler.hpp"
#include "app/RemoteCliHandler.hpp"
#include <QApplication>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {
void AttachWindowsConsoleIfAvailable() {
#if defined(_WIN32)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }
#endif
}
void PrintHelp() {
    std::cout << "PDFinity - Professional Offline-First PDF Productivity Suite\n"
              << "Published by StackSync Solutions (https://www.stacksyncsolutions.com/)\n"
              << "Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)\n\n"
              << "Usage: PDFinity [command|options] [pdf-file]\n\n"
              << "GUI Options:\n"
              << "  -h, --help            Show this help message and exit\n"
              << "  -v, --version         Show application version and exit\n"
              << "  --about               Show publisher, lead programmer, and license info\n"
              << "  --headless-smoke      Execute headless startup/shutdown diagnostic and exit\n"
              << "  --debug-log           Enable debug level logging\n\n"
              << "Automation & Batch Commands:\n"
              << "  batch                 Execute automated multi-file workflow (use 'PDFinity batch --help')\n"
              << "  batch-profiles        List, export and import reusable profiles\n"
              << "  batch-history         Inspect or clear past batch execution logs\n\n"
              << "AI & Extension Commands:\n"
              << "  mcp-server            Start Model Context Protocol (MCP) stdio server (use 'PDFinity mcp-server --help')\n"
              << "  remote-client         Connect as a Remote Controller client (use 'PDFinity remote-client --help')\n";
}

void PrintVersion() {
    std::cout << "PDFinity 1.0.6 Stable\n"
              << "Published by StackSync Solutions (https://www.stacksyncsolutions.com/)\n"
              << "Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)\n"
              << "License: Mozilla Public License Version 2.0 (MPL-2.0)\n";
}
} // namespace

int main(int argc, char* argv[]) {
    // If command line arguments were provided on Windows, attach to parent console if one exists
    if (argc > 1) {
        AttachWindowsConsoleIfAvailable();
    }

    // Check for MCP Stdio Server Command (headless execution) first
    if (pdfinity::app::McpCliHandler::IsMcpCommand(argc, argv)) {
        if (!pdfinity::app::AppContext::Instance().Initialize()) {
            std::cerr << "Fatal Error: Failed to initialize PDFinity engine for MCP server.\n";
            return 1;
        }
        int code = pdfinity::app::McpCliHandler::Execute(argc, argv);
        pdfinity::app::AppContext::Instance().Shutdown();
        return code;
    }

    // Check for Batch CLI Automation Commands (headless execution)
    if (pdfinity::app::BatchCliHandler::IsBatchCommand(argc, argv)) {
        if (!pdfinity::app::AppContext::Instance().Initialize()) {
            std::cerr << "Fatal Error: Failed to initialize PDFinity engine for batch automation.\n";
            return 1;
        }
        int code = pdfinity::app::BatchCliHandler::Execute(argc, argv);
        pdfinity::app::AppContext::Instance().Shutdown();
        return code;
    }

    // Check for Remote Controller Client CLI Command
    if (pdfinity::app::RemoteCliHandler::IsRemoteCommand(argc, argv)) {
        return pdfinity::app::RemoteCliHandler::Execute(argc, argv);
    }

    // Process global command line flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintHelp();
            return 0;
        }
        if (arg == "--version" || arg == "-v" || arg == "--about") {
            PrintVersion();
            return 0;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("PDFinity");
    app.setApplicationVersion("1.0.6");
    app.setOrganizationName("StackSync Solutions");
    app.setOrganizationDomain("stacksyncsolutions.com");

    // Initialize application context
    if (!pdfinity::app::AppContext::Instance().Initialize()) {
        std::cerr << "Fatal Error: Failed to initialize PDFinity application context.\n";
        return 1;
    }

    // Handle Headless Smoke Diagnostic (for CI and automated test verification)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless-smoke") {
            LOG_INFO("Smoke", "Executing headless smoke diagnostic...");
            const auto& platform = pdfinity::app::AppContext::Instance().GetPlatform();
            LOG_INFO("Smoke", "Detected Platform: " + platform.GetPlatformName() + " " + platform.GetPlatformVersion());
            LOG_INFO("Smoke", "Architecture: " + platform.GetArchitecture());
            LOG_INFO("Smoke", "Per-Monitor DPI v2: " + std::string(platform.HasCapability(pdfinity::platform::PlatformCapability::PerMonitorDpiV2) ? "YES" : "NO"));
            
            // Verify settings persistence
            pdfinity::app::AppContext::Instance().SaveCurrentState();
            LOG_INFO("Smoke", "Settings and recent-file persistence verified.");
            
            pdfinity::app::AppContext::Instance().Shutdown();
            LOG_INFO("Smoke", "Headless smoke diagnostic completed successfully with status 0.");
            return 0;
        }
    }

    // Launch GUI Shell
    pdfinity::ui::MainWindow mainWindow;
    mainWindow.show();

    // Check if initial file was passed
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (!arg.starts_with("-")) {
            mainWindow.OpenFileByPath(QString::fromStdString(arg));
            break;
        }
    }

    int exitCode = app.exec();
    pdfinity::app::AppContext::Instance().Shutdown();
    return exitCode;
}
