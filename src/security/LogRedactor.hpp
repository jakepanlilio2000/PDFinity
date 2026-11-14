// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>

namespace pdfinity::security {

class LogRedactor {
public:
    LogRedactor();

    void AddRedactionPattern(const std::string& pattern, const std::string& replacement);
    void SetRedactUserPaths(bool enable);

    std::string Redact(const std::string& message) const;

private:
    struct RedactionRule {
        std::string pattern;
        std::string replacement;
    };

    bool m_redactUserPaths{true};
    std::string m_currentUser;
    std::vector<RedactionRule> m_rules;
};

} // namespace pdfinity::security
