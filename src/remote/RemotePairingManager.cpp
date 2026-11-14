// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemotePairingManager.hpp"
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <algorithm>

namespace pdfinity::remote {

RemotePairingManager& RemotePairingManager::Instance() {
    static RemotePairingManager instance;
    return instance;
}

std::string RemotePairingManager::GenerateNewPin(int lifetimeSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Cryptographically secure 6-digit PIN [100000 .. 999999]
    quint32 num = QRandomGenerator::securelySeeded().bounded(100000u, 999999u);
    m_activePin = std::to_string(num);
    m_pinExpiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(lifetimeSeconds);
    return m_activePin;
}

std::string RemotePairingManager::GetActivePin() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!IsPairingActive()) return "";
    return m_activePin;
}

bool RemotePairingManager::IsPairingActive() const {
    if (m_activePin.empty()) return false;
    return std::chrono::steady_clock::now() < m_pinExpiresAt;
}

int RemotePairingManager::GetSecondsRemaining() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activePin.empty()) return 0;
    auto now = std::chrono::steady_clock::now();
    if (now >= m_pinExpiresAt) return 0;
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(m_pinExpiresAt - now).count();
    return static_cast<int>(diff);
}

void RemotePairingManager::CancelPairing() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activePin.clear();
    m_pinExpiresAt = std::chrono::steady_clock::time_point{};
}

PairingResult RemotePairingManager::AttemptPairing(const std::string& clientIp,
                                                   const std::string& pin,
                                                   const std::string& clientName,
                                                   const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();

    // 1. Rate limiting & Lockout check
    auto& tracker = m_ipAttempts[clientIp];
    if (tracker.lockedUntil > now) {
        auto waitSec = std::chrono::duration_cast<std::chrono::seconds>(tracker.lockedUntil - now).count();
        return {false, -32005, "Too many failed attempts. Locked out for " + std::to_string(waitSec) + " seconds.", "", {}};
    }

    // 2. Active PIN check
    if (m_activePin.empty() || now >= m_pinExpiresAt) {
        tracker.failedCount++;
        if (tracker.failedCount >= MAX_PAIRING_FAILED_ATTEMPTS) {
            tracker.lockedUntil = now + std::chrono::seconds(PAIRING_LOCKOUT_SECONDS);
        }
        return {false, -32004, "Pairing is not active or PIN has expired.", "", {}};
    }

    // 3. PIN comparison
    if (pin != m_activePin) {
        tracker.failedCount++;
        if (tracker.failedCount >= MAX_PAIRING_FAILED_ATTEMPTS) {
            tracker.lockedUntil = now + std::chrono::seconds(PAIRING_LOCKOUT_SECONDS);
            return {false, -32005, "Maximum failed attempts exceeded. Locked out for 5 minutes.", "", {}};
        }
        int remaining = MAX_PAIRING_FAILED_ATTEMPTS - tracker.failedCount;
        return {false, -32004, "Invalid PIN. " + std::to_string(remaining) + " attempt(s) remaining.", "", {}};
    }

    // Successful match - reset tracker
    tracker.failedCount = 0;
    tracker.lockedUntil = std::chrono::steady_clock::time_point{};

    // Consume single-use PIN
    m_activePin.clear();
    m_pinExpiresAt = std::chrono::steady_clock::time_point{};

    // 4. Generate 256-bit cryptographically secure bearer token
    QByteArray entropy;
    for (int i = 0; i < 32; ++i) {
        entropy.append(static_cast<char>(QRandomGenerator::securelySeeded().generate() & 0xFF));
    }
    std::string token = QCryptographicHash::hash(entropy, QCryptographicHash::Sha256).toHex().toStdString();

    // 5. Register device
    std::string actualDeviceId = deviceId.empty() ? ("dev_" + token.substr(0, 8)) : deviceId;
    std::string isoNow = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();

    // Remove any existing registration for this deviceId
    m_pairedDevices.erase(
        std::remove_if(m_pairedDevices.begin(), m_pairedDevices.end(),
                       [&actualDeviceId](const RemoteDeviceInfo& d) { return d.deviceId == actualDeviceId; }),
        m_pairedDevices.end()
    );

    RemoteDeviceInfo info;
    info.deviceId = actualDeviceId;
    info.clientName = clientName.empty() ? "Remote Device" : clientName;
    info.token = token;
    info.pairedAtIso = isoNow;
    info.lastSeenIso = isoNow;
    info.lastSeenIp = clientIp;
    info.isRevoked = false;
    info.lastNonce = 0;

    m_pairedDevices.push_back(info);

    return {true, 0, "", token, info};
}

std::vector<RemoteDeviceInfo> RemotePairingManager::GetPairedDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pairedDevices;
}

std::optional<RemoteDeviceInfo> RemotePairingManager::GetDevice(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& d : m_pairedDevices) {
        if (d.deviceId == deviceId) {
            return d;
        }
    }
    return std::nullopt;
}

bool RemotePairingManager::RevokeDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_pairedDevices) {
        if (d.deviceId == deviceId) {
            d.isRevoked = true;
            return true;
        }
    }
    return false;
}

void RemotePairingManager::ClearAllDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pairedDevices.clear();
    m_ipAttempts.clear();
}

void RemotePairingManager::ResetRateLimits() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ipAttempts.clear();
}

void RemotePairingManager::UpdateDeviceLastSeen(const std::string& deviceId, const std::string& ip, uint64_t nonce) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& d : m_pairedDevices) {
        if (d.deviceId == deviceId) {
            d.lastSeenIso = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
            d.lastSeenIp = ip;
            if (nonce > d.lastNonce) {
                d.lastNonce = nonce;
            }
            break;
        }
    }
}

std::string RemotePairingManager::ExportDevicesJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    QJsonArray arr;
    for (const auto& d : m_pairedDevices) {
        arr.append(d.ToJson());
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)).toStdString();
}

void RemotePairingManager::ImportDevicesJson(const std::string& json) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pairedDevices.clear();
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (doc.isArray()) {
        for (const auto& val : doc.array()) {
            if (val.isObject()) {
                m_pairedDevices.push_back(RemoteDeviceInfo::FromJson(val.toObject()));
            }
        }
    }
}

} // namespace pdfinity::remote
