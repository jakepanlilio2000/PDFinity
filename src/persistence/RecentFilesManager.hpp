// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <mutex>
#include <cstdint>

namespace pdfinity::persistence {

struct RecentFileItem {
    std::string canonicalPath;
    std::string fileName;
    std::string lastOpenedIso;
    uint64_t fileSizeBytes{0};
};

class RecentFilesManager {
public:
    RecentFilesManager(std::filesystem::path filePath, int maxEntries = 10);

    bool AddFile(const std::filesystem::path& path);
    bool RemoveFile(const std::filesystem::path& path);
    void Clear();
    void RemoveMissingFiles();

    std::vector<RecentFileItem> GetRecentFiles() const;

    bool Load();
    bool Save();

private:
    std::filesystem::path m_storagePath;
    int m_maxEntries{10};
    std::vector<RecentFileItem> m_items;
    mutable std::mutex m_mutex;
};

} // namespace pdfinity::persistence
