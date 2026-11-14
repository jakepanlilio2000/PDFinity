// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pdfinity::app {

enum class AppThemeMode {
    System,
    Light,
    Dark
};

struct AppConfig {
    AppThemeMode themeMode{AppThemeMode::System};
    int windowWidth{1280};
    int windowHeight{820};
    int windowPosX{-1};
    int windowPosY{-1};
    bool isMaximized{false};

    std::string logLevel{"Info"};
    bool autosaveEnabled{true};
    int autosaveIntervalSeconds{300};
    int maxRecentFiles{10};
    bool hardwareAcceleration{true};

    bool restoreSessionOnStartup{true};
    std::vector<std::string> sessionOpenFiles;
    int sessionActiveTabIndex{0};

    // 3-Pane Document Studio Shell State Persistence
    bool leftRailDrawerExpanded{true};
    int leftRailActivePanel{0};
    bool rightInspectorVisible{true};
    std::vector<int> viewerSplitterSizes{44, 850, 280};

    // Model Context Protocol (MCP) Integration
    bool mcpEnabled{false}; // Strictly off by default
    std::string mcpConsentPolicy{"AskForDestructive"};
    std::vector<std::string> mcpApprovedRoots;
    bool mcpAllowDestructiveOperations{false};
    size_t mcpMaxRequestSizeBytes{10 * 1024 * 1024};

    // Remote Controller Integration
    bool remoteEnabled{false}; // Strictly off by default
    std::string remoteBindAddress{"127.0.0.1"};
    uint16_t remotePort{8443};
    bool remoteRequireHostConfirmation{true};
    std::string remotePairedDevicesJson{"[]"};

    static std::string ThemeModeToString(AppThemeMode mode);
    static AppThemeMode StringToThemeMode(const std::string& str);
};

} // namespace pdfinity::app
