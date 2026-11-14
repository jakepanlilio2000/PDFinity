// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "SettingsManager.hpp"
#include "application/Logger.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QSaveFile>

namespace pdfinity::persistence {

SettingsManager::SettingsManager(std::filesystem::path settingsFilePath) 
    : m_settingsPath(std::move(settingsFilePath)) {}

bool SettingsManager::LoadSettings(app::AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString qPath = QString::fromStdString(m_settingsPath.string());
    QFile file(qPath);
    if (!file.exists()) {
        LOG_INFO("Settings", "No settings file found; using defaults.");
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Settings", "Failed to open settings file for reading: " + m_settingsPath.string());
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        LOG_WARN("Settings", "Failed to parse settings JSON: " + parseError.errorString().toStdString());
        return false;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("themeMode")) {
        config.themeMode = app::AppConfig::StringToThemeMode(obj["themeMode"].toString().toStdString());
    }
    if (obj.contains("windowWidth")) {
        config.windowWidth = obj["windowWidth"].toInt(1280);
    }
    if (obj.contains("windowHeight")) {
        config.windowHeight = obj["windowHeight"].toInt(820);
    }
    if (obj.contains("windowPosX")) {
        config.windowPosX = obj["windowPosX"].toInt(-1);
    }
    if (obj.contains("windowPosY")) {
        config.windowPosY = obj["windowPosY"].toInt(-1);
    }
    if (obj.contains("isMaximized")) {
        config.isMaximized = obj["isMaximized"].toBool(false);
    }
    if (obj.contains("logLevel")) {
        config.logLevel = obj["logLevel"].toString("Info").toStdString();
    }
    if (obj.contains("autosaveEnabled")) {
        config.autosaveEnabled = obj["autosaveEnabled"].toBool(true);
    }
    if (obj.contains("autosaveIntervalSeconds")) {
        config.autosaveIntervalSeconds = obj["autosaveIntervalSeconds"].toInt(300);
    }
    if (obj.contains("maxRecentFiles")) {
        config.maxRecentFiles = obj["maxRecentFiles"].toInt(10);
    }
    if (obj.contains("hardwareAcceleration")) {
        config.hardwareAcceleration = obj["hardwareAcceleration"].toBool(true);
    }
    if (obj.contains("restoreSessionOnStartup")) {
        config.restoreSessionOnStartup = obj["restoreSessionOnStartup"].toBool(true);
    }
    if (obj.contains("sessionActiveTabIndex")) {
        config.sessionActiveTabIndex = obj["sessionActiveTabIndex"].toInt(0);
    }
    if (obj.contains("sessionOpenFiles") && obj["sessionOpenFiles"].isArray()) {
        config.sessionOpenFiles.clear();
        for (const auto& item : obj["sessionOpenFiles"].toArray()) {
            config.sessionOpenFiles.push_back(item.toString().toStdString());
        }
    }

    if (obj.contains("leftRailDrawerExpanded")) {
        config.leftRailDrawerExpanded = obj["leftRailDrawerExpanded"].toBool(true);
    }
    if (obj.contains("leftRailActivePanel")) {
        config.leftRailActivePanel = obj["leftRailActivePanel"].toInt(0);
    }
    if (obj.contains("rightInspectorVisible")) {
        config.rightInspectorVisible = obj["rightInspectorVisible"].toBool(true);
    }
    if (obj.contains("viewerSplitterSizes") && obj["viewerSplitterSizes"].isArray()) {
        config.viewerSplitterSizes.clear();
        for (const auto& item : obj["viewerSplitterSizes"].toArray()) {
            config.viewerSplitterSizes.push_back(item.toInt(200));
        }
    }

    if (obj.contains("mcpEnabled")) {
        config.mcpEnabled = obj["mcpEnabled"].toBool(false);
    }
    if (obj.contains("mcpConsentPolicy")) {
        config.mcpConsentPolicy = obj["mcpConsentPolicy"].toString("AskForDestructive").toStdString();
    }
    if (obj.contains("mcpAllowDestructiveOperations")) {
        config.mcpAllowDestructiveOperations = obj["mcpAllowDestructiveOperations"].toBool(false);
    }
    if (obj.contains("mcpApprovedRoots") && obj["mcpApprovedRoots"].isArray()) {
        config.mcpApprovedRoots.clear();
        for (const auto& r : obj["mcpApprovedRoots"].toArray()) {
            config.mcpApprovedRoots.push_back(r.toString().toStdString());
        }
    }

    if (obj.contains("remoteEnabled")) {
        config.remoteEnabled = obj["remoteEnabled"].toBool(false);
    }
    if (obj.contains("remoteBindAddress")) {
        config.remoteBindAddress = obj["remoteBindAddress"].toString("127.0.0.1").toStdString();
    }
    if (obj.contains("remotePort")) {
        config.remotePort = static_cast<uint16_t>(obj["remotePort"].toInt(8443));
    }
    if (obj.contains("remoteRequireHostConfirmation")) {
        config.remoteRequireHostConfirmation = obj["remoteRequireHostConfirmation"].toBool(true);
    }
    if (obj.contains("remotePairedDevicesJson")) {
        config.remotePairedDevicesJson = obj["remotePairedDevicesJson"].toString("[]").toStdString();
    }

    LOG_INFO("Settings", "Settings successfully loaded from: " + m_settingsPath.string());
    return true;
}

bool SettingsManager::SaveSettings(const app::AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        std::filesystem::create_directories(m_settingsPath.parent_path());
    } catch (...) {
        LOG_ERROR("Settings", "Failed to create directory for settings: " + m_settingsPath.parent_path().string());
        return false;
    }

    QJsonObject obj;
    obj["themeMode"] = QString::fromStdString(app::AppConfig::ThemeModeToString(config.themeMode));
    obj["windowWidth"] = config.windowWidth;
    obj["windowHeight"] = config.windowHeight;
    obj["windowPosX"] = config.windowPosX;
    obj["windowPosY"] = config.windowPosY;
    obj["isMaximized"] = config.isMaximized;
    obj["logLevel"] = QString::fromStdString(config.logLevel);
    obj["autosaveEnabled"] = config.autosaveEnabled;
    obj["autosaveIntervalSeconds"] = config.autosaveIntervalSeconds;
    obj["maxRecentFiles"] = config.maxRecentFiles;
    obj["hardwareAcceleration"] = config.hardwareAcceleration;
    obj["restoreSessionOnStartup"] = config.restoreSessionOnStartup;
    obj["sessionActiveTabIndex"] = config.sessionActiveTabIndex;

    QJsonArray sessionArr;
    for (const auto& path : config.sessionOpenFiles) {
        sessionArr.append(QString::fromStdString(path));
    }
    obj["sessionOpenFiles"] = sessionArr;

    obj["leftRailDrawerExpanded"] = config.leftRailDrawerExpanded;
    obj["leftRailActivePanel"] = config.leftRailActivePanel;
    obj["rightInspectorVisible"] = config.rightInspectorVisible;
    QJsonArray splitterArr;
    for (int sz : config.viewerSplitterSizes) {
        splitterArr.append(sz);
    }
    obj["viewerSplitterSizes"] = splitterArr;

    obj["mcpEnabled"] = config.mcpEnabled;
    obj["mcpConsentPolicy"] = QString::fromStdString(config.mcpConsentPolicy);
    obj["mcpAllowDestructiveOperations"] = config.mcpAllowDestructiveOperations;
    QJsonArray rootsArr;
    for (const auto& root : config.mcpApprovedRoots) {
        rootsArr.append(QString::fromStdString(root));
    }
    obj["mcpApprovedRoots"] = rootsArr;

    obj["remoteEnabled"] = config.remoteEnabled;
    obj["remoteBindAddress"] = QString::fromStdString(config.remoteBindAddress);
    obj["remotePort"] = config.remotePort;
    obj["remoteRequireHostConfirmation"] = config.remoteRequireHostConfirmation;
    obj["remotePairedDevicesJson"] = QString::fromStdString(config.remotePairedDevicesJson);

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QString qPath = QString::fromStdString(m_settingsPath.string());
    // QSaveFile provides safe atomic replacement: writes to temporary file and renames atomically on commit
    QSaveFile saveFile(qPath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Settings", "Failed to open QSaveFile for settings write: " + m_settingsPath.string());
        return false;
    }

    saveFile.write(data);
    if (!saveFile.commit()) {
        LOG_ERROR("Settings", "Atomic commit failed for settings write: " + m_settingsPath.string());
        return false;
    }

    LOG_INFO("Settings", "Settings saved atomically to: " + m_settingsPath.string());
    return true;
}

} // namespace pdfinity::persistence
