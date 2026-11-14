// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>

namespace pdfinity::services {

struct TransformationJobTask {
    std::string jobId;
    std::string description;
    std::function<pdf_api::EngineResult<void>(std::atomic<bool>& cancelToken, std::function<void(int current, int total, const std::string&)> progressCb)> executeFunc;
    pdf_api::JobStatus status{pdf_api::JobStatus::Queued};
    pdf_api::JobProgress progress{0, 100, "Queued"};
    std::string errorMessage;
};

class TransformationJobQueue {
public:
    static TransformationJobQueue& Instance();

    std::string EnqueueJob(
        const std::string& description,
        std::function<pdf_api::EngineResult<void>(std::atomic<bool>& cancelToken, std::function<void(int current, int total, const std::string&)> progressCb)> taskFunc
    );

    pdf_api::EngineResult<void> RunJobSynchronously(const std::string& jobId);

    void CancelJob(const std::string& jobId);
    void CancelAll();

    pdf_api::JobStatus GetJobStatus(const std::string& jobId) const;
    pdf_api::JobProgress GetJobProgress(const std::string& jobId) const;
    std::string GetJobError(const std::string& jobId) const;
    std::vector<std::string> GetAllJobIds() const;
    void ClearCompletedJobs();

private:
    TransformationJobQueue() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, TransformationJobTask> m_jobs;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> m_cancelTokens;
    std::atomic<uint64_t> m_jobCounter{1};
};

} // namespace pdfinity::services
