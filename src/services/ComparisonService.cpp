// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ComparisonService.hpp"
#include "application/Logger.hpp"

#include <cmath>
#include <sstream>
#include <algorithm>

namespace pdfinity::services {

ComparisonService& ComparisonService::Instance() {
    static ComparisonService instance;
    return instance;
}

QImage ComparisonService::GenerateVisualDiffImage(
    const QImage& imgA,
    const QImage& imgB,
    double* outDiffPercent,
    int* outDiffPixels
) {
    if (imgA.isNull() && imgB.isNull()) {
        if (outDiffPercent) *outDiffPercent = 0.0;
        if (outDiffPixels) *outDiffPixels = 0;
        return {};
    }

    int w = std::max(imgA.width(), imgB.width());
    int h = std::max(imgA.height(), imgB.height());
    if (w <= 0 || h <= 0) {
        if (outDiffPercent) *outDiffPercent = 0.0;
        if (outDiffPixels) *outDiffPixels = 0;
        return {};
    }

    QImage diff(w, h, QImage::Format_ARGB32);
    diff.fill(Qt::white);

    int diffCount = 0;
    int totalPixels = w * h;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            bool inA = (x < imgA.width() && y < imgA.height());
            bool inB = (x < imgB.width() && y < imgB.height());

            if (inA && inB) {
                QRgb rgbA = imgA.pixel(x, y);
                QRgb rgbB = imgB.pixel(x, y);

                int dr = std::abs(qRed(rgbA) - qRed(rgbB));
                int dg = std::abs(qGreen(rgbA) - qGreen(rgbB));
                int db = std::abs(qBlue(rgbA) - qBlue(rgbB));

                // Threshold of 24 to ignore minor raster anti-aliasing jitter
                if (dr > 24 || dg > 24 || db > 24) {
                    diffCount++;
                    // Highlight modified/differing pixels in bright red
                    diff.setPixel(x, y, qRgb(239, 68, 68));
                } else {
                    // Muted background for unchanged pixels
                    int gray = qGray(rgbA);
                    int lightGray = 210 + (gray * 45 / 255);
                    diff.setPixel(x, y, qRgb(lightGray, lightGray, lightGray));
                }
            } else if (inA && !inB) {
                diffCount++;
                // Removed in B -> bright crimson
                diff.setPixel(x, y, qRgb(220, 38, 38));
            } else { // !inA && inB
                diffCount++;
                // Added in B -> bright emerald green
                diff.setPixel(x, y, qRgb(16, 185, 129));
            }
        }
    }

    if (outDiffPixels) *outDiffPixels = diffCount;
    if (outDiffPercent) {
        *outDiffPercent = (totalPixels > 0) ? (static_cast<double>(diffCount) / totalPixels) * 100.0 : 0.0;
    }

    return diff;
}

std::vector<pdf_api::TextDiffLine> ComparisonService::DiffPageTexts(
    int pageIndexA,
    const std::string& textA,
    int pageIndexB,
    const std::string& textB
) {
    std::vector<pdf_api::TextDiffLine> result;

    auto splitLines = [](const std::string& s) {
        std::vector<std::string> lines;
        std::istringstream stream(s);
        std::string l;
        while (std::getline(stream, l)) {
            // Trim carriage returns
            if (!l.empty() && l.back() == '\r') l.pop_back();
            lines.push_back(l);
        }
        return lines;
    };

    auto linesA = splitLines(textA);
    auto linesB = splitLines(textB);

    size_t i = 0;
    size_t j = 0;
    while (i < linesA.size() || j < linesB.size()) {
        if (i < linesA.size() && j < linesB.size()) {
            if (linesA[i] == linesB[j]) {
                pdf_api::TextDiffLine diff;
                diff.type = pdf_api::DiffType::Unchanged;
                diff.pageIndexA = pageIndexA;
                diff.pageIndexB = pageIndexB;
                diff.textA = linesA[i];
                diff.textB = linesB[j];
                result.push_back(diff);
                i++;
                j++;
            } else {
                pdf_api::TextDiffLine diff;
                diff.type = pdf_api::DiffType::Modified;
                diff.pageIndexA = pageIndexA;
                diff.pageIndexB = pageIndexB;
                diff.textA = linesA[i];
                diff.textB = linesB[j];
                result.push_back(diff);
                i++;
                j++;
            }
        } else if (i < linesA.size()) {
            pdf_api::TextDiffLine diff;
            diff.type = pdf_api::DiffType::Deleted;
            diff.pageIndexA = pageIndexA;
            diff.pageIndexB = pageIndexB;
            diff.textA = linesA[i];
            result.push_back(diff);
            i++;
        } else {
            pdf_api::TextDiffLine diff;
            diff.type = pdf_api::DiffType::Added;
            diff.pageIndexA = pageIndexA;
            diff.pageIndexB = pageIndexB;
            diff.textB = linesB[j];
            result.push_back(diff);
            j++;
        }
    }

    return result;
}

pdf_api::EngineResult<pdf_api::DocumentComparisonReport> ComparisonService::CompareDocuments(
    domain::DocumentModel& docA,
    domain::DocumentModel& docB,
    double renderDpi
) {
    if (!docA.IsValid() || !docB.IsValid()) {
        return pdf_api::EngineResult<pdf_api::DocumentComparisonReport>::Fail(
            pdf_api::EngineError::Unknown, "Both Document A and Document B must be valid models"
        );
    }

    pdf_api::DocumentComparisonReport report;
    report.pathDocA = docA.GetPath().string();
    report.pathDocB = docB.GetPath().string();
    report.pageCountA = docA.GetPageCount();
    report.pageCountB = docB.GetPageCount();

    int maxPages = std::max(report.pageCountA, report.pageCountB);
    double totalDiffPercent = 0.0;
    bool hasAnyDifference = (report.pageCountA != report.pageCountB);

    double scale = renderDpi / 72.0;
    pdf_api::RenderOptions renderOpt;
    renderOpt.scale = scale;
    renderOpt.renderAnnotations = true;

    for (int p = 0; p < maxPages; ++p) {
        pdf_api::PageComparisonResult pageRes;
        pageRes.pageIndex = p;

        QImage imgA;
        std::string textA;
        if (p < report.pageCountA) {
            auto rA = docA.RenderPage(p, renderOpt);
            if (rA.has_value()) {
                const auto& id = rA.value;
                imgA = QImage(id.pixels.data(), id.width, id.height, id.stride > 0 ? id.stride : id.width * 4, QImage::Format_ARGB32).copy();
            }
            auto tA = docA.ExtractPageText(p);
            if (tA.has_value()) textA = tA.value;
        }

        QImage imgB;
        std::string textB;
        if (p < report.pageCountB) {
            auto rB = docB.RenderPage(p, renderOpt);
            if (rB.has_value()) {
                const auto& id = rB.value;
                imgB = QImage(id.pixels.data(), id.width, id.height, id.stride > 0 ? id.stride : id.width * 4, QImage::Format_ARGB32).copy();
            }
            auto tB = docB.ExtractPageText(p);
            if (tB.has_value()) textB = tB.value;
        }

        // Visual diff
        GenerateVisualDiffImage(imgA, imgB, &pageRes.visualDifferencePercent, &pageRes.visualDifferencePixelCount);
        totalDiffPercent += pageRes.visualDifferencePercent;

        // Text diff
        auto diffLines = DiffPageTexts(p, textA, p, textB);
        for (const auto& line : diffLines) {
            if (line.type == pdf_api::DiffType::Added) pageRes.addedLinesCount++;
            else if (line.type == pdf_api::DiffType::Deleted) pageRes.deletedLinesCount++;
            else if (line.type == pdf_api::DiffType::Modified) pageRes.modifiedLinesCount++;
            report.textDiffs.push_back(line);
        }

        if (pageRes.visualDifferencePixelCount > 0 || pageRes.addedLinesCount > 0 || pageRes.deletedLinesCount > 0 || pageRes.modifiedLinesCount > 0) {
            hasAnyDifference = true;
        }

        report.pageResults.push_back(pageRes);
    }

    if (maxPages > 0) {
        report.overallVisualDifferencePercent = totalDiffPercent / maxPages;
    }
    report.isIdentical = !hasAnyDifference;

    LOG_INFO("Comparison", "Comparison complete. Pages: " + std::to_string(maxPages) +
             ", Diff: " + std::to_string(report.overallVisualDifferencePercent) + "%" +
             (report.isIdentical ? " (Identical)" : " (Differences Found)"));

    return pdf_api::EngineResult<pdf_api::DocumentComparisonReport>::Ok(std::move(report));
}

} // namespace pdfinity::services
