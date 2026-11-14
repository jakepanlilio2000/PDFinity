// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AppContext.hpp"
#include "platform/PlatformFactory.hpp"
#include "application/Logger.hpp"
#include "mcp/McpSecurityPolicy.hpp"
#include "remote/RemoteSecurityPolicy.hpp"
#include "remote/RemotePairingManager.hpp"
#include "remote/RemoteControllerServer.hpp"

namespace pdfinity::app {

AppContext& AppContext::Instance() {
    static AppContext instance;
    return instance;
}

AppContext::~AppContext() {
    Shutdown();
}

bool AppContext::Initialize() {
    if (m_initialized) return true;

    m_platform = platform::PlatformFactory::CreateAdapter();
    std::filesystem::path appDataDir = m_platform->GetAppDataDirectory();

    // Initialize logger
    std::filesystem::path logFile = appDataDir / "logs" / "pdfinity.log";
    Logger::Instance().Initialize(logFile, LogLevel::Info);
    LOG_INFO("App", "Initializing PDFinity on " + m_platform->GetPlatformName() + " (" + m_platform->GetArchitecture() + ")");

    // Initialize persistence
    std::filesystem::path settingsFile = appDataDir / "settings.json";
    m_settings = std::make_unique<persistence::SettingsManager>(settingsFile);
    m_settings->LoadSettings(m_config);

    // Initialize MCP security policy from settings (disabled by default)
    auto& sec = mcp::McpSecurityPolicy::Instance();
    sec.SetEnabled(m_config.mcpEnabled);
    sec.SetConsentPolicy(mcp::StringToConsentPolicy(m_config.mcpConsentPolicy));
    sec.SetAllowDestructive(m_config.mcpAllowDestructiveOperations);
    sec.ClearApprovedRoots();
    for (const auto& r : m_config.mcpApprovedRoots) {
        sec.AddApprovedRoot(r);
    }

    // Initialize Remote Controller policy and pairing manager from settings
    auto& remoteSec = remote::RemoteSecurityPolicy::Instance();
    remoteSec.SetEnabled(m_config.remoteEnabled);
    remoteSec.SetBindAddress(m_config.remoteBindAddress);
    remoteSec.SetServerPort(m_config.remotePort);
    remote::RemotePairingManager::Instance().ImportDevicesJson(m_config.remotePairedDevicesJson);

    if (m_config.remoteEnabled) {
        remote::RemoteControllerServer::Instance().Start(m_config.remoteBindAddress, m_config.remotePort);
    }

    std::filesystem::path recentFile = appDataDir / "recent_files.json";
    m_recentFiles = std::make_unique<persistence::RecentFilesManager>(recentFile, m_config.maxRecentFiles);
    m_recentFiles->Load();
    m_recentFiles->RemoveMissingFiles();

    // Initialize PDF Engine and Lifecycle Service
    m_engine = std::make_shared<pdf_adapters::PdfiumAdapter>();
    m_engine->Initialize();
    m_lifecycleService = std::make_unique<services::DocumentLifecycleService>(m_engine);

    m_initialized = true;
    LOG_INFO("App", "Application context initialized successfully with Google PDFium engine.");
    return true;
}

void AppContext::Shutdown() {
    if (!m_initialized) return;

    remote::RemoteControllerServer::Instance().Stop();

    SaveCurrentState();
    LOG_INFO("App", "Shutting down application context.");

    m_lifecycleService.reset();
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }

    Logger::Instance().Shutdown();
    m_initialized = false;
}

bool AppContext::SaveCurrentState() {
    if (!m_initialized) return false;
    bool s1 = m_settings->SaveSettings(m_config);
    bool s2 = m_recentFiles->Save();
    return s1 && s2;
}

} // namespace pdfinity::app
