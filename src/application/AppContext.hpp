// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "platform/IPlatformAdapter.hpp"
#include "application/AppConfig.hpp"
#include "persistence/SettingsManager.hpp"
#include "persistence/RecentFilesManager.hpp"
#include "services/DocumentLifecycleService.hpp"
#include "pdf_adapters/PdfiumAdapter.hpp"
#include <memory>
#include <filesystem>

namespace pdfinity::app {

class AppContext {
public:
    static AppContext& Instance();

    bool Initialize();
    void Shutdown();

    platform::IPlatformAdapter& GetPlatform() { return *m_platform; }
    const platform::IPlatformAdapter& GetPlatform() const { return *m_platform; }

    AppConfig& GetConfig() { return m_config; }
    const AppConfig& GetConfig() const { return m_config; }

    persistence::SettingsManager& GetSettings() { return *m_settings; }
    persistence::RecentFilesManager& GetRecentFiles() { return *m_recentFiles; }

    std::shared_ptr<pdf_api::IPdfEngine> GetPdfEngine() const { return m_engine; }
    services::DocumentLifecycleService& GetLifecycleService() { return *m_lifecycleService; }

    bool SaveCurrentState();

private:
    AppContext() = default;
    ~AppContext();

    std::unique_ptr<platform::IPlatformAdapter> m_platform;
    AppConfig m_config;
    std::unique_ptr<persistence::SettingsManager> m_settings;
    std::unique_ptr<persistence::RecentFilesManager> m_recentFiles;
    std::shared_ptr<pdf_adapters::PdfiumAdapter> m_engine;
    std::unique_ptr<services::DocumentLifecycleService> m_lifecycleService;
    bool m_initialized{false};
};

} // namespace pdfinity::app
