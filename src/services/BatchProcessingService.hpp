// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/BatchTypes.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_api/IPdfEngine.hpp"

#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <filesystem>
#include <vector>

namespace pdfinity::services {

class BatchProcessingService {
public:
    static BatchProcessingService& Instance();

    using ProgressCallback = std::function<void(int completedFiles, int totalFiles, const domain::BatchFileItem& currentItem)>;

    // Synchronous execution (used for CLI, tests, and workers)
    domain::BatchJobSummary RunJobSynchronously(
        const domain::BatchJobConfig& config,
        ProgressCallback progressCb = nullptr
    );

    // Asynchronous background execution (used for UI)
    std::string StartJobAsync(
        const domain::BatchJobConfig& config,
        ProgressCallback progressCb = nullptr,
        std::function<void(const domain::BatchJobSummary&)> finishCb = nullptr
    );

    void PauseCurrentJob();
    void ResumeCurrentJob();
    void CancelCurrentJob();

    bool IsJobRunning() const;
    domain::BatchJobState GetCurrentJobState() const;

    // Helper utilities for file paths & collision resolution
    static std::filesystem::path ResolveOutputPath(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputDir,
        const std::string& filenameTemplate,
        domain::CollisionPolicy collisionPolicy
    );

    static std::vector<std::filesystem::path> CollectInputFiles(
        const std::vector<std::filesystem::path>& paths,
        bool recursive = true
    );

private:
    BatchProcessingService();

    pdf_api::EngineResult<void> ProcessSingleFile(
        domain::BatchFileItem& item,
        const domain::BatchProfile& profile,
        int maxRetries
    );

    pdf_api::EngineResult<void> ExecuteOperation(
        const std::filesystem::path& currentWorkingPath,
        const std::filesystem::path& nextWorkingPath,
        const domain::BatchOperation& op,
        domain::BatchFileItem& item
    );

    mutable std::mutex m_mutex;
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_pauseRequested{false};
    std::atomic<bool> m_isRunning{false};
    domain::BatchJobState m_currentState{domain::BatchJobState::Queued};

    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

} // namespace pdfinity::services
