// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "LogRedactor.hpp"
#include <cstdlib>
#include <cstring>
#include <regex>

namespace pdfinity::security {

LogRedactor::LogRedactor() {
    // Detect current username
    const char* user = std::getenv("USERNAME");
    if (!user) {
        user = std::getenv("USER");
    }
    if (user && std::strlen(user) > 0) {
        m_currentUser = user;
    }
}

void LogRedactor::AddRedactionPattern(const std::string& pattern, const std::string& replacement) {
    if (!pattern.empty()) {
        m_rules.push_back({ pattern, replacement });
    }
}

void LogRedactor::SetRedactUserPaths(bool enable) {
    m_redactUserPaths = enable;
}

std::string LogRedactor::Redact(const std::string& message) const {
    std::string result = message;

    // Apply custom rules first
    for (const auto& rule : m_rules) {
        size_t pos = 0;
        while ((pos = result.find(rule.pattern, pos)) != std::string::npos) {
            result.replace(pos, rule.pattern.length(), rule.replacement);
            pos += rule.replacement.length();
        }
    }

    // Redact current username in paths (e.g., C:\Users\Username or /home/username)
    if (m_redactUserPaths && !m_currentUser.empty()) {
        size_t pos = 0;
        while ((pos = result.find(m_currentUser, pos)) != std::string::npos) {
            // Check context to avoid accidental substring matches on common words
            bool isPathContext = false;
            if (pos > 0 && (result[pos - 1] == '\\' || result[pos - 1] == '/')) {
                isPathContext = true;
            }
            if (pos + m_currentUser.length() < result.length()) {
                char nextChar = result[pos + m_currentUser.length()];
                if (nextChar == '\\' || nextChar == '/' || nextChar == ' ' || nextChar == '\"' || nextChar == '\'') {
                    isPathContext = true;
                }
            }

            if (isPathContext) {
                const std::string replacement = "<USER>";
                result.replace(pos, m_currentUser.length(), replacement);
                pos += replacement.length();
            } else {
                pos += m_currentUser.length();
            }
        }
    }

    // Redact password parameters (e.g. password=XYZ or pwd=XYZ)
    static const std::regex passRegex(R"((password|pwd|pass|secret)\s*[:=]\s*([^\s,;]+))", std::regex::icase);
    result = std::regex_replace(result, passRegex, "$1=********");

    return result;
}

} // namespace pdfinity::security
