// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "application/AppConfig.hpp"
#include <filesystem>
#include <mutex>

namespace pdfinity::persistence {

class SettingsManager {
public:
    explicit SettingsManager(std::filesystem::path settingsFilePath);

    bool LoadSettings(app::AppConfig& config);
    bool SaveSettings(const app::AppConfig& config);

    const std::filesystem::path& GetSettingsPath() const { return m_settingsPath; }

private:
    std::filesystem::path m_settingsPath;
    mutable std::mutex m_mutex;
};

} // namespace pdfinity::persistence
