// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "BatchJobHistory.hpp"
#include "application/Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace pdfinity::services {

namespace {

QJsonObject FileItemToJson(const domain::BatchFileItem& item) {
    QJsonObject obj;
    obj["fileId"] = QString::fromStdString(item.fileId);
    obj["inputPath"] = QString::fromStdString(item.inputPath.string());
    obj["outputPath"] = QString::fromStdString(item.outputPath.string());
    obj["status"] = static_cast<int>(item.status);
    obj["inputSizeBytes"] = static_cast<qint64>(item.inputSizeBytes);
    obj["outputSizeBytes"] = static_cast<qint64>(item.outputSizeBytes);
    obj["durationMs"] = static_cast<qint64>(item.durationMs);
    obj["retryCount"] = item.retryCount;
    obj["errorMessage"] = QString::fromStdString(item.errorMessage);

    QJsonArray logs;
    for (const auto& l : item.logs) {
        QJsonObject logObj;
        logObj["timestamp"] = QString::fromStdString(l.timestamp);
        logObj["level"] = QString::fromStdString(l.level);
        logObj["message"] = QString::fromStdString(l.message);
        logs.append(logObj);
    }
    obj["logs"] = logs;
    return obj;
}

domain::BatchFileItem FileItemFromJson(const QJsonObject& obj) {
    domain::BatchFileItem item;
    item.fileId = obj["fileId"].toString().toStdString();
    item.inputPath = obj["inputPath"].toString().toStdString();
    item.outputPath = obj["outputPath"].toString().toStdString();
    item.status = static_cast<domain::BatchFileStatus>(obj["status"].toInt());
    item.inputSizeBytes = static_cast<uint64_t>(obj["inputSizeBytes"].toInteger());
    item.outputSizeBytes = static_cast<uint64_t>(obj["outputSizeBytes"].toInteger());
    item.durationMs = obj["durationMs"].toInteger();
    item.retryCount = obj["retryCount"].toInt();
    item.errorMessage = obj["errorMessage"].toString().toStdString();

    QJsonArray logs = obj["logs"].toArray();
    for (const auto& logVal : logs) {
        QJsonObject logObj = logVal.toObject();
        domain::BatchFileLogEntry l;
        l.timestamp = logObj["timestamp"].toString().toStdString();
        l.level = logObj["level"].toString().toStdString();
        l.message = logObj["message"].toString().toStdString();
        item.logs.push_back(l);
    }
    return item;
}

QJsonObject SummaryToJson(const domain::BatchJobSummary& s) {
    QJsonObject obj;
    obj["jobId"] = QString::fromStdString(s.jobId);
    obj["jobName"] = QString::fromStdString(s.jobName);
    obj["profileName"] = QString::fromStdString(s.profileName);
    obj["state"] = static_cast<int>(s.state);
    obj["startTime"] = QString::fromStdString(s.startTime);
    obj["finishTime"] = QString::fromStdString(s.finishTime);
    obj["totalFiles"] = s.totalFiles;
    obj["succeededFiles"] = s.succeededFiles;
    obj["failedFiles"] = s.failedFiles;
    obj["skippedFiles"] = s.skippedFiles;
    obj["totalInputBytes"] = static_cast<qint64>(s.totalInputBytes);
    obj["totalOutputBytes"] = static_cast<qint64>(s.totalOutputBytes);
    obj["totalDurationMs"] = static_cast<qint64>(s.totalDurationMs);

    QJsonArray files;
    for (const auto& f : s.fileItems) {
        files.append(FileItemToJson(f));
    }
    obj["fileItems"] = files;
    return obj;
}

domain::BatchJobSummary SummaryFromJson(const QJsonObject& obj) {
    domain::BatchJobSummary s;
    s.jobId = obj["jobId"].toString().toStdString();
    s.jobName = obj["jobName"].toString().toStdString();
    s.profileName = obj["profileName"].toString().toStdString();
    s.state = static_cast<domain::BatchJobState>(obj["state"].toInt());
    s.startTime = obj["startTime"].toString().toStdString();
    s.finishTime = obj["finishTime"].toString().toStdString();
    s.totalFiles = obj["totalFiles"].toInt();
    s.succeededFiles = obj["succeededFiles"].toInt();
    s.failedFiles = obj["failedFiles"].toInt();
    s.skippedFiles = obj["skippedFiles"].toInt();
    s.totalInputBytes = static_cast<uint64_t>(obj["totalInputBytes"].toInteger());
    s.totalOutputBytes = static_cast<uint64_t>(obj["totalOutputBytes"].toInteger());
    s.totalDurationMs = obj["totalDurationMs"].toInteger();

    QJsonArray files = obj["fileItems"].toArray();
    for (const auto& fileVal : files) {
        s.fileItems.push_back(FileItemFromJson(fileVal.toObject()));
    }
    return s;
}

std::string FormatBytes(uint64_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB";
    return ss.str();
}

} // namespace

BatchJobHistory& BatchJobHistory::Instance() {
    static BatchJobHistory instance;
    return instance;
}

BatchJobHistory::BatchJobHistory() {
    LoadHistory();
}

std::filesystem::path BatchJobHistory::GetHistoryFilePath() const {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    return std::filesystem::path(appData.toStdString()) / "batch_jobs.json";
}

void BatchJobHistory::LoadHistory() {
    m_history.clear();
    auto path = GetHistoryFilePath();
    if (!std::filesystem::exists(path)) return;

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        if (item.isObject()) {
            m_history.push_back(SummaryFromJson(item.toObject()));
        }
    }
}

void BatchJobHistory::PersistHistory() {
    auto path = GetHistoryFilePath();
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_WARN("BatchJobHistory", "Failed to open history file for writing: " + path.string());
        return;
    }

    QJsonArray arr;
    for (const auto& item : m_history) {
        arr.append(SummaryToJson(item));
    }

    QJsonDocument doc(arr);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void BatchJobHistory::RecordJob(const domain::BatchJobSummary& summary) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Remove if already exists with same jobId
    auto it = std::remove_if(m_history.begin(), m_history.end(), [&](const domain::BatchJobSummary& s) {
        return s.jobId == summary.jobId;
    });
    m_history.erase(it, m_history.end());

    // Insert at front
    m_history.insert(m_history.begin(), summary);

    // Keep at most 50 recent jobs
    if (m_history.size() > 50) {
        m_history.resize(50);
    }

    PersistHistory();
}

std::vector<domain::BatchJobSummary> BatchJobHistory::GetRecentJobs(int limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (limit <= 0 || static_cast<size_t>(limit) >= m_history.size()) {
        return m_history;
    }
    return std::vector<domain::BatchJobSummary>(m_history.begin(), m_history.begin() + limit);
}

std::optional<domain::BatchJobSummary> BatchJobHistory::GetJobSummary(const std::string& jobId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& s : m_history) {
        if (s.jobId == jobId) {
            return s;
        }
    }
    return std::nullopt;
}

void BatchJobHistory::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
    PersistHistory();
}

bool BatchJobHistory::ExportSummaryToJson(const domain::BatchJobSummary& summary, const std::filesystem::path& destination) {
    QFile file(QString::fromStdString(destination.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonObject obj = SummaryToJson(summary);
    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool BatchJobHistory::ExportSummaryToHtml(const domain::BatchJobSummary& summary, const std::filesystem::path& destination) {
    std::ofstream out(destination, std::ios::trunc);
    if (!out.is_open()) return false;

    std::string stateStr;
    std::string stateBadgeColor;
    switch (summary.state) {
        case domain::BatchJobState::Succeeded:
            stateStr = "SUCCEEDED";
            stateBadgeColor = "#16a34a";
            break;
        case domain::BatchJobState::Failed:
            stateStr = "FAILED";
            stateBadgeColor = "#dc2626";
            break;
        case domain::BatchJobState::Cancelled:
            stateStr = "CANCELLED";
            stateBadgeColor = "#ca8a04";
            break;
        default:
            stateStr = "COMPLETED";
            stateBadgeColor = "#2563eb";
            break;
    }

    double ratio = 0.0;
    if (summary.totalInputBytes > 0) {
        ratio = (1.0 - (static_cast<double>(summary.totalOutputBytes) / summary.totalInputBytes)) * 100.0;
    }

    out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
        << "  <meta charset=\"UTF-8\">\n"
        << "  <title>Batch Execution Report - " << summary.jobName << "</title>\n"
        << "  <style>\n"
        << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #f8fafc; color: #1e293b; padding: 24px; margin: 0; }\n"
        << "    .container { max-width: 1000px; margin: 0 auto; background: #ffffff; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); padding: 32px; }\n"
        << "    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #e2e8f0; padding-bottom: 16px; margin-bottom: 24px; }\n"
        << "    .badge { display: inline-block; padding: 6px 12px; border-radius: 9999px; font-weight: bold; font-size: 14px; color: #fff; background: " << stateBadgeColor << "; }\n"
        << "    .metrics-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 32px; }\n"
        << "    .card { background: #f1f5f9; border-radius: 6px; padding: 16px; text-align: center; }\n"
        << "    .card-num { font-size: 28px; font-weight: bold; margin-top: 4px; }\n"
        << "    table { width: 100%; border-collapse: collapse; margin-top: 16px; font-size: 13px; }\n"
        << "    th, td { padding: 10px 12px; text-align: left; border-bottom: 1px solid #e2e8f0; }\n"
        << "    th { background: #f8fafc; font-weight: 600; }\n"
        << "    .status-badge { padding: 3px 8px; border-radius: 4px; font-weight: 600; font-size: 11px; text-transform: uppercase; }\n"
        << "    .status-success { background: #dcfce7; color: #166534; }\n"
        << "    .status-error { background: #fee2e2; color: #991b1b; }\n"
        << "    .status-skipped { background: #fef3c7; color: #92400e; }\n"
        << "  </style>\n</head>\n<body>\n"
        << "<div class=\"container\">\n"
        << "  <div class=\"header\">\n"
        << "    <div>\n"
        << "      <h1 style=\"margin:0 0 8px 0; font-size:24px;\">Batch Processing Summary Report</h1>\n"
        << "      <div style=\"color:#64748b;\">Job: <b>" << summary.jobName << "</b> | Profile: <b>" << summary.profileName << "</b></div>\n"
        << "      <div style=\"font-size:12px; color:#94a3b8; margin-top:4px;\">ID: " << summary.jobId << " | Finished: " << summary.finishTime << "</div>\n"
        << "    </div>\n"
        << "    <div><span class=\"badge\">" << stateStr << "</span></div>\n"
        << "  </div>\n"
        << "  <div class=\"metrics-grid\">\n"
        << "    <div class=\"card\"><div>Total Files</div><div class=\"card-num\" style=\"color:#0f172a;\">" << summary.totalFiles << "</div></div>\n"
        << "    <div class=\"card\"><div>Succeeded</div><div class=\"card-num\" style=\"color:#16a34a;\">" << summary.succeededFiles << "</div></div>\n"
        << "    <div class=\"card\"><div>Failed</div><div class=\"card-num\" style=\"color:#dc2626;\">" << summary.failedFiles << "</div></div>\n"
        << "    <div class=\"card\"><div>Skipped</div><div class=\"card-num\" style=\"color:#d97706;\">" << summary.skippedFiles << "</div></div>\n"
        << "  </div>\n"
        << "  <div style=\"margin-bottom: 24px; padding: 12px 16px; background:#f8fafc; border:1px solid #e2e8f0; border-radius:6px;\">\n"
        << "    <b>Data Processed:</b> " << FormatBytes(summary.totalInputBytes) << " → " << FormatBytes(summary.totalOutputBytes)
        << " (" << std::fixed << std::setprecision(1) << ratio << "% reduction) | "
        << "    <b>Execution Time:</b> " << (summary.totalDurationMs / 1000.0) << "s\n"
        << "  </div>\n"
        << "  <h2>Processed Files</h2>\n"
        << "  <table>\n"
        << "    <thead>\n"
        << "      <tr><th>#</th><th>Status</th><th>Source File</th><th>Destination File</th><th>Size</th><th>Duration</th><th>Details</th></tr>\n"
        << "    </thead>\n"
        << "    <tbody>\n";

    for (size_t i = 0; i < summary.fileItems.size(); ++i) {
        const auto& f = summary.fileItems[i];
        std::string sClass = "status-success";
        std::string sText = "Success";
        if (f.status == domain::BatchFileStatus::Error) {
            sClass = "status-error";
            sText = "Error";
        } else if (f.status == domain::BatchFileStatus::Skipped) {
            sClass = "status-skipped";
            sText = "Skipped";
        }

        out << "      <tr>\n"
            << "        <td>" << (i + 1) << "</td>\n"
            << "        <td><span class=\"status-badge " << sClass << "\">" << sText << "</span></td>\n"
            << "        <td style=\"font-family:monospace;\">" << f.inputPath.filename().string() << "</td>\n"
            << "        <td style=\"font-family:monospace;\">" << f.outputPath.filename().string() << "</td>\n"
            << "        <td>" << FormatBytes(f.inputSizeBytes) << " → " << FormatBytes(f.outputSizeBytes) << "</td>\n"
            << "        <td>" << f.durationMs << " ms</td>\n"
            << "        <td style=\"color:" << (f.status == domain::BatchFileStatus::Error ? "#dc2626" : "#64748b") << ";\">"
            << (f.errorMessage.empty() ? "Completed successfully" : f.errorMessage)
            << "</td>\n"
            << "      </tr>\n";
    }

    out << "    </tbody>\n  </table>\n</div>\n</body>\n</html>\n";
    out.close();
    return true;
}

} // namespace pdfinity::services
