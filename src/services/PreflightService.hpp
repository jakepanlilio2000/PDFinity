// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"

namespace pdfinity::services {

struct PreflightOptions {
    double minImageDpi{150.0};
    bool requireAllFontsEmbedded{true};
    bool checkPageBoxInconsistencies{true};
    bool warnOnEncryption{true};
    bool warnOnMissingMetadata{true};
};

class PreflightService {
public:
    static PreflightService& Instance();

    // Run comprehensive document preflight inspection
    pdf_api::EngineResult<pdf_api::PreflightReport> RunPreflight(
        domain::DocumentModel& doc,
        const PreflightOptions& options = {}
    );

private:
    PreflightService() = default;
};

} // namespace pdfinity::services
