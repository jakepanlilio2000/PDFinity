// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"

namespace pdfinity::services {

class AccessibilityService {
public:
    static AccessibilityService& Instance();

    // Inspect document for Tagged PDF and accessibility compliance
    pdf_api::EngineResult<pdf_api::AccessibilityReport> InspectAccessibility(
        domain::DocumentModel& doc
    );

private:
    AccessibilityService() = default;
};

} // namespace pdfinity::services
