// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentAssemblyService.hpp"
#include "domain/PageRangeParser.hpp"
#include "application/Logger.hpp"
#include "security/PathSanitizer.hpp"
#include <iomanip>
#include <sstream>

namespace pdfinity::services {

DocumentAssemblyService::DocumentAssemblyService(std::shared_ptr<pdf_api::IPdfEngine> engine)
    : m_engine(std::move(engine)) {}

pdf_api::EngineResult<MergeResult> DocumentAssemblyService::MergeDocuments(
    const std::vector<MergeSource>& sources,
    const std::filesystem::path& outputPath
) {
    if (!m_engine) {
        return pdf_api::EngineResult<MergeResult>::Fail(pdf_api::EngineError::Unknown, "PDF Engine uninitialized");
    }
    if (sources.empty()) {
        return pdf_api::EngineResult<MergeResult>::Fail(pdf_api::EngineError::InvalidFormat, "No source documents provided for merge");
    }

    auto outSanitized = security::PathSanitizer::Canonicalize(outputPath.parent_path());
    if (!outSanitized) {
        return pdf_api::EngineResult<MergeResult>::Fail(pdf_api::EngineError::FileNotFound, "Invalid output directory");
    }

    auto newDocRes = m_engine->CreateNewDocument();
    if (!newDocRes.has_value()) {
        return pdf_api::EngineResult<MergeResult>::Fail(newDocRes.error, newDocRes.errorMessage);
    }
    pdf_api::DocumentId mergedDocId = newDocRes.value;

    int totalPages = 0;

    for (const auto& src : sources) {
        auto srcSanitized = security::PathSanitizer::Canonicalize(src.path);
        if (!srcSanitized) {
            m_engine->CloseDocument(mergedDocId);
            return pdf_api::EngineResult<MergeResult>::Fail(pdf_api::EngineError::FileNotFound, "Source file not found: " + src.path.string());
        }

        auto openRes = m_engine->OpenDocument(srcSanitized.value(), src.password);
        if (!openRes.has_value()) {
            m_engine->CloseDocument(mergedDocId);
            return pdf_api::EngineResult<MergeResult>::Fail(openRes.error, "Failed to open source file: " + src.path.filename().string());
        }
        pdf_api::DocumentId srcId = openRes.value;
        int srcPages = m_engine->GetPageCount(srcId);

        auto rangeRes = domain::PageRangeParser::Parse(src.pageRange, srcPages);
        if (!rangeRes.has_value()) {
            m_engine->CloseDocument(srcId);
            m_engine->CloseDocument(mergedDocId);
            return pdf_api::EngineResult<MergeResult>::Fail(rangeRes.error, rangeRes.errorMessage);
        }

        auto importRes = m_engine->ImportPages(mergedDocId, srcId, rangeRes.value, totalPages);
        m_engine->CloseDocument(srcId);

        if (!importRes.has_value()) {
            m_engine->CloseDocument(mergedDocId);
            return pdf_api::EngineResult<MergeResult>::Fail(importRes.error, "Failed to import pages from: " + src.path.filename().string());
        }

        totalPages += static_cast<int>(rangeRes.value.size());
    }

    auto saveRes = m_engine->SaveCopy(mergedDocId, outputPath);
    m_engine->CloseDocument(mergedDocId);

    if (!saveRes.has_value()) {
        return pdf_api::EngineResult<MergeResult>::Fail(saveRes.error, saveRes.errorMessage);
    }

    // Validation: Reopen saved output to verify integrity and page count
    auto verifyRes = m_engine->OpenDocument(outputPath);
    if (!verifyRes.has_value()) {
        return pdf_api::EngineResult<MergeResult>::Fail(pdf_api::EngineError::CorruptedData, "Merged output failed verification check");
    }
    int verifyPages = m_engine->GetPageCount(verifyRes.value);
    m_engine->CloseDocument(verifyRes.value);

    if (verifyPages != totalPages) {
        return pdf_api::EngineResult<MergeResult>::Fail(
            pdf_api::EngineError::CorruptedData,
            "Verification page count mismatch: expected " + std::to_string(totalPages) + " but got " + std::to_string(verifyPages)
        );
    }

    MergeResult result;
    result.totalPagesMerged = totalPages;
    result.outputPath = outputPath;
    std::error_code ec;
    result.outputSizeBytes = std::filesystem::file_size(outputPath, ec);

    LOG_INFO("Assembly", "Successfully merged " + std::to_string(sources.size()) + " sources into " + outputPath.filename().string() + " (" + std::to_string(totalPages) + " pages)");
    return pdf_api::EngineResult<MergeResult>::Ok(result);
}

pdf_api::EngineResult<SplitResult> DocumentAssemblyService::SplitDocument(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& outputDirectory,
    const SplitOptions& options
) {
    if (!m_engine) {
        return pdf_api::EngineResult<SplitResult>::Fail(pdf_api::EngineError::Unknown, "PDF Engine uninitialized");
    }

    auto srcSanitized = security::PathSanitizer::Canonicalize(sourcePath);
    if (!srcSanitized) {
        return pdf_api::EngineResult<SplitResult>::Fail(pdf_api::EngineError::FileNotFound, "Source document not found");
    }

    auto openRes = m_engine->OpenDocument(srcSanitized.value(), options.password);
    if (!openRes.has_value()) {
        return pdf_api::EngineResult<SplitResult>::Fail(openRes.error, openRes.errorMessage);
    }
    pdf_api::DocumentId srcId = openRes.value;
    int totalPages = m_engine->GetPageCount(srcId);

    std::vector<std::vector<int>> splitChunks;

    if (options.mode == SplitMode::IndividualPages) {
        for (int i = 0; i < totalPages; ++i) {
            splitChunks.push_back({i});
        }
    } else if (options.mode == SplitMode::EveryNPages) {
        int chunkSize = std::max(1, options.pagesPerChunk);
        for (int i = 0; i < totalPages; i += chunkSize) {
            std::vector<int> chunk;
            for (int p = i; p < std::min(i + chunkSize, totalPages); ++p) {
                chunk.push_back(p);
            }
            splitChunks.push_back(std::move(chunk));
        }
    } else if (options.mode == SplitMode::ByPageRanges) {
        for (const auto& rStr : options.customRanges) {
            auto rRes = domain::PageRangeParser::Parse(rStr, totalPages);
            if (!rRes.has_value()) {
                m_engine->CloseDocument(srcId);
                return pdf_api::EngineResult<SplitResult>::Fail(rRes.error, "Invalid range: " + rStr);
            }
            splitChunks.push_back(rRes.value);
        }
    } else if (options.mode == SplitMode::ByBookmarks) {
        auto bmRes = m_engine->GetBookmarks(srcId);
        std::vector<int> splitPoints;
        if (bmRes.has_value()) {
            for (const auto& bm : bmRes.value) {
                if (bm.targetPageIndex >= 0 && bm.targetPageIndex < totalPages) {
                    splitPoints.push_back(bm.targetPageIndex);
                }
            }
        }
        std::sort(splitPoints.begin(), splitPoints.end());
        splitPoints.erase(std::unique(splitPoints.begin(), splitPoints.end()), splitPoints.end());

        if (splitPoints.empty() || splitPoints.front() != 0) {
            splitPoints.insert(splitPoints.begin(), 0);
        }
        splitPoints.push_back(totalPages);

        for (size_t s = 0; s + 1 < splitPoints.size(); ++s) {
            int start = splitPoints[s];
            int end = splitPoints[s + 1];
            std::vector<int> chunk;
            for (int p = start; p < end; ++p) {
                chunk.push_back(p);
            }
            if (!chunk.empty()) splitChunks.push_back(std::move(chunk));
        }
    }

    SplitResult result;
    int chunkIdx = 1;

    for (const auto& chunk : splitChunks) {
        auto chunkDocRes = m_engine->CreateNewDocument();
        if (!chunkDocRes.has_value()) {
            m_engine->CloseDocument(srcId);
            return pdf_api::EngineResult<SplitResult>::Fail(chunkDocRes.error, chunkDocRes.errorMessage);
        }
        pdf_api::DocumentId chunkDocId = chunkDocRes.value;

        auto importRes = m_engine->ImportPages(chunkDocId, srcId, chunk, 0);
        if (!importRes.has_value()) {
            m_engine->CloseDocument(chunkDocId);
            m_engine->CloseDocument(srcId);
            return pdf_api::EngineResult<SplitResult>::Fail(importRes.error, "Failed to import pages into chunk");
        }

        std::ostringstream nameOss;
        nameOss << options.filePrefix << std::setfill('0') << std::setw(3) << chunkIdx++ << ".pdf";
        std::filesystem::path outPath = outputDirectory / nameOss.str();

        auto saveRes = m_engine->SaveCopy(chunkDocId, outPath);
        m_engine->CloseDocument(chunkDocId);

        if (!saveRes.has_value()) {
            m_engine->CloseDocument(srcId);
            return pdf_api::EngineResult<SplitResult>::Fail(saveRes.error, "Failed to save split chunk: " + outPath.string());
        }

        result.generatedFiles.push_back(outPath);
        result.totalPagesProcessed += static_cast<int>(chunk.size());
    }

    m_engine->CloseDocument(srcId);
    LOG_INFO("Assembly", "Split document into " + std::to_string(result.generatedFiles.size()) + " files");
    return pdf_api::EngineResult<SplitResult>::Ok(result);
}

pdf_api::EngineResult<void> DocumentAssemblyService::ExtractPages(
    const std::filesystem::path& sourcePath,
    const std::string& pageRange,
    const std::filesystem::path& outputPath,
    const security::SecureString& password
) {
    MergeSource src;
    src.path = sourcePath;
    src.pageRange = pageRange;
    src.password = password;

    auto res = MergeDocuments({src}, outputPath);
    if (!res.has_value()) {
        return pdf_api::EngineResult<void>::Fail(res.error, res.errorMessage);
    }
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> DocumentAssemblyService::ReorderDocument(
    const std::filesystem::path& sourcePath,
    const std::vector<int>& newPageOrder,
    const std::filesystem::path& outputPath,
    const security::SecureString& password
) {
    if (newPageOrder.empty()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "New page order cannot be empty");
    }

    auto srcSanitized = security::PathSanitizer::Canonicalize(sourcePath);
    if (!srcSanitized) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Source document not found");
    }

    auto openRes = m_engine->OpenDocument(srcSanitized.value(), password);
    if (!openRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(openRes.error, openRes.errorMessage);
    }
    pdf_api::DocumentId srcId = openRes.value;

    auto newDocRes = m_engine->CreateNewDocument();
    if (!newDocRes.has_value()) {
        m_engine->CloseDocument(srcId);
        return pdf_api::EngineResult<void>::Fail(newDocRes.error, newDocRes.errorMessage);
    }
    pdf_api::DocumentId newDocId = newDocRes.value;

    auto importRes = m_engine->ImportPages(newDocId, srcId, newPageOrder, 0);
    m_engine->CloseDocument(srcId);

    if (!importRes.has_value()) {
        m_engine->CloseDocument(newDocId);
        return pdf_api::EngineResult<void>::Fail(importRes.error, importRes.errorMessage);
    }

    auto saveRes = m_engine->SaveCopy(newDocId, outputPath);
    m_engine->CloseDocument(newDocId);

    if (!saveRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(saveRes.error, saveRes.errorMessage);
    }
    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::services
