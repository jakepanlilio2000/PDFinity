// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include <string>
#include <vector>

namespace pdfinity::domain {

class PageRangeParser {
public:
    // Parses human 1-based page range string (e.g. "1-3, 5, 8-10", "odd", "even", "all", "reverse")
    // into 0-based page indices validated against totalPages.
    static pdf_api::EngineResult<std::vector<int>> Parse(
        const std::string& rangeStr,
        int totalPages
    );

    // Formats a list of 0-based page indices into a compact 1-based human string (e.g. "1-3, 5, 8-10")
    static std::string Format(const std::vector<int>& indices);

    // Validates whether rangeStr is syntactically valid for totalPages
    static bool IsValid(const std::string& rangeStr, int totalPages);
};

} // namespace pdfinity::domain
