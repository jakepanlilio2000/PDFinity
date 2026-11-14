// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RecentFilesManager.hpp"
#include "security/PathSanitizer.hpp"
#include "application/Logger.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QSaveFile>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace pdfinity::persistence {

namespace {
std::string GetCurrentTimestampIso() {
    auto now = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}
} // namespace

RecentFilesManager::RecentFilesManager(std::filesystem::path filePath, int maxEntries)
    : m_storagePath(std::move(filePath)), m_maxEntries(maxEntries) {}

bool RecentFilesManager::AddFile(const std::filesystem::path& path) {
    auto sanitized = security::PathSanitizer::Canonicalize(path);
    if (!sanitized) {
        LOG_WARN("RecentFiles", "Refusing to add un-sanitizable path to recent files: " + path.string());
        return false;
    }

    std::filesystem::path cleanPath = sanitized.value();
    if (!std::filesystem::exists(cleanPath) || !std::filesystem::is_regular_file(cleanPath)) {
        LOG_WARN("RecentFiles", "File does not exist: " + cleanPath.string());
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string canonicalStr = cleanPath.string();
    // Remove if already present (to move to front)
    m_items.erase(
        std::remove_if(m_items.begin(), m_items.end(),
            [&](const RecentFileItem& item) { return item.canonicalPath == canonicalStr; }),
        m_items.end()
    );

    RecentFileItem item;
    item.canonicalPath = canonicalStr;
    item.fileName = cleanPath.filename().string();
    item.lastOpenedIso = GetCurrentTimestampIso();
    try {
        item.fileSizeBytes = std::filesystem::file_size(cleanPath);
    } catch (...) {
        item.fileSizeBytes = 0;
    }

    m_items.insert(m_items.begin(), item);

    if (static_cast<int>(m_items.size()) > m_maxEntries) {
        m_items.resize(m_maxEntries);
    }

    return Save();
}

bool RecentFilesManager::RemoveFile(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string target = path.string();

    auto it = std::remove_if(m_items.begin(), m_items.end(),
        [&](const RecentFileItem& item) { return item.canonicalPath == target; });

    if (it != m_items.end()) {
        m_items.erase(it, m_items.end());
        return Save();
    }
    return false;
}

void RecentFilesManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items.clear();
    Save();
}

void RecentFilesManager::RemoveMissingFiles() {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool changed = false;

    auto it = std::remove_if(m_items.begin(), m_items.end(),
        [&](const RecentFileItem& item) {
            std::error_code ec;
            bool exists = std::filesystem::exists(item.canonicalPath, ec);
            if (!exists) {
                changed = true;
                return true;
            }
            return false;
        });

    if (changed) {
        m_items.erase(it, m_items.end());
        Save();
    }
}

std::vector<RecentFileItem> RecentFilesManager::GetRecentFiles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items;
}

bool RecentFilesManager::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString qPath = QString::fromStdString(m_storagePath.string());
    QFile file(qPath);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return false;
    }

    m_items.clear();
    QJsonArray array = doc.array();
    for (const auto& val : array) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        RecentFileItem item;
        item.canonicalPath = obj["canonicalPath"].toString().toStdString();
        item.fileName = obj["fileName"].toString().toStdString();
        item.lastOpenedIso = obj["lastOpenedIso"].toString().toStdString();
        item.fileSizeBytes = static_cast<uint64_t>(obj["fileSizeBytes"].toVariant().toULongLong());

        // Validate that path is non-empty
        if (!item.canonicalPath.empty()) {
            m_items.push_back(item);
        }
    }

    return true;
}

bool RecentFilesManager::Save() {
    try {
        std::filesystem::create_directories(m_storagePath.parent_path());
    } catch (...) {
        return false;
    }

    QJsonArray array;
    for (const auto& item : m_items) {
        QJsonObject obj;
        obj["canonicalPath"] = QString::fromStdString(item.canonicalPath);
        obj["fileName"] = QString::fromStdString(item.fileName);
        obj["lastOpenedIso"] = QString::fromStdString(item.lastOpenedIso);
        obj["fileSizeBytes"] = static_cast<qint64>(item.fileSizeBytes);
        array.append(obj);
    }

    QJsonDocument doc(array);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QString qPath = QString::fromStdString(m_storagePath.string());
    QSaveFile saveFile(qPath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    saveFile.write(data);
    return saveFile.commit();
}

} // namespace pdfinity::persistence
