// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteCommandDispatcher.hpp"
#include "RemoteSecurityPolicy.hpp"
#include "services/BatchProcessingService.hpp"
#include "services/BatchProfileManager.hpp"
#include "application/Logger.hpp"

namespace pdfinity::remote {

RemoteCommandDispatcher& RemoteCommandDispatcher::Instance() {
    static RemoteCommandDispatcher instance;
    return instance;
}

void RemoteCommandDispatcher::SetExecutionHooks(const ExecutionHooks& hooks) {
    m_hooks = hooks;
}

CommandExecutionResult RemoteCommandDispatcher::Execute(const std::string& commandName,
                                                        const QJsonObject& params,
                                                        const RemoteDeviceInfo& clientDevice) {
    RemoteCommand cmd = StringToRemoteCommand(commandName);

    // 1. Command Allowlist Check
    if (cmd == RemoteCommand::Unknown) {
        RemoteSecurityPolicy::Instance().RecordAudit(
            clientDevice.deviceId, clientDevice.clientName, clientDevice.lastSeenIp,
            commandName, "BLOCKED", "Command is not in allowlist (Method not found)"
        );
        return CommandExecutionResult::Error(-32601, "Method not found: " + commandName + " is not an approved Remote Controller command.");
    }

    // 2. Sensitive Action Confirmation Gate
    if (IsSensitiveRemoteCommand(cmd)) {
        std::string details;
        if (cmd == RemoteCommand::CloseDocument) {
            details = "Request to close active document.";
        } else if (cmd == RemoteCommand::BatchStart) {
            details = "Request to start batch profile: " + params.value("profileId").toString().toStdString();
        } else if (cmd == RemoteCommand::JobCancel) {
            details = "Request to cancel active batch job: " + params.value("jobId").toString().toStdString();
        }

        bool confirmed = RemoteSecurityPolicy::Instance().RequestHostConfirmation(
            clientDevice.clientName, commandName, details
        );

        if (!confirmed) {
            RemoteSecurityPolicy::Instance().RecordAudit(
                clientDevice.deviceId, clientDevice.clientName, clientDevice.lastSeenIp,
                commandName, "DENIED", "Host desktop operator rejected or timed out confirmation dialog"
            );
            return CommandExecutionResult::Error(-32001, "Action requires operator confirmation on host desktop; request was rejected or timed out.");
        }

        RemoteSecurityPolicy::Instance().RecordAudit(
            clientDevice.deviceId, clientDevice.clientName, clientDevice.lastSeenIp,
            commandName, "CONFIRMED", "Host desktop operator approved sensitive action: " + details
        );
    }

    // 3. Execution Dispatch
    QJsonObject result;

    if (cmd == RemoteCommand::Status) {
        if (m_hooks.getStatus) {
            result = m_hooks.getStatus();
        } else {
            result["hasDocument"] = false;
            result["status"] = "idle";
        }
    } else if (cmd == RemoteCommand::GotoPage) {
        if (!params.contains("page") || !params.value("page").isDouble()) {
            return CommandExecutionResult::Error(-32602, "Invalid params: 'page' parameter must be a positive integer.");
        }
        int page = params.value("page").toInt(0);
        if (page < 1) {
            return CommandExecutionResult::Error(-32602, "Invalid params: 'page' must be >= 1.");
        }
        bool ok = m_hooks.gotoPage ? m_hooks.gotoPage(page) : true;
        result["status"] = ok ? "success" : "failed";
        result["requestedPage"] = page;
    } else if (cmd == RemoteCommand::NextPage) {
        bool ok = m_hooks.nextPage ? m_hooks.nextPage() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::PrevPage) {
        bool ok = m_hooks.prevPage ? m_hooks.prevPage() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::FirstPage) {
        bool ok = m_hooks.firstPage ? m_hooks.firstPage() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::LastPage) {
        bool ok = m_hooks.lastPage ? m_hooks.lastPage() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::SetZoom) {
        double level = 1.0;
        if (params.contains("level")) {
            level = params.value("level").toDouble(0.0);
        } else if (params.contains("zoom")) {
            level = params.value("zoom").toDouble(0.0);
        }
        if (level < 0.05 || level > 10.0) {
            return CommandExecutionResult::Error(-32602, "Invalid params: zoom scale must be between 0.1 (10%) and 10.0 (1000%).");
        }
        std::string mode = params.value("mode").toString().toStdString();
        bool ok = m_hooks.setZoom ? m_hooks.setZoom(level, mode) : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::Search) {
        std::string query = params.value("query").toString().toStdString();
        bool matchCase = params.value("matchCase").toBool(false);
        if (m_hooks.search) {
            result = m_hooks.search(query, matchCase);
        } else {
            result["query"] = QString::fromStdString(query);
            result["matches"] = 0;
        }
    } else if (cmd == RemoteCommand::Thumbnail) {
        int page = params.value("page").toInt(1);
        int width = params.value("width").toInt(200);
        std::string b64 = m_hooks.getThumbnailBase64 ? m_hooks.getThumbnailBase64(page, width) : "";
        result["page"] = page;
        result["mimeType"] = "image/png";
        result["base64"] = QString::fromStdString(b64);
    } else if (cmd == RemoteCommand::JobStatus) {
        bool running = services::BatchProcessingService::Instance().IsJobRunning();
        auto state = services::BatchProcessingService::Instance().GetCurrentJobState();
        result["hasActiveJob"] = running;
        result["state"] = QString::fromStdString(domain::BatchJobStateToString(state));
    } else if (cmd == RemoteCommand::JobPause) {
        services::BatchProcessingService::Instance().PauseCurrentJob();
        result["status"] = "paused";
    } else if (cmd == RemoteCommand::JobResume) {
        services::BatchProcessingService::Instance().ResumeCurrentJob();
        result["status"] = "resumed";
    } else if (cmd == RemoteCommand::JobCancel) {
        services::BatchProcessingService::Instance().CancelCurrentJob();
        result["status"] = "cancelled";
    } else if (cmd == RemoteCommand::ToggleFullscreen) {
        bool ok = m_hooks.toggleFullscreen ? m_hooks.toggleFullscreen() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::CloseDocument) {
        bool ok = m_hooks.closeDocument ? m_hooks.closeDocument() : true;
        result["status"] = ok ? "success" : "failed";
    } else if (cmd == RemoteCommand::BatchStart) {
        std::string profileId = params.value("profileId").toString().toStdString();
        std::string inDir = params.value("inputDir").toString().toStdString();
        std::string outDir = params.value("outputDir").toString().toStdString();

        auto profile = services::BatchProfileManager::Instance().FindProfile(profileId);
        if (!profile.has_value()) {
            return CommandExecutionResult::Error(-32602, "Unknown batch profile ID: " + profileId);
        }

        domain::BatchJobConfig config;
        config.profile = profile.value();
        config.outputDirectory = outDir;
        if (!inDir.empty()) {
            config.inputFiles = services::BatchProcessingService::CollectInputFiles({inDir}, true);
        }

        std::string jobId = services::BatchProcessingService::Instance().StartJobAsync(config);
        result["status"] = "started";
        result["jobId"] = QString::fromStdString(jobId);
    } else if (cmd == RemoteCommand::Disconnect) {
        result["status"] = "disconnected";
    }

    if (!IsSensitiveRemoteCommand(cmd)) {
        RemoteSecurityPolicy::Instance().RecordAudit(
            clientDevice.deviceId, clientDevice.clientName, clientDevice.lastSeenIp,
            commandName, "ALLOWED", "Executed successfully"
        );
    }

    return CommandExecutionResult::Success(result);
}

} // namespace pdfinity::remote
