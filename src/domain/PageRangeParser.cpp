// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PageRangeParser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace pdfinity::domain {

namespace {
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}
} // namespace

pdf_api::EngineResult<std::vector<int>> PageRangeParser::Parse(
    const std::string& rangeStr,
    int totalPages
) {
    if (totalPages <= 0) {
        return pdf_api::EngineResult<std::vector<int>>::Fail(
            pdf_api::EngineError::PageNotFound,
            "Total page count must be greater than zero"
        );
    }

    std::string clean = ToLower(Trim(rangeStr));
    if (clean.empty() || clean == "all" || clean == "*") {
        std::vector<int> allPages(static_cast<size_t>(totalPages));
        for (int i = 0; i < totalPages; ++i) allPages[static_cast<size_t>(i)] = i;
        return pdf_api::EngineResult<std::vector<int>>::Ok(std::move(allPages));
    }

    if (clean == "odd") {
        std::vector<int> odds;
        for (int i = 0; i < totalPages; i += 2) odds.push_back(i);
        return pdf_api::EngineResult<std::vector<int>>::Ok(std::move(odds));
    }

    if (clean == "even") {
        std::vector<int> evens;
        for (int i = 1; i < totalPages; i += 2) evens.push_back(i);
        return pdf_api::EngineResult<std::vector<int>>::Ok(std::move(evens));
    }

    if (clean == "reverse") {
        std::vector<int> rev(static_cast<size_t>(totalPages));
        for (int i = 0; i < totalPages; ++i) rev[static_cast<size_t>(i)] = totalPages - 1 - i;
        return pdf_api::EngineResult<std::vector<int>>::Ok(std::move(rev));
    }

    std::vector<int> result;
    std::stringstream ss(rangeStr);
    std::string token;

    while (std::getline(ss, token, ',')) {
        // Also split by semicolon if present
        std::stringstream subSs(token);
        std::string subToken;
        while (std::getline(subSs, subToken, ';')) {
            std::string item = Trim(subToken);
            if (item.empty()) continue;

            size_t hyphenPos = item.find('-');
            if (hyphenPos == std::string::npos) {
                // Single page number
                try {
                    size_t idx = 0;
                    int page = std::stoi(item, &idx);
                    if (idx != item.length() || page < 1 || page > totalPages) {
                        return pdf_api::EngineResult<std::vector<int>>::Fail(
                            pdf_api::EngineError::PageNotFound,
                            "Page number out of bounds: " + item
                        );
                    }
                    result.push_back(page - 1);
                } catch (...) {
                    return pdf_api::EngineResult<std::vector<int>>::Fail(
                        pdf_api::EngineError::InvalidFormat,
                        "Invalid page number token: " + item
                    );
                }
            } else {
                // Range A-B
                std::string startStr = Trim(item.substr(0, hyphenPos));
                std::string endStr = Trim(item.substr(hyphenPos + 1));

                int startPage = 1;
                int endPage = totalPages;

                if (!startStr.empty()) {
                    try {
                        startPage = std::stoi(startStr);
                    } catch (...) {
                        return pdf_api::EngineResult<std::vector<int>>::Fail(
                            pdf_api::EngineError::InvalidFormat,
                            "Invalid start page in range: " + item
                        );
                    }
                }

                if (!endStr.empty()) {
                    try {
                        endPage = std::stoi(endStr);
                    } catch (...) {
                        return pdf_api::EngineResult<std::vector<int>>::Fail(
                            pdf_api::EngineError::InvalidFormat,
                            "Invalid end page in range: " + item
                        );
                    }
                }

                if (startPage < 1 || startPage > totalPages || endPage < 1 || endPage > totalPages) {
                    return pdf_api::EngineResult<std::vector<int>>::Fail(
                        pdf_api::EngineError::PageNotFound,
                        "Range out of bounds: " + item
                    );
                }

                if (startPage <= endPage) {
                    for (int p = startPage; p <= endPage; ++p) {
                        result.push_back(p - 1);
                    }
                } else {
                    // Descending range: e.g. 5-3
                    for (int p = startPage; p >= endPage; --p) {
                        result.push_back(p - 1);
                    }
                }
            }
        }
    }

    if (result.empty()) {
        return pdf_api::EngineResult<std::vector<int>>::Fail(
            pdf_api::EngineError::InvalidFormat,
            "No valid pages specified in range: " + rangeStr
        );
    }

    return pdf_api::EngineResult<std::vector<int>>::Ok(std::move(result));
}

std::string PageRangeParser::Format(const std::vector<int>& indices) {
    if (indices.empty()) return "";

    std::vector<int> sorted = indices;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    std::ostringstream oss;
    size_t i = 0;
    while (i < sorted.size()) {
        size_t start = i;
        while (i + 1 < sorted.size() && sorted[i + 1] == sorted[i] + 1) {
            ++i;
        }

        if (start > 0) oss << ", ";

        if (start == i) {
            oss << (sorted[start] + 1);
        } else {
            oss << (sorted[start] + 1) << "-" << (sorted[i] + 1);
        }
        ++i;
    }

    return oss.str();
}

bool PageRangeParser::IsValid(const std::string& rangeStr, int totalPages) {
    auto res = Parse(rangeStr, totalPages);
    return res.has_value();
}

} // namespace pdfinity::domain
