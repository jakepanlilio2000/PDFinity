// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "RemoteTypes.hpp"
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace pdfinity::remote {

struct AuthCheckResult {
    bool allowed{false};
    int errorCode{0};
    std::string errorMessage;
    RemoteDeviceInfo deviceInfo;

    static AuthCheckResult Allow(const RemoteDeviceInfo& dev) {
        return {true, 0, "", dev};
    }

    static AuthCheckResult Deny(int code, const std::string& msg) {
        return {false, code, msg, {}};
    }
};

class RemoteSecurityPolicy {
public:
    using ConfirmationCallback = std::function<bool(const std::string& clientName, const std::string& command, const std::string& details)>;

    static RemoteSecurityPolicy& Instance();

    void SetConfig(const RemoteServerConfig& config);
    RemoteServerConfig GetConfig() const;

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void SetBindAddress(const std::string& addr);
    std::string GetBindAddress() const;

    void SetServerPort(uint16_t port);
    uint16_t GetServerPort() const;

    // Authentication & Replay Protection Check
    AuthCheckResult ValidateRequest(const std::string& deviceId,
                                    const std::string& token,
                                    uint64_t nonce,
                                    int64_t timestampSec,
                                    const std::string& clientIp);

    // Rate Limiting per Device
    bool CheckRequestRateLimit(const std::string& deviceId);

    // Host Confirmation Callback for Sensitive Actions
    void SetConfirmationCallback(ConfirmationCallback cb);
    bool RequestHostConfirmation(const std::string& clientName, const std::string& command, const std::string& details);

    // Audit Logging
    void RecordAudit(const std::string& deviceId,
                     const std::string& clientName,
                     const std::string& ip,
                     const std::string& command,
                     const std::string& status,
                     const std::string& details);

    std::vector<RemoteAuditEntry> GetAuditLogs() const;
    void ClearAuditLogs();

private:
    RemoteSecurityPolicy();

    mutable std::mutex m_mutex;
    RemoteServerConfig m_config;
    ConfirmationCallback m_confirmCallback;

    // Sliding window rate limiting: deviceId -> (windowStart, requestCount)
    struct RateLimiter {
        std::chrono::steady_clock::time_point windowStart;
        int count{0};
    };
    std::unordered_map<std::string, RateLimiter> m_rateLimiters;

    std::vector<RemoteAuditEntry> m_auditLogs;
};

} // namespace pdfinity::remote
