// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "RemoteTypes.hpp"
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace pdfinity::remote {

struct PairingResult {
    bool success{false};
    int errorCode{0};
    std::string errorMessage;
    std::string token;
    RemoteDeviceInfo deviceInfo;
};

class RemotePairingManager {
public:
    static RemotePairingManager& Instance();

    // 1. PIN Lifecycle
    std::string GenerateNewPin(int lifetimeSeconds = DEFAULT_PIN_LIFETIME_SECONDS);
    std::string GetActivePin() const;
    bool IsPairingActive() const;
    int GetSecondsRemaining() const;
    void CancelPairing();

    // 2. Client Pairing Attempt
    PairingResult AttemptPairing(const std::string& clientIp,
                                 const std::string& pin,
                                 const std::string& clientName,
                                 const std::string& deviceId);

    // 3. Paired Devices Registry
    std::vector<RemoteDeviceInfo> GetPairedDevices() const;
    std::optional<RemoteDeviceInfo> GetDevice(const std::string& deviceId) const;
    bool RevokeDevice(const std::string& deviceId);
    void ClearAllDevices();
    void ResetRateLimits();
    void UpdateDeviceLastSeen(const std::string& deviceId, const std::string& ip, uint64_t nonce);

    // 4. Persistence
    std::string ExportDevicesJson() const;
    void ImportDevicesJson(const std::string& json);

private:
    RemotePairingManager() = default;

    mutable std::mutex m_mutex;

    // PIN State
    std::string m_activePin;
    std::chrono::steady_clock::time_point m_pinExpiresAt;

    // Rate Limiting on Pairing Attempts
    struct FailedAttemptTracker {
        int failedCount{0};
        std::chrono::steady_clock::time_point lockedUntil;
    };
    std::unordered_map<std::string, FailedAttemptTracker> m_ipAttempts;

    // Paired Devices
    std::vector<RemoteDeviceInfo> m_pairedDevices;
};

} // namespace pdfinity::remote
