// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdint>

namespace pdfinity::domain {

enum class BatchJobState {
    Queued,
    Running,
    Paused,
    Succeeded,
    Failed,
    Cancelled
};

inline std::string BatchJobStateToString(BatchJobState state) {
    switch (state) {
        case BatchJobState::Queued: return "Queued";
        case BatchJobState::Running: return "Running";
        case BatchJobState::Paused: return "Paused";
        case BatchJobState::Succeeded: return "Succeeded";
        case BatchJobState::Failed: return "Failed";
        case BatchJobState::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

enum class BatchFileStatus {
    Pending,
    Processing,
    Success,
    Error,
    Skipped
};

enum class CollisionPolicy {
    AutoRename,
    Overwrite,
    Skip,
    Fail
};

enum class BatchOperationType {
    Optimize,
    Ocr,
    ConvertToPdfA,
    Sanitize,
    ExportImages,
    ExportText,
    WatermarkStamp,
    FlattenForms
};

struct BatchOperation {
    BatchOperationType type{BatchOperationType::Optimize};
    std::string name;
    std::unordered_map<std::string, std::string> parameters;
};

struct BatchProfile {
    std::string id;
    std::string name;
    std::string description;
    std::vector<BatchOperation> operations;
    bool isBuiltIn{false};
};

struct BatchFileLogEntry {
    std::string timestamp;
    std::string level; // "INFO", "WARN", "ERROR"
    std::string message;
};

struct BatchFileItem {
    std::string fileId;
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;
    BatchFileStatus status{BatchFileStatus::Pending};
    int currentOpIndex{0};
    int totalOps{0};
    uint64_t inputSizeBytes{0};
    uint64_t outputSizeBytes{0};
    int64_t durationMs{0};
    int retryCount{0};
    std::string errorMessage;
    std::vector<BatchFileLogEntry> logs;
};

struct BatchJobConfig {
    std::string jobId;
    std::string jobName;
    BatchProfile profile;
    std::vector<std::filesystem::path> inputFiles;
    std::filesystem::path outputDirectory;
    std::string filenameTemplate{"{basename}_processed.pdf"};
    CollisionPolicy collisionPolicy{CollisionPolicy::AutoRename};
    int maxConcurrency{2};
    int maxRetries{1};
    bool resumeExistingOutputs{true};
    size_t maxMemoryLimitMb{1024};
    uint64_t maxFileSizeBytes{0};
};

struct BatchJobSummary {
    std::string jobId;
    std::string jobName;
    std::string profileName;
    BatchJobState state{BatchJobState::Queued};
    std::string startTime;
    std::string finishTime;
    int totalFiles{0};
    int succeededFiles{0};
    int failedFiles{0};
    int skippedFiles{0};
    uint64_t totalInputBytes{0};
    uint64_t totalOutputBytes{0};
    int64_t totalDurationMs{0};
    std::vector<BatchFileItem> fileItems;
};

} // namespace pdfinity::domain
