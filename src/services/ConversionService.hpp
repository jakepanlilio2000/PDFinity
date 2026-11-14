// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "domain/DocumentModel.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <functional>

namespace pdfinity::services {

class ConversionService {
public:
    static ConversionService& Instance();

    // 1. Image-to-PDF Conversion
    pdf_api::EngineResult<void> ConvertImagesToPdf(
        const std::vector<std::filesystem::path>& imagePaths,
        const std::filesystem::path& outputPdfPath,
        const pdf_api::ImageToPdfOptions& options,
        std::shared_ptr<pdf_api::IPdfEngine> engine
    );

    // 2. PDF-to-Image Export
    pdf_api::EngineResult<std::vector<std::filesystem::path>> ExportPdfToImages(
        domain::DocumentModel& doc,
        const std::filesystem::path& outputDirectory,
        const std::string& baseName,
        const pdf_api::PdfToImageOptions& options
    );

    // 3. PDF-to-Text Export
    pdf_api::EngineResult<void> ExportPdfToText(
        domain::DocumentModel& doc,
        const std::filesystem::path& outputPath,
        const std::vector<int>& pageIndices = {}
    );

    // 4. PDF-to-HTML Export with Explicit Fidelity Disclosures
    pdf_api::EngineResult<void> ExportPdfToHtml(
        domain::DocumentModel& doc,
        const std::filesystem::path& outputPath,
        const pdf_api::PdfToHtmlOptions& options = {}
    );

    // 5. Office Conversion Engine Detection
    pdf_api::ExternalEngineStatus DetectOfficeEngine();

    // 6. Office-to-PDF Conversion via Detected External Engine
    pdf_api::EngineResult<void> ConvertOfficeToPdf(
        const std::filesystem::path& officeDocumentPath,
        const std::filesystem::path& outputPdfPath,
        const pdf_api::OfficeConversionOptions& options = {}
    );

private:
    ConversionService() = default;
    std::filesystem::path FindOfficeExecutable();
};

} // namespace pdfinity::services
