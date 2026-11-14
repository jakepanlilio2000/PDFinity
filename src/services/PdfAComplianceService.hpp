// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>
#include <string>

namespace pdfinity::services {

class PdfAComplianceService {
public:
    static PdfAComplianceService& Instance();

    // Validate a PDF document against target ISO 19005 profile (PDF/A-1b or PDF/A-2b)
    pdf_api::EngineResult<pdf_api::PdfAValidationReport> ValidateCompliance(
        domain::DocumentModel& doc,
        pdf_api::PdfAProfile targetProfile = pdf_api::PdfAProfile::PdfA1b
    );

    // Convert document to compliant PDF/A where structurally viable
    pdf_api::EngineResult<pdf_api::PdfAValidationReport> ConvertToPdfA(
        domain::DocumentModel& doc,
        const std::filesystem::path& dstPath,
        const pdf_api::PdfAConversionOptions& options = {}
    );

private:
    PdfAComplianceService() = default;
};

} // namespace pdfinity::services
