// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <mutex>

namespace pdfinity::services {

class RedactionService {
public:
    static RedactionService& Instance();

    void AddRedactionMark(const pdf_api::RedactionMark& mark);
    void RemoveRedactionMark(const std::string& markId);
    std::vector<pdf_api::RedactionMark> GetRedactionMarks() const;
    std::vector<pdf_api::RedactionMark> GetRedactionMarksForPage(int pageIndex) const;
    void ClearRedactionMarks();

    pdf_api::EngineResult<std::vector<pdf_api::RedactionTargetAnalysis>> AnalyzeAllMarks(
        domain::DocumentModel& doc
    );

    pdf_api::EngineResult<pdf_api::RedactionVerificationReport> ApplyRedactionsAndSave(
        domain::DocumentModel& doc,
        const std::filesystem::path& srcPath,
        const std::filesystem::path& dstPath,
        const pdf_api::SanitizationOptions& sanitizationOptions = {}
    );

    static pdf_api::EngineResult<bool> VerifyRedactedDocument(
        const std::filesystem::path& redactedPdfPath,
        const std::vector<std::string>& forbiddenSecrets
    );

    static std::string GenerateVerificationReport(
        const pdf_api::RedactionVerificationReport& report
    );

private:
    RedactionService() = default;
    mutable std::mutex m_mutex;
    std::vector<pdf_api::RedactionMark> m_marks;
};

} // namespace pdfinity::services
