// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace pdfinity::remote {

inline constexpr uint16_t DEFAULT_REMOTE_PORT = 8443;
inline constexpr int DEFAULT_PIN_LIFETIME_SECONDS = 60;
inline constexpr int DEFAULT_SESSION_IDLE_TIMEOUT_SECONDS = 1800; // 30 minutes
inline constexpr int MAX_PAIRING_FAILED_ATTEMPTS = 5;
inline constexpr int PAIRING_LOCKOUT_SECONDS = 300; // 5 minutes
inline constexpr size_t MAX_REMOTE_FRAME_BYTES = 2 * 1024 * 1024; // 2 MB

enum class RemoteCommand {
    Status,
    GotoPage,
    NextPage,
    PrevPage,
    FirstPage,
    LastPage,
    SetZoom,
    Search,
    Thumbnail,
    JobStatus,
    JobPause,
    JobResume,
    JobCancel,
    ToggleFullscreen,
    CloseDocument,
    BatchStart,
    Disconnect,
    Unknown
};

inline RemoteCommand StringToRemoteCommand(const std::string& name) {
    if (name == "status") return RemoteCommand::Status;
    if (name == "goto_page") return RemoteCommand::GotoPage;
    if (name == "next_page") return RemoteCommand::NextPage;
    if (name == "prev_page") return RemoteCommand::PrevPage;
    if (name == "first_page") return RemoteCommand::FirstPage;
    if (name == "last_page") return RemoteCommand::LastPage;
    if (name == "set_zoom") return RemoteCommand::SetZoom;
    if (name == "search") return RemoteCommand::Search;
    if (name == "thumbnail") return RemoteCommand::Thumbnail;
    if (name == "job_status") return RemoteCommand::JobStatus;
    if (name == "job_pause") return RemoteCommand::JobPause;
    if (name == "job_resume") return RemoteCommand::JobResume;
    if (name == "job_cancel") return RemoteCommand::JobCancel;
    if (name == "toggle_fullscreen") return RemoteCommand::ToggleFullscreen;
    if (name == "close_document") return RemoteCommand::CloseDocument;
    if (name == "batch_start") return RemoteCommand::BatchStart;
    if (name == "disconnect") return RemoteCommand::Disconnect;
    return RemoteCommand::Unknown;
}

inline std::string RemoteCommandToString(RemoteCommand cmd) {
    switch (cmd) {
        case RemoteCommand::Status: return "status";
        case RemoteCommand::GotoPage: return "goto_page";
        case RemoteCommand::NextPage: return "next_page";
        case RemoteCommand::PrevPage: return "prev_page";
        case RemoteCommand::FirstPage: return "first_page";
        case RemoteCommand::LastPage: return "last_page";
        case RemoteCommand::SetZoom: return "set_zoom";
        case RemoteCommand::Search: return "search";
        case RemoteCommand::Thumbnail: return "thumbnail";
        case RemoteCommand::JobStatus: return "job_status";
        case RemoteCommand::JobPause: return "job_pause";
        case RemoteCommand::JobResume: return "job_resume";
        case RemoteCommand::JobCancel: return "job_cancel";
        case RemoteCommand::ToggleFullscreen: return "toggle_fullscreen";
        case RemoteCommand::CloseDocument: return "close_document";
        case RemoteCommand::BatchStart: return "batch_start";
        case RemoteCommand::Disconnect: return "disconnect";
        case RemoteCommand::Unknown: return "unknown";
    }
    return "unknown";
}

inline bool IsSensitiveRemoteCommand(RemoteCommand cmd) {
    return (cmd == RemoteCommand::CloseDocument ||
            cmd == RemoteCommand::BatchStart ||
            cmd == RemoteCommand::JobCancel);
}

struct RemoteDeviceInfo {
    std::string deviceId;
    std::string clientName;
    std::string token;
    std::string pairedAtIso;
    std::string lastSeenIso;
    std::string lastSeenIp;
    bool isRevoked{false};
    uint64_t lastNonce{0};

    QJsonObject ToJson() const {
        QJsonObject obj;
        obj["deviceId"] = QString::fromStdString(deviceId);
        obj["clientName"] = QString::fromStdString(clientName);
        obj["token"] = QString::fromStdString(token);
        obj["pairedAtIso"] = QString::fromStdString(pairedAtIso);
        obj["lastSeenIso"] = QString::fromStdString(lastSeenIso);
        obj["lastSeenIp"] = QString::fromStdString(lastSeenIp);
        obj["isRevoked"] = isRevoked;
        obj["lastNonce"] = static_cast<qint64>(lastNonce);
        return obj;
    }

    static RemoteDeviceInfo FromJson(const QJsonObject& obj) {
        RemoteDeviceInfo info;
        info.deviceId = obj.value("deviceId").toString().toStdString();
        info.clientName = obj.value("clientName").toString().toStdString();
        info.token = obj.value("token").toString().toStdString();
        info.pairedAtIso = obj.value("pairedAtIso").toString().toStdString();
        info.lastSeenIso = obj.value("lastSeenIso").toString().toStdString();
        info.lastSeenIp = obj.value("lastSeenIp").toString().toStdString();
        info.isRevoked = obj.value("isRevoked").toBool(false);
        info.lastNonce = static_cast<uint64_t>(obj.value("lastNonce").toInteger(0));
        return info;
    }
};

struct RemoteSessionState {
    bool isConnected{false};
    std::string activeClientName;
    std::string activeDeviceId;
    std::string activeIp;
    std::string connectedAtIso;
    uint64_t requestsHandled{0};
    uint64_t commandsExecuted{0};
    uint64_t violationsCount{0};
};

struct RemoteAuditEntry {
    std::string timestamp;
    std::string deviceId;
    std::string clientName;
    std::string ip;
    std::string command;
    std::string status; // "ALLOWED", "CONFIRMED", "DENIED", "RATE_LIMITED", "BLOCKED"
    std::string details;
};

struct RemoteServerConfig {
    bool enabled{false}; // strictly off by default
    std::string bindAddress{"127.0.0.1"}; // loopback by default
    uint16_t port{DEFAULT_REMOTE_PORT};
    bool requireHostConfirmation{true};
    int idleTimeoutSeconds{DEFAULT_SESSION_IDLE_TIMEOUT_SECONDS};
    int maxRequestsPerSecond{25};
};

} // namespace pdfinity::remote
