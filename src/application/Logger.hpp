// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "security/LogRedactor.hpp"
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <filesystem>

namespace pdfinity::app {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    static Logger& Instance();

    void Initialize(const std::filesystem::path& logFilePath, LogLevel minimumLevel = LogLevel::Info);
    void Shutdown();

    void SetMinimumLevel(LogLevel level);
    LogLevel GetMinimumLevel() const;

    void Log(LogLevel level, const std::string& tag, const std::string& message);

    void Debug(const std::string& tag, const std::string& message);
    void Info(const std::string& tag, const std::string& message);
    void Warning(const std::string& tag, const std::string& message);
    void Error(const std::string& tag, const std::string& message);
    void Fatal(const std::string& tag, const std::string& message);

    security::LogRedactor& GetRedactor() { return m_redactor; }

private:
    Logger() = default;
    ~Logger();

    std::string LevelToString(LogLevel level) const;
    std::string GetCurrentTimestampIso() const;

    mutable std::mutex m_mutex;
    std::ofstream m_fileStream;
    LogLevel m_minimumLevel{LogLevel::Info};
    security::LogRedactor m_redactor;
    bool m_initialized{false};
};

} // namespace pdfinity::app

#define LOG_DEBUG(tag, msg)   ::pdfinity::app::Logger::Instance().Debug(tag, msg)
#define LOG_INFO(tag, msg)    ::pdfinity::app::Logger::Instance().Info(tag, msg)
#define LOG_WARN(tag, msg)    ::pdfinity::app::Logger::Instance().Warning(tag, msg)
#define LOG_ERROR(tag, msg)   ::pdfinity::app::Logger::Instance().Error(tag, msg)
#define LOG_FATAL(tag, msg)   ::pdfinity::app::Logger::Instance().Fatal(tag, msg)
