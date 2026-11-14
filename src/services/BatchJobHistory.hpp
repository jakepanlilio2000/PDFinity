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

class BatchJobHistory {
public:
    static BatchJobHistory& Instance();

    void RecordJob(const domain::BatchJobSummary& summary);
    std::vector<domain::BatchJobSummary> GetRecentJobs(int limit = 50) const;
    std::optional<domain::BatchJobSummary> GetJobSummary(const std::string& jobId) const;
    void ClearHistory();

    static bool ExportSummaryToHtml(const domain::BatchJobSummary& summary, const std::filesystem::path& destination);
    static bool ExportSummaryToJson(const domain::BatchJobSummary& summary, const std::filesystem::path& destination);

private:
    BatchJobHistory();
    void LoadHistory();
    void PersistHistory();
    std::filesystem::path GetHistoryFilePath() const;

    mutable std::mutex m_mutex;
    std::vector<domain::BatchJobSummary> m_history;
};

} // namespace pdfinity::services
