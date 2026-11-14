// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "domain/DocumentModel.hpp"
#include <memory>
#include <vector>
#include <string>

namespace pdfinity::services {

class AnnotationService {
public:
    explicit AnnotationService() = default;
    ~AnnotationService() = default;

    // Collect all annotations from all pages of the document
    static std::vector<pdf_api::AnnotationData> GetAllAnnotations(const domain::DocumentModel& doc);

    // Helpers to create standard annotation data structures
    static pdf_api::AnnotationData CreateHighlight(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const std::vector<pdf_api::PdfRect>& quadPoints = {},
        const pdf_api::PdfColor& color = {255, 255, 0, 128},
        const std::string& contents = ""
    );

    static pdf_api::AnnotationData CreateUnderline(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const std::vector<pdf_api::PdfRect>& quadPoints = {},
        const pdf_api::PdfColor& color = {0, 128, 255, 255},
        const std::string& contents = ""
    );

    static pdf_api::AnnotationData CreateStrikeout(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const std::vector<pdf_api::PdfRect>& quadPoints = {},
        const pdf_api::PdfColor& color = {255, 0, 0, 255},
        const std::string& contents = ""
    );

    static pdf_api::AnnotationData CreateStickyNote(
        int pageIndex,
        const pdf_api::PdfPoint& position,
        const std::string& contents,
        const std::string& author = "PDFinity User",
        const pdf_api::PdfColor& color = {255, 220, 0, 255}
    );

    static pdf_api::AnnotationData CreateFreehand(
        int pageIndex,
        const std::vector<std::vector<pdf_api::PdfPoint>>& strokes,
        const pdf_api::PdfColor& color = {0, 0, 255, 255},
        float strokeWidth = 2.0f
    );

    static pdf_api::AnnotationData CreateShape(
        int pageIndex,
        pdf_api::AnnotationType type,
        const pdf_api::PdfRect& rect,
        const pdf_api::PdfColor& strokeColor = {255, 0, 0, 255},
        const pdf_api::PdfColor& interiorColor = {0, 0, 0, 0},
        float strokeWidth = 2.0f
    );

    static pdf_api::AnnotationData CreateStamp(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const std::string& stampName = "APPROVED"
    );

    static pdf_api::AnnotationData CreateFreeText(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const std::string& text,
        const pdf_api::PdfColor& textColor = {0, 0, 0, 255},
        const pdf_api::PdfColor& bgColor = {255, 255, 255, 200}
    );
};

} // namespace pdfinity::services
