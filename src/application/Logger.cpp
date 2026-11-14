// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace pdfinity::app {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Initialize(const std::filesystem::path& logFilePath, LogLevel minimumLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minimumLevel = minimumLevel;

    try {
        if (!logFilePath.empty()) {
            std::filesystem::create_directories(logFilePath.parent_path());
            m_fileStream.open(logFilePath, std::ios::out | std::ios::app);
        }
    } catch (...) {
        // Fallback to console if file opening fails
    }
    m_initialized = true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
    m_initialized = false;
}

void Logger::SetMinimumLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minimumLevel = level;
}

LogLevel Logger::GetMinimumLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_minimumLevel;
}

void Logger::Log(LogLevel level, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (level < m_minimumLevel) {
        return;
    }

    std::string timestamp = GetCurrentTimestampIso();
    std::string levelStr = LevelToString(level);
    std::string redactedMsg = m_redactor.Redact(message);

    std::ostringstream formatted;
    formatted << "[" << timestamp << "] [" << levelStr << "] [" << tag << "] " << redactedMsg << "\n";
    std::string line = formatted.str();

    // Print to console
    if (level >= LogLevel::Warning) {
        std::cerr << line;
    } else {
        std::cout << line;
    }

    // Write to file
    if (m_fileStream.is_open()) {
        m_fileStream << line;
        m_fileStream.flush();
    }
}

void Logger::Debug(const std::string& tag, const std::string& message) {
    Log(LogLevel::Debug, tag, message);
}

void Logger::Info(const std::string& tag, const std::string& message) {
    Log(LogLevel::Info, tag, message);
}

void Logger::Warning(const std::string& tag, const std::string& message) {
    Log(LogLevel::Warning, tag, message);
}

void Logger::Error(const std::string& tag, const std::string& message) {
    Log(LogLevel::Error, tag, message);
}

void Logger::Fatal(const std::string& tag, const std::string& message) {
    Log(LogLevel::Fatal, tag, message);
}

std::string Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::GetCurrentTimestampIso() const {
    auto now = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif

    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace pdfinity::app
