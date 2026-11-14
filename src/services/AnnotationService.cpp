// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AnnotationService.hpp"
#include <algorithm>

namespace pdfinity::services {

std::vector<pdf_api::AnnotationData> AnnotationService::GetAllAnnotations(const domain::DocumentModel& doc) {
    std::vector<pdf_api::AnnotationData> result;
    if (!doc.IsValid()) return result;

    int pageCount = doc.GetPageCount();
    for (int p = 0; p < pageCount; ++p) {
        auto res = doc.GetAnnotations(p);
        if (res.has_value()) {
            for (auto& a : res.value) {
                result.push_back(std::move(a));
            }
        }
    }
    return result;
}

pdf_api::AnnotationData AnnotationService::CreateHighlight(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const std::vector<pdf_api::PdfRect>& quadPoints,
    const pdf_api::PdfColor& color,
    const std::string& contents
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::Highlight;
    data.rect = rect;
    data.color = color;
    data.quadPoints = quadPoints;
    data.contents = contents;
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateUnderline(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const std::vector<pdf_api::PdfRect>& quadPoints,
    const pdf_api::PdfColor& color,
    const std::string& contents
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::Underline;
    data.rect = rect;
    data.color = color;
    data.quadPoints = quadPoints;
    data.contents = contents;
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateStrikeout(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const std::vector<pdf_api::PdfRect>& quadPoints,
    const pdf_api::PdfColor& color,
    const std::string& contents
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::Strikeout;
    data.rect = rect;
    data.color = color;
    data.quadPoints = quadPoints;
    data.contents = contents;
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateStickyNote(
    int pageIndex,
    const pdf_api::PdfPoint& position,
    const std::string& contents,
    const std::string& author,
    const pdf_api::PdfColor& color
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::TextNote;
    // Standard sticky note icon size: 24x24 points
    data.rect = pdf_api::PdfRect{ position.x, position.y, 24.0, 24.0 };
    data.color = color;
    data.contents = contents;
    data.author = author;
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateFreehand(
    int pageIndex,
    const std::vector<std::vector<pdf_api::PdfPoint>>& strokes,
    const pdf_api::PdfColor& color,
    float strokeWidth
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::FreehandInk;
    data.color = color;
    data.strokeWidth = strokeWidth;
    data.inkStrokes = strokes;

    // Calculate bounding box of all ink points
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (const auto& stroke : strokes) {
        for (const auto& pt : stroke) {
            minX = std::min(minX, pt.x);
            maxX = std::max(maxX, pt.x);
            minY = std::min(minY, pt.y);
            maxY = std::max(maxY, pt.y);
        }
    }
    if (minX <= maxX && minY <= maxY) {
        // Add padding for stroke width
        double pad = strokeWidth * 2.0;
        data.rect = pdf_api::PdfRect{ minX - pad, minY - pad, (maxX - minX) + 2 * pad, (maxY - minY) + 2 * pad };
    }
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateShape(
    int pageIndex,
    pdf_api::AnnotationType type,
    const pdf_api::PdfRect& rect,
    const pdf_api::PdfColor& strokeColor,
    const pdf_api::PdfColor& interiorColor,
    float strokeWidth
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = type;
    data.rect = rect;
    data.color = strokeColor;
    data.interiorColor = interiorColor;
    data.strokeWidth = strokeWidth;
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateStamp(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const std::string& stampName
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::Stamp;
    data.rect = rect;
    data.stampName = stampName;
    data.contents = stampName;
    data.color = pdf_api::PdfColor{ 200, 0, 0, 255 }; // Red stamp default
    return data;
}

pdf_api::AnnotationData AnnotationService::CreateFreeText(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const std::string& text,
    const pdf_api::PdfColor& textColor,
    const pdf_api::PdfColor& bgColor
) {
    pdf_api::AnnotationData data;
    data.pageIndex = pageIndex;
    data.type = pdf_api::AnnotationType::FreeText;
    data.rect = rect;
    data.contents = text;
    data.color = textColor;
    data.interiorColor = bgColor;
    data.strokeWidth = 1.0f;
    return data;
}

} // namespace pdfinity::services
