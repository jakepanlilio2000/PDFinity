// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "ConversionService.hpp"
#include "application/Logger.hpp"

#include <QImage>
#include <QProcess>
#include <QTemporaryDir>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace pdfinity::services {

ConversionService& ConversionService::Instance() {
    static ConversionService instance;
    return instance;
}

pdf_api::EngineResult<void> ConversionService::ConvertImagesToPdf(
    const std::vector<std::filesystem::path>& imagePaths,
    const std::filesystem::path& outputPdfPath,
    const pdf_api::ImageToPdfOptions& options,
    std::shared_ptr<pdf_api::IPdfEngine> engine
) {
    if (!engine) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "PDF Engine is null");
    }
    if (imagePaths.empty()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "No image paths provided");
    }

    auto docRes = engine->CreateNewDocument();
    if (!docRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(docRes.error, "Failed to create new document: " + docRes.errorMessage);
    }
    auto docId = docRes.value;

    for (size_t i = 0; i < imagePaths.size(); ++i) {
        const auto& path = imagePaths[i];
        if (!std::filesystem::exists(path)) {
            engine->CloseDocument(docId);
            return pdf_api::EngineResult<void>::Fail(
                pdf_api::EngineError::FileNotFound, "Image file not found: " + path.string()
            );
        }

        QImage img(QString::fromStdString(path.string()));
        if (img.isNull()) {
            engine->CloseDocument(docId);
            return pdf_api::EngineResult<void>::Fail(
                pdf_api::EngineError::InvalidFormat, "Failed to decode image file: " + path.string()
            );
        }

        // Convert to 32-bit ARGB/BGRA format
        QImage converted = img.convertToFormat(QImage::Format_ARGB32);
        int pixelW = converted.width();
        int pixelH = converted.height();

        // Calculate page dimensions in points (72 points per inch)
        double pageW = 595.28; // Default A4
        double pageH = 841.89;

        if (options.pageFit == pdf_api::ImagePageFit::MatchImageSize) {
            pageW = pixelW * 72.0 / 96.0;
            pageH = pixelH * 72.0 / 96.0;
        } else if (options.pageFit == pdf_api::ImagePageFit::FitLetter) {
            pageW = 612.0;
            pageH = 792.0;
        }

        int pageIdx = static_cast<int>(i);
        auto insertPageRes = engine->InsertBlankPage(docId, pageIdx, pageW, pageH);
        if (!insertPageRes.has_value()) {
            engine->CloseDocument(docId);
            return pdf_api::EngineResult<void>::Fail(
                insertPageRes.error, "Failed to insert blank page: " + insertPageRes.errorMessage
            );
        }

        // Calculate image placement and aspect ratio
        double margin = options.marginPoints;
        double availW = std::max(10.0, pageW - 2.0 * margin);
        double availH = std::max(10.0, pageH - 2.0 * margin);

        double imgAspect = static_cast<double>(pixelW) / static_cast<double>(pixelH);
        double availAspect = availW / availH;

        double targetW = availW;
        double targetH = availH;

        if (imgAspect > availAspect) {
            targetH = targetW / imgAspect;
        } else {
            targetW = targetH * imgAspect;
        }

        double imgX = margin + (availW - targetW) * 0.5;
        double imgY = margin + (availH - targetH) * 0.5;

        // Extract raw 32-bit BGRA bytes
        const uint8_t* bits = converted.constBits();
        size_t byteCount = static_cast<size_t>(pixelW) * pixelH * 4;
        std::vector<uint8_t> bgraPixels(bits, bits + byteCount);

        auto imgRes = engine->InsertPageImage(
            docId, pageIdx, bgraPixels, pixelW, pixelH, imgX, imgY, targetW, targetH
        );
        if (!imgRes.has_value()) {
            engine->CloseDocument(docId);
            return pdf_api::EngineResult<void>::Fail(
                imgRes.error, "Failed to insert image on page " + std::to_string(pageIdx + 1) + ": " + imgRes.errorMessage
            );
        }
    }

    auto saveRes = engine->SaveCopy(docId, outputPdfPath);
    engine->CloseDocument(docId);

    if (!saveRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(saveRes.error, "Failed to save generated PDF: " + saveRes.errorMessage);
    }

    LOG_INFO("Conversion", "Image to PDF created successfully: " + outputPdfPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<std::vector<std::filesystem::path>> ConversionService::ExportPdfToImages(
    domain::DocumentModel& doc,
    const std::filesystem::path& outputDirectory,
    const std::string& baseName,
    const pdf_api::PdfToImageOptions& options
) {
    int pageCount = doc.GetPageCount();
    if (pageCount <= 0) {
        return pdf_api::EngineResult<std::vector<std::filesystem::path>>::Fail(
            pdf_api::EngineError::PageNotFound, "Document has no pages to export"
        );
    }

    std::error_code ec;
    if (!std::filesystem::exists(outputDirectory, ec)) {
        std::filesystem::create_directories(outputDirectory, ec);
    }

    std::vector<int> targetPages = options.pageIndices;
    if (targetPages.empty()) {
        targetPages.resize(pageCount);
        for (int i = 0; i < pageCount; ++i) targetPages[i] = i;
    }

    const char* ext = "png";
    const char* qtFormat = "PNG";
    if (options.format == pdf_api::ExportImageFormat::JPEG) {
        ext = "jpg";
        qtFormat = "JPEG";
    } else if (options.format == pdf_api::ExportImageFormat::BMP) {
        ext = "bmp";
        qtFormat = "BMP";
    } else if (options.format == pdf_api::ExportImageFormat::TIFF) {
        ext = "tif";
        qtFormat = "TIFF";
    }

    std::vector<std::filesystem::path> generatedFiles;
    double scale = std::max(0.2, options.dpi / 72.0);

    for (int pIndex : targetPages) {
        if (pIndex < 0 || pIndex >= pageCount) continue;

        pdf_api::RenderOptions renderOpt;
        renderOpt.scale = scale;
        renderOpt.renderAnnotations = true;

        auto renderRes = doc.RenderPage(pIndex, renderOpt);
        if (!renderRes.has_value()) {
            return pdf_api::EngineResult<std::vector<std::filesystem::path>>::Fail(
                renderRes.error, "Failed to render page " + std::to_string(pIndex + 1)
            );
        }

        const auto& imgData = renderRes.value;
        QImage qImg(
            imgData.pixels.data(),
            imgData.width,
            imgData.height,
            imgData.stride > 0 ? imgData.stride : imgData.width * 4,
            QImage::Format_ARGB32
        );

        std::string fileName = baseName + "_page_" + std::to_string(pIndex + 1) + "." + ext;
        std::filesystem::path outFilePath = outputDirectory / fileName;

        int quality = (options.format == pdf_api::ExportImageFormat::JPEG) ? options.jpegQuality : -1;
        if (!qImg.save(QString::fromStdString(outFilePath.string()), qtFormat, quality)) {
            return pdf_api::EngineResult<std::vector<std::filesystem::path>>::Fail(
                pdf_api::EngineError::SaveFailed, "Failed to save exported image: " + outFilePath.string()
            );
        }

        generatedFiles.push_back(outFilePath);
    }

    LOG_INFO("Conversion", "Exported " + std::to_string(generatedFiles.size()) + " page images to: " + outputDirectory.string());
    return pdf_api::EngineResult<std::vector<std::filesystem::path>>::Ok(std::move(generatedFiles));
}

pdf_api::EngineResult<void> ConversionService::ExportPdfToText(
    domain::DocumentModel& doc,
    const std::filesystem::path& outputPath,
    const std::vector<int>& pageIndices
) {
    int pageCount = doc.GetPageCount();
    if (pageCount <= 0) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Document has no pages");
    }

    std::vector<int> targetPages = pageIndices;
    if (targetPages.empty()) {
        targetPages.resize(pageCount);
        for (int i = 0; i < pageCount; ++i) targetPages[i] = i;
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to create text output file: " + outputPath.string()
        );
    }

    for (int pIndex : targetPages) {
        if (pIndex < 0 || pIndex >= pageCount) continue;

        out << "======================================================================\n";
        out << "--- Page " << (pIndex + 1) << " ---\n";
        out << "======================================================================\n\n";

        auto textRes = doc.ExtractPageText(pIndex);
        if (textRes.has_value()) {
            out << textRes.value << "\n\n";
        }
    }

    out.close();
    LOG_INFO("Conversion", "Exported document text to: " + outputPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> ConversionService::ExportPdfToHtml(
    domain::DocumentModel& doc,
    const std::filesystem::path& outputPath,
    const pdf_api::PdfToHtmlOptions& options
) {
    int pageCount = doc.GetPageCount();
    if (pageCount <= 0) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Document has no pages");
    }

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to write HTML file: " + outputPath.string()
        );
    }

    std::string title = options.title.empty() ? doc.GetPath().stem().string() : options.title;

    out << "<!DOCTYPE html>\n";
    out << "<html lang=\"en\">\n";
    out << "<head>\n";
    out << "  <meta charset=\"UTF-8\">\n";
    out << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    out << "  <title>" << title << "</title>\n";
    out << "  <style>\n";
    out << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f1f5f9; margin: 0; padding: 20px; }\n";
    out << "    .fidelity-disclaimer {\n";
    out << "      max-width: 800px; margin: 0 auto 24px auto; padding: 12px 16px; background-color: #fef3c7;\n";
    out << "      border: 1px solid #f59e0b; border-radius: 6px; color: #92400e; font-size: 13px; line-height: 1.5;\n";
    out << "    }\n";
    out << "    .pdf-page {\n";
    out << "      max-width: 800px; margin: 0 auto 30px auto; background: white; padding: 40px;\n";
    out << "      border-radius: 8px; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1); position: relative;\n";
    out << "    }\n";
    out << "    .page-header { font-size: 12px; color: #94a3b8; text-align: right; border-bottom: 1px solid #e2e8f0; padding-bottom: 8px; margin-bottom: 20px; }\n";
    out << "    .content-paragraph { font-size: 15px; line-height: 1.6; color: #1e293b; margin-bottom: 16px; white-space: pre-wrap; }\n";
    out << "  </style>\n";
    out << "</head>\n";
    out << "<body>\n\n";

    // Mandatory fidelity disclosure banner
    out << "  <div class=\"fidelity-disclaimer\">\n";
    out << "    <strong>⚠️ Notice on Layout Fidelity:</strong> PDF is a fixed-geometry print format. "
        << "This HTML export converts document text objects into responsive semantic paragraphs. "
        << "Exact column boundaries, wrapping points, and complex vector art may differ from native PDF rendering.\n";
    out << "  </div>\n\n";

    for (int p = 0; p < pageCount; ++p) {
        out << "  <div class=\"pdf-page\" id=\"page-" << (p + 1) << "\">\n";
        out << "    <div class=\"page-header\">Page " << (p + 1) << " of " << pageCount << "</div>\n";

        auto textRes = doc.ExtractPageText(p);
        if (textRes.has_value() && !textRes.value.empty()) {
            std::istringstream stream(textRes.value);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty()) {
                    out << "    <p class=\"content-paragraph\">" << line << "</p>\n";
                }
            }
        } else {
            out << "    <p class=\"content-paragraph\" style=\"color:#94a3b8; font-style:italic;\">[Blank page or non-text content]</p>\n";
        }

        out << "  </div>\n\n";
    }

    out << "</body>\n";
    out << "</html>\n";
    out.close();

    LOG_INFO("Conversion", "Exported document to HTML with fidelity disclosure: " + outputPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

std::filesystem::path ConversionService::FindOfficeExecutable() {
    // 1. Search system PATH
    QString pathFromEnv = QProcessEnvironment::systemEnvironment().value("PATH");
    QStringList paths = pathFromEnv.split(
#if defined(_WIN32)
        ';'
#else
        ':'
#endif
    );

    QString exeName = 
#if defined(_WIN32)
        "soffice.exe";
#else
        "soffice";
#endif

    for (const auto& p : paths) {
        std::filesystem::path candidate = std::filesystem::path(p.toStdString()) / exeName.toStdString();
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec)) {
            return candidate;
        }
    }

    // 2. Search well-known directory locations
#if defined(_WIN32)
    const std::vector<std::filesystem::path> wellKnown = {
        "C:\\Program Files\\LibreOffice\\program\\soffice.exe",
        "C:\\Program Files (x86)\\LibreOffice\\program\\soffice.exe",
        "C:\\LibreOffice\\program\\soffice.exe"
    };
    for (const auto& candidate : wellKnown) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
#else
    const std::vector<std::filesystem::path> wellKnownUnix = {
        "/usr/bin/soffice",
        "/usr/local/bin/soffice",
        "/Applications/LibreOffice.app/Contents/MacOS/soffice"
    };
    for (const auto& candidate : wellKnownUnix) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }
#endif

    return {};
}

pdf_api::ExternalEngineStatus ConversionService::DetectOfficeEngine() {
    pdf_api::ExternalEngineStatus status;
    status.engineName = "LibreOffice Headless Engine";
    status.installGuidance =
        "External Office conversion engine (LibreOffice) was not found in PATH or standard directories.\n"
        "To enable Word (.docx), Excel (.xlsx), and PowerPoint (.pptx) conversion:\n"
        "• Windows: Download and install LibreOffice from https://www.libreoffice.org/download and ensure 'soffice.exe' is added to PATH.\n"
        "• Linux: Run 'sudo apt install libreoffice' or 'sudo dnf install libreoffice'.\n"
        "• macOS: Install via DMG or 'brew install --cask libreoffice'.";

    auto exePath = FindOfficeExecutable();
    if (exePath.empty()) {
        status.isAvailable = false;
        return status;
    }

    status.executablePath = exePath.string();
    status.isAvailable = true;

    QProcess proc;
#if defined(Q_OS_WIN)
    proc.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= 0x08000000; // CREATE_NO_WINDOW
    });
#endif
    proc.start(QString::fromStdString(exePath.string()), {"--version"});
    if (proc.waitForFinished(3000)) {
        status.version = proc.readAllStandardOutput().trimmed().toStdString();
    }
    if (status.version.empty()) {
        status.version = "LibreOffice (Detected)";
    }

    return status;
}

pdf_api::EngineResult<void> ConversionService::ConvertOfficeToPdf(
    const std::filesystem::path& officeDocumentPath,
    const std::filesystem::path& outputPdfPath,
    const pdf_api::OfficeConversionOptions& options
) {
    if (!std::filesystem::exists(officeDocumentPath)) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Source office document not found: " + officeDocumentPath.string()
        );
    }

    auto status = DetectOfficeEngine();
    if (!status.isAvailable) {
        // Verification: Missing optional engines produce actionable messages, NOT fake success!
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::DependencyMissing,
            status.installGuidance
        );
    }

    // Execute LibreOffice headless conversion
    QTemporaryDir tempOutDir;
    if (!tempOutDir.isValid()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to create temporary output directory"
        );
    }

    QProcess proc;
#if defined(Q_OS_WIN)
    proc.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= 0x08000000; // CREATE_NO_WINDOW
    });
#endif
    QStringList args;
    args << "--headless"
         << "--convert-to" << "pdf"
         << "--outdir" << tempOutDir.path()
         << QString::fromStdString(officeDocumentPath.string());

    proc.start(QString::fromStdString(status.executablePath), args);
    int timeoutMs = options.timeoutSeconds * 1000;
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::RenderFailed, "Office conversion timed out after " + std::to_string(options.timeoutSeconds) + " seconds"
        );
    }

    if (proc.exitCode() != 0) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::RenderFailed,
            "Office conversion failed (exit code " + std::to_string(proc.exitCode()) + "): " + proc.readAllStandardError().toStdString()
        );
    }

    // Locate generated PDF in tempOutDir
    std::string stem = officeDocumentPath.stem().string();
    std::filesystem::path generatedPdf = std::filesystem::path(tempOutDir.path().toStdString()) / (stem + ".pdf");

    if (!std::filesystem::exists(generatedPdf)) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::FileNotFound, "Expected converted PDF not produced by external engine"
        );
    }

    std::error_code ec;
    std::filesystem::copy_file(generatedPdf, outputPdfPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Failed to copy converted PDF to destination: " + ec.message()
        );
    }

    LOG_INFO("Conversion", "Office document converted to PDF: " + outputPdfPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::services
