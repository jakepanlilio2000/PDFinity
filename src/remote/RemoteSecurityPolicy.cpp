// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteSecurityPolicy.hpp"
#include "RemotePairingManager.hpp"
#include "application/Logger.hpp"
#include <QDateTime>
#include <cmath>

namespace pdfinity::remote {

RemoteSecurityPolicy& RemoteSecurityPolicy::Instance() {
    static RemoteSecurityPolicy instance;
    return instance;
}

RemoteSecurityPolicy::RemoteSecurityPolicy() {
    m_config.enabled = false; // Strictly off by default
    m_config.bindAddress = "127.0.0.1";
    m_config.port = DEFAULT_REMOTE_PORT;
    m_config.requireHostConfirmation = true;
    m_config.idleTimeoutSeconds = DEFAULT_SESSION_IDLE_TIMEOUT_SECONDS;
    m_config.maxRequestsPerSecond = 25;
}

void RemoteSecurityPolicy::SetConfig(const RemoteServerConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

RemoteServerConfig RemoteSecurityPolicy::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void RemoteSecurityPolicy::SetEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.enabled = enabled;
    LOG_INFO("RemoteSecurity", std::string("Remote Controller service status changed: ") + (enabled ? "ENABLED" : "DISABLED"));
}

bool RemoteSecurityPolicy::IsEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.enabled;
}

void RemoteSecurityPolicy::SetBindAddress(const std::string& addr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.bindAddress = addr;
}

std::string RemoteSecurityPolicy::GetBindAddress() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.bindAddress;
}

void RemoteSecurityPolicy::SetServerPort(uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.port = port;
}

uint16_t RemoteSecurityPolicy::GetServerPort() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.port;
}

AuthCheckResult RemoteSecurityPolicy::ValidateRequest(const std::string& deviceId,
                                                      const std::string& token,
                                                      uint64_t nonce,
                                                      int64_t timestampSec,
                                                      const std::string& clientIp) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Service enabled check
    if (!m_config.enabled) {
        return AuthCheckResult::Deny(-32001, "Remote Controller is disabled on host desktop.");
    }

    if (deviceId.empty() || token.empty()) {
        return AuthCheckResult::Deny(-32002, "Missing authentication credentials (deviceId and token required).");
    }

    // 2. Device lookup
    auto devOpt = RemotePairingManager::Instance().GetDevice(deviceId);
    if (!devOpt.has_value()) {
        return AuthCheckResult::Deny(-32002, "Unauthorized device. Device is not paired.");
    }

    const auto& dev = devOpt.value();

    // 3. Revocation check
    if (dev.isRevoked) {
        return AuthCheckResult::Deny(-32003, "Access revoked. Device has been revoked by host operator.");
    }

    // 4. Token validation
    if (token != dev.token) {
        return AuthCheckResult::Deny(-32002, "Invalid authentication token.");
    }

    // 5. Replay protection: timestamp check (±30s window)
    int64_t currentSec = QDateTime::currentSecsSinceEpoch();
    if (timestampSec != 0 && std::abs(currentSec - timestampSec) > 30) {
        return AuthCheckResult::Deny(-32006, "Request timestamp expired or out of sync (exceeds ±30s window).");
    }

    // 6. Replay protection: nonce check (strictly greater)
    if (nonce != 0 && nonce <= dev.lastNonce) {
        return AuthCheckResult::Deny(-32007, "Replay attack detected: Nonce must be strictly monotonically increasing.");
    }

    // 7. Rate limit check
    auto now = std::chrono::steady_clock::now();
    auto& rLim = m_rateLimiters[deviceId];
    if (now - rLim.windowStart > std::chrono::seconds(1)) {
        rLim.windowStart = now;
        rLim.count = 1;
    } else {
        rLim.count++;
        if (rLim.count > m_config.maxRequestsPerSecond) {
            return AuthCheckResult::Deny(-32008, "Rate limit exceeded (maximum " +
                                                 std::to_string(m_config.maxRequestsPerSecond) + " requests/sec).");
        }
    }

    // Update last seen and nonce
    RemotePairingManager::Instance().UpdateDeviceLastSeen(deviceId, clientIp, nonce);

    return AuthCheckResult::Allow(dev);
}

bool RemoteSecurityPolicy::CheckRequestRateLimit(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    auto& rLim = m_rateLimiters[deviceId];
    if (now - rLim.windowStart > std::chrono::seconds(1)) {
        rLim.windowStart = now;
        rLim.count = 1;
        return true;
    }
    rLim.count++;
    return rLim.count <= m_config.maxRequestsPerSecond;
}

void RemoteSecurityPolicy::SetConfirmationCallback(ConfirmationCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_confirmCallback = cb;
}

bool RemoteSecurityPolicy::RequestHostConfirmation(const std::string& clientName,
                                                   const std::string& command,
                                                   const std::string& details) {
    ConfirmationCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_config.requireHostConfirmation) {
            return true;
        }
        cb = m_confirmCallback;
    }

    if (!cb) {
        LOG_WARN("RemoteSecurity", "No host confirmation callback registered; sensitive command denied by default: " + command);
        return false;
    }

    return cb(clientName, command, details);
}

void RemoteSecurityPolicy::RecordAudit(const std::string& deviceId,
                                      const std::string& clientName,
                                      const std::string& ip,
                                      const std::string& command,
                                      const std::string& status,
                                      const std::string& details) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RemoteAuditEntry entry;
    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    entry.deviceId = deviceId.empty() ? "-" : deviceId;
    entry.clientName = clientName.empty() ? "unpaired_client" : clientName;
    entry.ip = ip.empty() ? "-" : ip;
    entry.command = command;
    entry.status = status;
    entry.details = details;

    m_auditLogs.push_back(entry);
    if (m_auditLogs.size() > 1000) {
        m_auditLogs.erase(m_auditLogs.begin(), m_auditLogs.begin() + 100);
    }

    if (status == "DENIED" || status == "BLOCKED" || status == "RATE_LIMITED") {
        LOG_WARN("RemoteAudit", "[" + status + "] " + command + " from " + clientName + " (" + ip + "): " + details);
    } else {
        LOG_INFO("RemoteAudit", "[" + status + "] " + command + " from " + clientName + " (" + ip + "): " + details);
    }
}

std::vector<RemoteAuditEntry> RemoteSecurityPolicy::GetAuditLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auditLogs;
}

void RemoteSecurityPolicy::ClearAuditLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_auditLogs.clear();
    m_rateLimiters.clear();
}

} // namespace pdfinity::remote
