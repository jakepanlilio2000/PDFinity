// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "TransformationJobQueue.hpp"
#include "application/Logger.hpp"

namespace pdfinity::services {

TransformationJobQueue& TransformationJobQueue::Instance() {
    static TransformationJobQueue instance;
    return instance;
}

std::string TransformationJobQueue::EnqueueJob(
    const std::string& description,
    std::function<pdf_api::EngineResult<void>(std::atomic<bool>& cancelToken, std::function<void(int current, int total, const std::string&)> progressCb)> taskFunc
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = "job-" + std::to_string(m_jobCounter.fetch_add(1));

    TransformationJobTask task;
    task.jobId = id;
    task.description = description;
    task.executeFunc = std::move(taskFunc);
    task.status = pdf_api::JobStatus::Queued;
    task.progress = {0, 100, "Queued"};

    m_jobs[id] = std::move(task);
    m_cancelTokens[id] = std::make_shared<std::atomic<bool>>(false);

    LOG_INFO("JobQueue", "Enqueued job: " + id + " (" + description + ")");
    return id;
}

pdf_api::EngineResult<void> TransformationJobQueue::RunJobSynchronously(const std::string& jobId) {
    std::function<pdf_api::EngineResult<void>(std::atomic<bool>& cancelToken, std::function<void(int current, int total, const std::string&)> progressCb)> execFunc;
    std::shared_ptr<std::atomic<bool>> token;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it == m_jobs.end()) {
            return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Job not found: " + jobId);
        }
        execFunc = it->second.executeFunc;
        auto tokenIt = m_cancelTokens.find(jobId);
        if (tokenIt != m_cancelTokens.end() && tokenIt->second) {
            token = tokenIt->second;
        } else {
            token = std::make_shared<std::atomic<bool>>(false);
            m_cancelTokens[jobId] = token;
        }
        it->second.status = pdf_api::JobStatus::Running;
        it->second.progress = {0, 100, "Running"};
    }

    if (!execFunc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Job execution function is not set");
    }

    auto progressCallback = [this, jobId](int cur, int tot, const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it != m_jobs.end()) {
            it->second.progress = {cur, tot, msg};
        }
    };

    auto res = execFunc(*token, progressCallback);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it != m_jobs.end()) {
            if (token->load()) {
                it->second.status = pdf_api::JobStatus::Cancelled;
                it->second.progress = {0, 100, "Cancelled by user"};
            } else if (res.has_value()) {
                it->second.status = pdf_api::JobStatus::Completed;
                it->second.progress = {100, 100, "Completed"};
            } else {
                it->second.status = pdf_api::JobStatus::Failed;
                it->second.errorMessage = res.errorMessage;
                it->second.progress = {0, 100, "Failed: " + res.errorMessage};
            }
        }
    }

    return res;
}

void TransformationJobQueue::CancelJob(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cancelTokens.find(jobId);
    if (it != m_cancelTokens.end() && it->second) {
        it->second->store(true);
    }
    auto jobIt = m_jobs.find(jobId);
    if (jobIt != m_jobs.end()) {
        jobIt->second.status = pdf_api::JobStatus::Cancelled;
    }
}

void TransformationJobQueue::CancelAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, token] : m_cancelTokens) {
        if (token) token->store(true);
    }
    for (auto& [id, task] : m_jobs) {
        if (task.status == pdf_api::JobStatus::Queued || task.status == pdf_api::JobStatus::Running) {
            task.status = pdf_api::JobStatus::Cancelled;
        }
    }
}

pdf_api::JobStatus TransformationJobQueue::GetJobStatus(const std::string& jobId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_jobs.find(jobId);
    if (it != m_jobs.end()) return it->second.status;
    return pdf_api::JobStatus::Failed;
}

pdf_api::JobProgress TransformationJobQueue::GetJobProgress(const std::string& jobId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_jobs.find(jobId);
    if (it != m_jobs.end()) return it->second.progress;
    return {0, 100, "Unknown"};
}

std::string TransformationJobQueue::GetJobError(const std::string& jobId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_jobs.find(jobId);
    if (it != m_jobs.end()) return it->second.errorMessage;
    return "Job not found";
}

std::vector<std::string> TransformationJobQueue::GetAllJobIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_jobs.size());
    for (const auto& [id, _] : m_jobs) {
        ids.push_back(id);
    }
    return ids;
}

void TransformationJobQueue::ClearCompletedJobs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_jobs.begin(); it != m_jobs.end();) {
        if (it->second.status == pdf_api::JobStatus::Completed ||
            it->second.status == pdf_api::JobStatus::Cancelled ||
            it->second.status == pdf_api::JobStatus::Failed) {
            m_cancelTokens.erase(it->first);
            it = m_jobs.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace pdfinity::services
