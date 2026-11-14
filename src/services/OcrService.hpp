// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <QImage>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace pdfinity::services {

class OcrService {
public:
    static OcrService& Instance();

    // Detect Tesseract engine and language packs
    pdf_api::OcrEngineStatus DetectOcrEngine();

    // Recognize text from a rendered page image
    pdf_api::EngineResult<pdf_api::OcrPageResult> RecognizePageImage(
        const QImage& pageImage,
        const pdf_api::OcrOptions& options,
        bool useSyntheticFallbackIfMissing = false
    );

    // Inject invisible searchable text runs onto a PDF page behind/over the scanned image
    pdf_api::EngineResult<void> ApplySearchableOcrLayer(
        domain::DocumentModel& doc,
        int pageIndex,
        const pdf_api::OcrPageResult& ocrResult
    );

    // Full workflow: Render pages -> OCR -> Inject text layer -> Save to dstPath
    pdf_api::EngineResult<void> ProcessDocumentOcr(
        domain::DocumentModel& doc,
        const std::filesystem::path& dstPath,
        const pdf_api::OcrOptions& options,
        std::function<void(int current, int total, const std::string& status)> progressCallback = nullptr
    );

private:
    OcrService() = default;
    std::filesystem::path FindTesseractExecutable();
};

} // namespace pdfinity::services
