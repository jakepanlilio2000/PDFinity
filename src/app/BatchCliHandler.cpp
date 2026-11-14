// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchCliHandler.hpp"
#include "services/BatchProcessingService.hpp"
#include "services/BatchProfileManager.hpp"
#include "services/BatchJobHistory.hpp"
#include "application/Logger.hpp"

#include <iostream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace pdfinity::app {

bool BatchCliHandler::IsBatchCommand(int argc, char* argv[]) {
    if (argc < 2) return false;
    std::string cmd = argv[1];
    return (cmd == "batch" || cmd == "batch-profiles" || cmd == "batch-history" ||
            cmd == "--batch" || cmd == "--batch-profiles" || cmd == "--batch-history");
}

void BatchCliHandler::PrintBatchHelp() {
    std::cout << "PDFinity - Batch Processing & Automation CLI\n\n"
              << "Commands:\n"
              << "  PDFinity batch [options]             Execute automated multi-file workflow\n"
              << "  PDFinity batch-profiles [options]    Manage, list, export and import profiles\n"
              << "  PDFinity batch-history [options]     Inspect or clear job history\n\n"
              << "Batch Options:\n"
              << "  --profile <name|id>        Profile name or ID to execute (Required)\n"
              << "  --input <path...>          Input PDF files or directory paths (Required)\n"
              << "  --output <dir>             Output destination directory (Required)\n"
              << "  --collision <policy>       Collision handling: autorename (default), overwrite, skip, fail\n"
              << "  --concurrency <N>          Worker threads count (1-16, default: 2)\n"
              << "  --retries <N>              Max retry attempts per file on failure (0-3, default: 1)\n"
              << "  --template <pattern>       Filename pattern (default: {basename}_processed.pdf)\n"
              << "  --resume                   Skip files with existing valid outputs\n"
              << "  --recursive                Recursively scan input directories for .pdf files\n"
              << "  --report <path>            Export execution summary report (HTML or JSON)\n"
              << "  --json                     Output final summary to stdout as JSON\n"
              << "  -h, --help                 Display batch command help\n\n"
              << "Batch-Profiles Options:\n"
              << "  --list                     List all available built-in and custom profiles\n"
              << "  --export <name> <file>     Export profile to JSON file\n"
              << "  --import <file>            Import profile from JSON file\n\n"
              << "Batch-History Options:\n"
              << "  --list                     List recent batch job executions\n"
              << "  --clear                    Clear all job history records\n";
}

int BatchCliHandler::Execute(int argc, char* argv[]) {
    if (argc < 2) return 1;

    std::string cmd = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (cmd == "batch" || cmd == "--batch") {
        return HandleBatchProcess(args);
    } else if (cmd == "batch-profiles" || cmd == "--batch-profiles") {
        return HandleBatchProfiles(args);
    } else if (cmd == "batch-history" || cmd == "--batch-history") {
        return HandleBatchHistory(args);
    }

    PrintBatchHelp();
    return 1;
}

int BatchCliHandler::HandleBatchProfiles(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        PrintBatchHelp();
        return 0;
    }

    auto& mgr = services::BatchProfileManager::Instance();

    if (args[0] == "--list") {
        std::cout << "\n=== PDFinity Batch Profiles ===\n";
        auto profiles = mgr.GetAllProfiles();
        for (const auto& p : profiles) {
            std::cout << "\n[" << (p.isBuiltIn ? "Built-in" : "Custom") << "] " << p.name << " (ID: " << p.id << ")\n"
                      << "  Description: " << p.description << "\n"
                      << "  Operations (" << p.operations.size() << "):\n";
            for (size_t i = 0; i < p.operations.size(); ++i) {
                std::cout << "    " << (i + 1) << ". " << p.operations[i].name << "\n";
            }
        }
        std::cout << "\nTotal Profiles: " << profiles.size() << "\n\n";
        return 0;
    }

    if (args[0] == "--export") {
        if (args.size() < 3) {
            std::cerr << "Error: --export requires <profile-name-or-id> and <destination-path>\n";
            return 1;
        }
        auto p = mgr.FindProfile(args[1]);
        if (!p.has_value()) {
            std::cerr << "Error: Profile not found: " << args[1] << "\n";
            return 1;
        }
        if (!mgr.ExportProfileToJson(p.value(), args[2])) {
            std::cerr << "Error: Failed to export profile to " << args[2] << "\n";
            return 1;
        }
        std::cout << "Successfully exported profile '" << p.value().name << "' to " << args[2] << "\n";
        return 0;
    }

    if (args[0] == "--import") {
        if (args.size() < 2) {
            std::cerr << "Error: --import requires <source-file-path>\n";
            return 1;
        }
        auto p = mgr.ImportProfileFromJson(args[1]);
        if (!p.has_value()) {
            std::cerr << "Error: Failed to import profile from " << args[1] << "\n";
            return 1;
        }
        std::cout << "Successfully imported profile: " << p.value().name << " (ID: " << p.value().id << ")\n";
        return 0;
    }

    std::cerr << "Error: Unknown batch-profiles option: " << args[0] << "\n";
    return 1;
}

int BatchCliHandler::HandleBatchHistory(const std::vector<std::string>& args) {
    if (args.empty() || args[0] == "--help" || args[0] == "-h" || args[0] == "--list") {
        auto history = services::BatchJobHistory::Instance().GetRecentJobs();
        std::cout << "\n=== PDFinity Batch Execution History ===\n";
        if (history.empty()) {
            std::cout << "No past batch jobs recorded.\n\n";
            return 0;
        }
        for (const auto& job : history) {
            std::string stateStr = (job.state == domain::BatchJobState::Succeeded) ? "SUCCEEDED" :
                                   (job.state == domain::BatchJobState::Failed) ? "FAILED" : "CANCELLED";
            std::cout << "\nJob ID: " << job.jobId << " | State: " << stateStr << "\n"
                      << "  Job Name: " << job.jobName << " | Profile: " << job.profileName << "\n"
                      << "  Time: " << job.startTime << " -> " << job.finishTime << " (" << (job.totalDurationMs / 1000.0) << "s)\n"
                      << "  Files: " << job.succeededFiles << "/" << job.totalFiles << " Succeeded, "
                      << job.failedFiles << " Failed, " << job.skippedFiles << " Skipped\n";
        }
        std::cout << "\n";
        return 0;
    }

    if (args[0] == "--clear") {
        services::BatchJobHistory::Instance().ClearHistory();
        std::cout << "Batch job history cleared successfully.\n";
        return 0;
    }

    std::cerr << "Error: Unknown batch-history option: " << args[0] << "\n";
    return 1;
}

int BatchCliHandler::HandleBatchProcess(const std::vector<std::string>& args) {
    std::string profileIdOrName;
    std::vector<std::filesystem::path> inputPaths;
    std::filesystem::path outputDir;
    std::string filenameTemplate = "{basename}_processed.pdf";
    domain::CollisionPolicy collisionPolicy = domain::CollisionPolicy::AutoRename;
    int concurrency = 2;
    int retries = 1;
    bool resumeExisting = false;
    bool recursive = false;
    std::filesystem::path reportPath;
    bool outputJson = false;

    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "--help" || arg == "-h") {
            PrintBatchHelp();
            return 0;
        } else if (arg == "--profile") {
            if (++i >= args.size()) { std::cerr << "Error: --profile requires an argument\n"; return 1; }
            profileIdOrName = args[i];
        } else if (arg == "--input") {
            if (++i >= args.size()) { std::cerr << "Error: --input requires an argument\n"; return 1; }
            while (i < args.size() && !args[i].starts_with("-")) {
                inputPaths.push_back(args[i]);
                i++;
            }
            i--; // back up one
        } else if (arg == "--output") {
            if (++i >= args.size()) { std::cerr << "Error: --output requires a directory path\n"; return 1; }
            outputDir = args[i];
        } else if (arg == "--template") {
            if (++i >= args.size()) { std::cerr << "Error: --template requires an argument\n"; return 1; }
            filenameTemplate = args[i];
        } else if (arg == "--collision") {
            if (++i >= args.size()) { std::cerr << "Error: --collision requires an argument\n"; return 1; }
            std::string c = args[i];
            std::transform(c.begin(), c.end(), c.begin(), ::tolower);
            if (c == "overwrite") collisionPolicy = domain::CollisionPolicy::Overwrite;
            else if (c == "skip") collisionPolicy = domain::CollisionPolicy::Skip;
            else if (c == "fail") collisionPolicy = domain::CollisionPolicy::Fail;
            else collisionPolicy = domain::CollisionPolicy::AutoRename;
        } else if (arg == "--concurrency") {
            if (++i >= args.size()) { std::cerr << "Error: --concurrency requires a number\n"; return 1; }
            try { concurrency = std::clamp(std::stoi(args[i]), 1, 16); } catch (...) {}
        } else if (arg == "--retries") {
            if (++i >= args.size()) { std::cerr << "Error: --retries requires a number\n"; return 1; }
            try { retries = std::clamp(std::stoi(args[i]), 0, 3); } catch (...) {}
        } else if (arg == "--resume") {
            resumeExisting = true;
        } else if (arg == "--recursive") {
            recursive = true;
        } else if (arg == "--report") {
            if (++i >= args.size()) { std::cerr << "Error: --report requires a destination path\n"; return 1; }
            reportPath = args[i];
        } else if (arg == "--json") {
            outputJson = true;
        }
    }

    if (profileIdOrName.empty()) {
        std::cerr << "Error: --profile is required. Use 'PDFinity batch-profiles --list' to see available profiles.\n";
        return 1;
    }
    if (inputPaths.empty()) {
        std::cerr << "Error: --input is required.\n";
        return 1;
    }
    if (outputDir.empty()) {
        std::cerr << "Error: --output is required.\n";
        return 1;
    }

    auto profileOpt = services::BatchProfileManager::Instance().FindProfile(profileIdOrName);
    if (!profileOpt.has_value()) {
        std::cerr << "Error: Profile '" << profileIdOrName << "' not found.\n";
        return 1;
    }

    auto files = services::BatchProcessingService::CollectInputFiles(inputPaths, recursive);
    if (files.empty()) {
        std::cerr << "Error: No matching PDF input files found.\n";
        return 1;
    }

    domain::BatchJobConfig config;
    config.jobName = "CLI Batch: " + profileOpt.value().name;
    config.profile = profileOpt.value();
    config.inputFiles = files;
    config.outputDirectory = outputDir;
    config.filenameTemplate = filenameTemplate;
    config.collisionPolicy = collisionPolicy;
    config.maxConcurrency = concurrency;
    config.maxRetries = retries;
    config.resumeExistingOutputs = resumeExisting;

    if (!outputJson) {
        std::cout << "Starting PDFinity Batch Processing...\n"
                  << "  Profile: " << config.profile.name << "\n"
                  << "  Files: " << config.inputFiles.size() << "\n"
                  << "  Output: " << config.outputDirectory.string() << "\n"
                  << "  Concurrency: " << config.maxConcurrency << " thread(s)\n\n";
    }

    auto progressCallback = [outputJson](int completed, int total, const domain::BatchFileItem& item) {
        if (!outputJson) {
            std::cout << "[" << std::setw(3) << completed << "/" << total << "] "
                      << item.inputPath.filename().string() << " -> "
                      << (item.status == domain::BatchFileStatus::Success ? "SUCCESS" :
                          item.status == domain::BatchFileStatus::Skipped ? "SKIPPED" :
                          item.status == domain::BatchFileStatus::Error ? ("FAILED: " + item.errorMessage) : "PROCESSING")
                      << "\n";
        }
    };

    auto summary = services::BatchProcessingService::Instance().RunJobSynchronously(config, progressCallback);

    // Export report if requested
    if (!reportPath.empty()) {
        std::string ext = reportPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".json") {
            services::BatchJobHistory::ExportSummaryToJson(summary, reportPath);
        } else {
            services::BatchJobHistory::ExportSummaryToHtml(summary, reportPath);
        }
        if (!outputJson) {
            std::cout << "\nExecution summary report saved to: " << reportPath.string() << "\n";
        }
    }

    if (outputJson) {
        // Output clean JSON to stdout
        std::cout << "{\n"
                  << "  \"jobId\": \"" << summary.jobId << "\",\n"
                  << "  \"status\": \"" << (summary.state == domain::BatchJobState::Succeeded ? "succeeded" : "failed") << "\",\n"
                  << "  \"totalFiles\": " << summary.totalFiles << ",\n"
                  << "  \"succeededFiles\": " << summary.succeededFiles << ",\n"
                  << "  \"failedFiles\": " << summary.failedFiles << ",\n"
                  << "  \"skippedFiles\": " << summary.skippedFiles << ",\n"
                  << "  \"durationMs\": " << summary.totalDurationMs << "\n"
                  << "}\n";
    } else {
        std::cout << "\n=== Batch Execution Completed ===\n"
                  << "  Status: " << (summary.state == domain::BatchJobState::Succeeded ? "SUCCESS" : "FAILED") << "\n"
                  << "  Succeeded: " << summary.succeededFiles << "/" << summary.totalFiles << "\n"
                  << "  Failed: " << summary.failedFiles << "\n"
                  << "  Skipped: " << summary.skippedFiles << "\n"
                  << "  Elapsed: " << (summary.totalDurationMs / 1000.0) << "s\n\n";
    }

    if (summary.failedFiles > 0) {
        return 2; // Partial or complete failure
    }
    return 0;
}

} // namespace pdfinity::app
