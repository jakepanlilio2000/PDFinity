// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/BatchTypes.hpp"
#include <vector>
#include <string>
#include <optional>
#include <mutex>
#include <filesystem>

namespace pdfinity::services {

class BatchProfileManager {
public:
    static BatchProfileManager& Instance();

    std::vector<domain::BatchProfile> GetAllProfiles() const;
    std::vector<domain::BatchProfile> GetBuiltInProfiles() const;
    std::vector<domain::BatchProfile> GetCustomProfiles() const;

    std::optional<domain::BatchProfile> FindProfile(const std::string& idOrName) const;

    bool SaveCustomProfile(const domain::BatchProfile& profile);
    bool DeleteCustomProfile(const std::string& profileId);

    bool ExportProfileToJson(const domain::BatchProfile& profile, const std::filesystem::path& destination) const;
    std::optional<domain::BatchProfile> ImportProfileFromJson(const std::filesystem::path& sourcePath);

    void ResetToDefaults();

private:
    BatchProfileManager();
    void InitializeBuiltInProfiles();
    void LoadCustomProfiles();
    void PersistCustomProfiles();
    std::filesystem::path GetCustomProfilesFilePath() const;

    mutable std::mutex m_mutex;
    std::vector<domain::BatchProfile> m_builtInProfiles;
    std::vector<domain::BatchProfile> m_customProfiles;
};

} // namespace pdfinity::services
