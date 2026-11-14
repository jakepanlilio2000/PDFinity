// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchProcessingService.hpp"
#include "BatchJobHistory.hpp"
#include "OptimizationService.hpp"
#include "OcrService.hpp"
#include "PdfAComplianceService.hpp"
#include "SanitizationService.hpp"
#include "ConversionService.hpp"
#include "application/AppContext.hpp"
#include "application/Logger.hpp"

#include <QTemporaryDir>
#include <QDateTime>
#include <QThreadPool>
#include <QRunnable>
#include <fstream>
#include <chrono>
#include <thread>
#include <regex>
#include <algorithm>

namespace pdfinity::services {

namespace {

std::string GetCurrentIsoTime() {
    return QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
}

std::string FormatTimestamp() {
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz").toStdString();
}

void LogToFileItem(domain::BatchFileItem& item, const std::string& level, const std::string& msg) {
    domain::BatchFileLogEntry entry;
    entry.timestamp = FormatTimestamp();
    entry.level = level;
    entry.message = msg;
    item.logs.push_back(entry);
    if (level == "ERROR") {
        LOG_ERROR("Batch", "[" + item.inputPath.filename().string() + "] " + msg);
    } else if (level == "WARN") {
        LOG_WARN("Batch", "[" + item.inputPath.filename().string() + "] " + msg);
    } else {
        LOG_INFO("Batch", "[" + item.inputPath.filename().string() + "] " + msg);
    }
}

} // namespace

BatchProcessingService& BatchProcessingService::Instance() {
    static BatchProcessingService instance;
    return instance;
}

BatchProcessingService::BatchProcessingService() {
    m_engine = app::AppContext::Instance().GetPdfEngine();
}

std::vector<std::filesystem::path> BatchProcessingService::CollectInputFiles(
    const std::vector<std::filesystem::path>& paths,
    bool recursive
) {
    std::vector<std::filesystem::path> result;
    for (const auto& p : paths) {
        if (!std::filesystem::exists(p)) continue;

        if (std::filesystem::is_directory(p)) {
            if (recursive) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(p)) {
                    if (entry.is_regular_file()) {
                        auto ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".pdf") {
                            result.push_back(entry.path());
                        }
                    }
                }
            } else {
                for (const auto& entry : std::filesystem::directory_iterator(p)) {
                    if (entry.is_regular_file()) {
                        auto ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".pdf") {
                            result.push_back(entry.path());
                        }
                    }
                }
            }
        } else if (std::filesystem::is_regular_file(p)) {
            result.push_back(p);
        }
    }
    return result;
}

std::filesystem::path BatchProcessingService::ResolveOutputPath(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputDir,
    const std::string& filenameTemplate,
    domain::CollisionPolicy collisionPolicy
) {
    std::string tpl = filenameTemplate.empty() ? "{basename}_processed.pdf" : filenameTemplate;
    std::string stem = inputPath.stem().string();
    std::string ext = inputPath.extension().string();
    if (ext.empty()) ext = ".pdf";

    std::string dateStr = QDateTime::currentDateTime().toString("yyyyMMdd").toStdString();
    std::string timeStr = QDateTime::currentDateTime().toString("hhmmss").toStdString();

    std::string resolved = tpl;
    auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t start = 0;
        while ((start = str.find(from, start)) != std::string::npos) {
            str.replace(start, from.length(), to);
            start += to.length();
        }
    };

    replaceAll(resolved, "{basename}", stem);
    replaceAll(resolved, "{name}", stem);
    replaceAll(resolved, "{ext}", ext);
    replaceAll(resolved, "{date}", dateStr);
    replaceAll(resolved, "{time}", timeStr);

    // If resolved has no extension, append original extension
    if (std::filesystem::path(resolved).extension().empty()) {
        resolved += ext;
    }

    std::filesystem::path target = outputDir / resolved;

    // Source immutability guard: Never allow resolving to the exact input file
    std::error_code eqEc;
    bool isSameAsInput = false;
    if (std::filesystem::exists(target) && std::filesystem::exists(inputPath)) {
        isSameAsInput = std::filesystem::equivalent(target, inputPath, eqEc);
    } else {
        isSameAsInput = (target == inputPath);
    }

    if (isSameAsInput) {
        // Enforce safe suffix so source is never overwritten
        std::string baseStem = inputPath.stem().string();
        std::string baseExt = inputPath.extension().string();
        int counter = 1;
        while (true) {
            std::filesystem::path candidate = outputDir / (baseStem + "_processed_" + std::to_string(counter) + baseExt);
            if (!std::filesystem::exists(candidate)) {
                return candidate;
            }
            counter++;
        }
    }

    if (!std::filesystem::exists(target)) {
        return target;
    }

    if (collisionPolicy == domain::CollisionPolicy::Overwrite || collisionPolicy == domain::CollisionPolicy::Fail || collisionPolicy == domain::CollisionPolicy::Skip) {
        return target;
    }

    // AutoRename: append _1, _2, etc.
    std::string baseStem = target.stem().string();
    std::string baseExt = target.extension().string();
    int counter = 1;
    while (true) {
        std::filesystem::path candidate = outputDir / (baseStem + "_" + std::to_string(counter) + baseExt);
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
        counter++;
    }
}

domain::BatchJobSummary BatchProcessingService::RunJobSynchronously(
    const domain::BatchJobConfig& config,
    ProgressCallback progressCb
) {
    m_isRunning = true;
    m_cancelRequested = false;
    m_pauseRequested = false;
    m_currentState = domain::BatchJobState::Running;

    domain::BatchJobSummary summary;
    summary.jobId = config.jobId.empty() ? ("job_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) : config.jobId;
    summary.jobName = config.jobName.empty() ? "Batch Processing Job" : config.jobName;
    summary.profileName = config.profile.name;
    summary.startTime = GetCurrentIsoTime();
    summary.totalFiles = static_cast<int>(config.inputFiles.size());
    summary.state = domain::BatchJobState::Running;

    // Ensure output directory exists
    if (!config.outputDirectory.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(config.outputDirectory, ec);
    }

    // Initialize file items
    for (size_t i = 0; i < config.inputFiles.size(); ++i) {
        domain::BatchFileItem item;
        item.fileId = summary.jobId + "_file_" + std::to_string(i + 1);
        item.inputPath = config.inputFiles[i];
        item.outputPath = ResolveOutputPath(
            item.inputPath, config.outputDirectory, config.filenameTemplate, config.collisionPolicy
        );
        item.status = domain::BatchFileStatus::Pending;
        item.totalOps = static_cast<int>(config.profile.operations.size());

        if (std::filesystem::exists(item.inputPath)) {
            item.inputSizeBytes = std::filesystem::file_size(item.inputPath);
        }

        summary.fileItems.push_back(item);
    }

    auto jobStartClock = std::chrono::steady_clock::now();

    // Process files with bounded concurrency and memory limit protection
    int concurrency = std::clamp(config.maxConcurrency, 1, 8);
    uint64_t memoryLimitBytes = config.maxMemoryLimitMb > 0
        ? (static_cast<uint64_t>(config.maxMemoryLimitMb) * 1024ULL * 1024ULL)
        : (1024ULL * 1024ULL * 1024ULL);

    if (config.maxFileSizeBytes > 0) {
        memoryLimitBytes = config.maxFileSizeBytes;
    }

    if (config.maxMemoryLimitMb > 0 && config.maxMemoryLimitMb < 512) {
        concurrency = 1; // Bound concurrency on constrained memory budgets
    }

    std::atomic<size_t> nextIndex{0};
    std::atomic<int> completedCount{0};
    std::mutex progressMutex;

    auto workerFunc = [&]() {
        while (!m_cancelRequested) {
            // Handle pause
            while (m_pauseRequested && !m_cancelRequested) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (m_cancelRequested) break;

            size_t idx = nextIndex.fetch_add(1);
            if (idx >= summary.fileItems.size()) break;

            auto& item = summary.fileItems[idx];
            item.status = domain::BatchFileStatus::Processing;

            if (progressCb) {
                std::lock_guard<std::mutex> lock(progressMutex);
                progressCb(completedCount.load(), summary.totalFiles, item);
            }

            // Memory limit guard per file
            if (item.inputSizeBytes > memoryLimitBytes) {
                item.status = domain::BatchFileStatus::Error;
                item.errorMessage = "File size (" + std::to_string(item.inputSizeBytes) +
                                    " bytes) exceeds memory/size limit of " + std::to_string(memoryLimitBytes) + " bytes";
                LogToFileItem(item, "ERROR", item.errorMessage);
                int done = completedCount.fetch_add(1) + 1;
                if (progressCb) {
                    std::lock_guard<std::mutex> lock(progressMutex);
                    progressCb(done, summary.totalFiles, item);
                }
                continue;
            }

            // Safe resume check
            bool canResumeSkip = false;
            if (config.resumeExistingOutputs && std::filesystem::exists(item.outputPath) && std::filesystem::file_size(item.outputPath) > 0) {
                if (m_engine) {
                    auto testOpen = m_engine->OpenDocument(item.outputPath);
                    if (testOpen.has_value()) {
                        m_engine->CloseDocument(testOpen.value);
                        canResumeSkip = true;
                    }
                }
            }

            if (canResumeSkip) {
                item.status = domain::BatchFileStatus::Skipped;
                item.outputSizeBytes = std::filesystem::file_size(item.outputPath);
                LogToFileItem(item, "INFO", "Output already exists and passes integrity check (safe resume).");
            } else if (config.collisionPolicy == domain::CollisionPolicy::Fail && std::filesystem::exists(item.outputPath)) {
                item.status = domain::BatchFileStatus::Error;
                item.errorMessage = "Destination file already exists (CollisionPolicy::Fail)";
                LogToFileItem(item, "ERROR", item.errorMessage);
            } else if (config.collisionPolicy == domain::CollisionPolicy::Skip && std::filesystem::exists(item.outputPath)) {
                item.status = domain::BatchFileStatus::Skipped;
                item.outputSizeBytes = std::filesystem::file_size(item.outputPath);
                LogToFileItem(item, "INFO", "Destination file already exists (CollisionPolicy::Skip).");
            } else {
                auto res = ProcessSingleFile(item, config.profile, config.maxRetries);
                if (!res.has_value()) {
                    item.status = domain::BatchFileStatus::Error;
                    item.errorMessage = res.errorMessage;
                }
            }

            int done = completedCount.fetch_add(1) + 1;
            if (progressCb) {
                std::lock_guard<std::mutex> lock(progressMutex);
                progressCb(done, summary.totalFiles, item);
            }
        }
    };

    if (concurrency <= 1 || summary.fileItems.size() <= 1) {
        workerFunc();
    } else {
        std::vector<std::thread> workers;
        for (int c = 0; c < concurrency; ++c) {
            workers.emplace_back(workerFunc);
        }
        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }
    }

    auto jobEndClock = std::chrono::steady_clock::now();
    summary.totalDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(jobEndClock - jobStartClock).count();
    summary.finishTime = GetCurrentIsoTime();

    // Tally results
    for (const auto& item : summary.fileItems) {
        summary.totalInputBytes += item.inputSizeBytes;
        summary.totalOutputBytes += item.outputSizeBytes;
        if (item.status == domain::BatchFileStatus::Success) {
            summary.succeededFiles++;
        } else if (item.status == domain::BatchFileStatus::Error) {
            summary.failedFiles++;
        } else if (item.status == domain::BatchFileStatus::Skipped) {
            summary.skippedFiles++;
        }
    }

    if (m_cancelRequested) {
        summary.state = domain::BatchJobState::Cancelled;
    } else if (summary.failedFiles > 0 && summary.succeededFiles == 0) {
        summary.state = domain::BatchJobState::Failed;
    } else if (summary.failedFiles > 0) {
        summary.state = domain::BatchJobState::Failed; // partial failure recorded
    } else {
        summary.state = domain::BatchJobState::Succeeded;
    }

    m_currentState = summary.state;
    m_isRunning = false;

    // Record in persistent history
    BatchJobHistory::Instance().RecordJob(summary);

    LOG_INFO("Batch", "Batch job finished: " + summary.jobId + " (" +
             std::to_string(summary.succeededFiles) + " succeeded, " +
             std::to_string(summary.failedFiles) + " failed, " +
             std::to_string(summary.skippedFiles) + " skipped).");

    return summary;
}

std::string BatchProcessingService::StartJobAsync(
    const domain::BatchJobConfig& config,
    ProgressCallback progressCb,
    std::function<void(const domain::BatchJobSummary&)> finishCb
) {
    std::string jobId = config.jobId.empty() ? ("job_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())) : config.jobId;
    domain::BatchJobConfig cfg = config;
    cfg.jobId = jobId;

    std::thread([this, cfg, progressCb, finishCb]() {
        auto summary = RunJobSynchronously(cfg, progressCb);
        if (finishCb) {
            finishCb(summary);
        }
    }).detach();

    return jobId;
}

pdf_api::EngineResult<void> BatchProcessingService::ProcessSingleFile(
    domain::BatchFileItem& item,
    const domain::BatchProfile& profile,
    int maxRetries
) {
    auto fileStartClock = std::chrono::steady_clock::now();
    LogToFileItem(item, "INFO", "Starting processing with profile: " + profile.name);

    if (!std::filesystem::exists(item.inputPath)) {
        LogToFileItem(item, "ERROR", "Input file does not exist: " + item.inputPath.string());
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Input file does not exist");
    }

    // Source immutability guard: Destination path must not be equivalent to source file
    std::error_code eqEc;
    if (item.inputPath == item.outputPath ||
        (std::filesystem::exists(item.inputPath) && std::filesystem::exists(item.outputPath) &&
         std::filesystem::equivalent(item.inputPath, item.outputPath, eqEc))) {
        LogToFileItem(item, "ERROR", "Source immutability violation: destination is identical to source document");
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Source immutability violation: destination is identical to source file");
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        LogToFileItem(item, "ERROR", "Failed to create temporary sandbox directory");
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to create temporary directory");
    }

    std::filesystem::path currentPath = std::filesystem::path(tempDir.path().toStdString()) / "step_0.pdf";
    std::error_code ec;
    std::filesystem::copy_file(item.inputPath, currentPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        LogToFileItem(item, "ERROR", "Failed to copy input file to temporary sandbox: " + ec.message());
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, ec.message());
    }

    // Execute chain of operations
    for (size_t opIdx = 0; opIdx < profile.operations.size(); ++opIdx) {
        if (m_cancelRequested) {
            LogToFileItem(item, "WARN", "Operation cancelled by user request.");
            item.status = domain::BatchFileStatus::Error;
            return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Cancelled");
        }

        const auto& op = profile.operations[opIdx];
        item.currentOpIndex = static_cast<int>(opIdx + 1);
        std::filesystem::path nextPath = std::filesystem::path(tempDir.path().toStdString()) / ("step_" + std::to_string(opIdx + 1) + ".pdf");

        LogToFileItem(item, "INFO", "Executing operation " + std::to_string(opIdx + 1) + "/" +
                      std::to_string(profile.operations.size()) + ": " + op.name);

        pdf_api::EngineResult<void> opResult = pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown);
        int attempts = 0;
        int maxAttempts = std::max(1, maxRetries + 1);

        while (attempts < maxAttempts) {
            attempts++;
            opResult = ExecuteOperation(currentPath, nextPath, op, item);
            if (opResult.has_value()) {
                break;
            }
            if (attempts < maxAttempts) {
                item.retryCount++;
                LogToFileItem(item, "WARN", "Operation failed (attempt " + std::to_string(attempts) + "/" +
                              std::to_string(maxAttempts) + "): " + opResult.errorMessage + " - retrying...");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        if (!opResult.has_value()) {
            LogToFileItem(item, "ERROR", "Operation failed permanently: " + opResult.errorMessage);
            return opResult;
        }

        currentPath = nextPath;
    }

    // Ensure parent directory of output exists
    std::filesystem::create_directories(item.outputPath.parent_path(), ec);

    // Atomically copy final working file to outputPath
    std::filesystem::copy_file(currentPath, item.outputPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        LogToFileItem(item, "ERROR", "Failed to write final output to " + item.outputPath.string() + ": " + ec.message());
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, ec.message());
    }

    auto fileEndClock = std::chrono::steady_clock::now();
    item.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(fileEndClock - fileStartClock).count();
    item.outputSizeBytes = std::filesystem::file_size(item.outputPath);
    item.status = domain::BatchFileStatus::Success;

    LogToFileItem(item, "INFO", "File processed successfully in " + std::to_string(item.durationMs) +
                  " ms (" + std::to_string(item.inputSizeBytes) + " -> " + std::to_string(item.outputSizeBytes) + " bytes).");

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> BatchProcessingService::ExecuteOperation(
    const std::filesystem::path& currentWorkingPath,
    const std::filesystem::path& nextWorkingPath,
    const domain::BatchOperation& op,
    domain::BatchFileItem& item
) {
    if (!m_engine) {
        m_engine = app::AppContext::Instance().GetPdfEngine();
    }
    if (!m_engine) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "PDF Engine unavailable");
    }

    auto openRes = m_engine->OpenDocument(currentWorkingPath);
    if (!openRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(openRes.error, "Failed to open document: " + openRes.errorMessage);
    }
    domain::DocumentModel doc(openRes.value, m_engine, currentWorkingPath);

    switch (op.type) {
        case domain::BatchOperationType::Optimize: {
            pdf_api::OptimizationOptions opt;
            auto itPreset = op.parameters.find("preset");
            if (itPreset != op.parameters.end()) {
                if (itPreset->second == "Prepress") opt.preset = pdf_api::CompressionPreset::PrepressLossless;
                else if (itPreset->second == "Screen") opt.preset = pdf_api::CompressionPreset::MaximumScreenWeb;
                else opt.preset = pdf_api::CompressionPreset::RecommendedStandard;
            }
            auto itDpi = op.parameters.find("targetDpi");
            if (itDpi != op.parameters.end()) {
                try { opt.targetImageDpi = std::stoi(itDpi->second); } catch (...) {}
            }
            auto itQual = op.parameters.find("jpegQuality");
            if (itQual != op.parameters.end()) {
                try { opt.jpegQuality = std::stoi(itQual->second); } catch (...) {}
            }
            auto itMeta = op.parameters.find("stripMetadata");
            if (itMeta != op.parameters.end()) {
                opt.stripMetadata = (itMeta->second == "true" || itMeta->second == "1");
            }

            auto optRes = OptimizationService::Instance().OptimizePdf(doc, currentWorkingPath, nextWorkingPath, opt);
            if (!optRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(optRes.error, optRes.errorMessage);
            }
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::Ocr: {
            pdf_api::OcrOptions ocrOpt;
            auto itLang = op.parameters.find("language");
            if (itLang != op.parameters.end()) ocrOpt.language = itLang->second;
            auto itDpi = op.parameters.find("dpi");
            if (itDpi != op.parameters.end()) {
                try { ocrOpt.targetDpi = std::stoi(itDpi->second); } catch (...) {}
            }

            auto ocrRes = OcrService::Instance().ProcessDocumentOcr(doc, nextWorkingPath, ocrOpt);
            if (!ocrRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(ocrRes.error, ocrRes.errorMessage);
            }
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::ConvertToPdfA: {
            pdf_api::PdfAConversionOptions pdfaOpt;
            auto itProf = op.parameters.find("profile");
            if (itProf != op.parameters.end() && itProf->second == "PdfA2b") {
                pdfaOpt.targetProfile = pdf_api::PdfAProfile::PdfA2b;
            } else {
                pdfaOpt.targetProfile = pdf_api::PdfAProfile::PdfA1b;
            }
            pdfaOpt.embedMissingMetadata = true;
            pdfaOpt.stripProhibitedActions = true;
            pdfaOpt.injectOutputIntent = true;

            auto convRes = PdfAComplianceService::Instance().ConvertToPdfA(doc, nextWorkingPath, pdfaOpt);
            if (!convRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(convRes.error, convRes.errorMessage);
            }
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::Sanitize: {
            pdf_api::SanitizationOptions sanOpt;
            sanOpt.scrubInfoMetadata = true;
            sanOpt.scrubXmpMetadata = true;
            sanOpt.removeJavaScript = true;
            sanOpt.removeAttachments = true;
            sanOpt.removeHiddenLayers = true;

            auto sanRes = SanitizationService::ScrubMetadataAndRebuild(currentWorkingPath, nextWorkingPath, sanOpt);
            if (!sanRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(sanRes.error, sanRes.errorMessage);
            }
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::ExportImages: {
            pdf_api::PdfToImageOptions imgOpt;
            auto itFmt = op.parameters.find("format");
            if (itFmt != op.parameters.end()) {
                if (itFmt->second == "JPEG") imgOpt.format = pdf_api::ExportImageFormat::JPEG;
                else if (itFmt->second == "TIFF") imgOpt.format = pdf_api::ExportImageFormat::TIFF;
                else if (itFmt->second == "BMP") imgOpt.format = pdf_api::ExportImageFormat::BMP;
                else imgOpt.format = pdf_api::ExportImageFormat::PNG;
            }
            auto itDpi = op.parameters.find("dpi");
            if (itDpi != op.parameters.end()) {
                try { imgOpt.dpi = std::stoi(itDpi->second); } catch (...) {}
            }

            std::filesystem::path imgOutputDir = nextWorkingPath.parent_path() / (item.inputPath.stem().string() + "_images");
            std::filesystem::create_directories(imgOutputDir);
            auto exportRes = ConversionService::Instance().ExportPdfToImages(doc, imgOutputDir, item.inputPath.stem().string(), imgOpt);
            if (!exportRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(exportRes.error, exportRes.errorMessage);
            }

            // Save copy of current PDF to nextWorkingPath to keep chain intact
            doc.SaveCopy(nextWorkingPath);
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::ExportText: {
            std::filesystem::path txtOut = nextWorkingPath.parent_path() / (item.inputPath.stem().string() + ".txt");
            auto txtRes = ConversionService::Instance().ExportPdfToText(doc, txtOut);
            if (!txtRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(txtRes.error, txtRes.errorMessage);
            }

            // Save copy to nextWorkingPath
            doc.SaveCopy(nextWorkingPath);
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::FlattenForms: {
            bool forPrint = false;
            auto itPrint = op.parameters.find("forPrint");
            if (itPrint != op.parameters.end()) {
                forPrint = (itPrint->second == "true" || itPrint->second == "1");
            }
            auto flatRes = doc.FlattenDocument(forPrint);
            if (!flatRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(flatRes.error, flatRes.errorMessage);
            }
            auto saveRes = doc.SaveCopy(nextWorkingPath);
            if (!saveRes.has_value()) {
                return pdf_api::EngineResult<void>::Fail(saveRes.error, saveRes.errorMessage);
            }
            return pdf_api::EngineResult<void>::Ok();
        }

        case domain::BatchOperationType::WatermarkStamp:
        default: {
            return pdf_api::EngineResult<void>::Fail(
                pdf_api::EngineError::UnsupportedFeature,
                "The requested operation type is not supported in this version."
            );
        }
    }
}

void BatchProcessingService::PauseCurrentJob() {
    m_pauseRequested = true;
    m_currentState = domain::BatchJobState::Paused;
    LOG_INFO("Batch", "Batch job pause requested.");
}

void BatchProcessingService::ResumeCurrentJob() {
    m_pauseRequested = false;
    m_currentState = domain::BatchJobState::Running;
    LOG_INFO("Batch", "Batch job resumed.");
}

void BatchProcessingService::CancelCurrentJob() {
    m_cancelRequested = true;
    m_pauseRequested = false;
    m_currentState = domain::BatchJobState::Cancelled;
    LOG_WARN("Batch", "Batch job cancellation requested.");
}

bool BatchProcessingService::IsJobRunning() const {
    return m_isRunning;
}

domain::BatchJobState BatchProcessingService::GetCurrentJobState() const {
    return m_currentState;
}

} // namespace pdfinity::services
