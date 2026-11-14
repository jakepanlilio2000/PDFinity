// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/PdfTypes.hpp"
#include "pdf_api/IPdfEngine.hpp"
#include "security/SecureString.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <string>

namespace pdfinity::services {

struct MergeSource {
    std::filesystem::path path;
    std::string pageRange{"all"}; // "all", "1-3, 5", "odd", etc.
    security::SecureString password;
};

struct MergeResult {
    int totalPagesMerged{0};
    uint64_t outputSizeBytes{0};
    std::filesystem::path outputPath;
};

enum class SplitMode {
    IndividualPages, // Explode to 1 page per file
    EveryNPages,     // Split into chunks of N pages
    ByPageRanges,    // Specific ranges e.g. {"1-3", "4-7"}
    ByBookmarks      // Split at top-level bookmark destination boundaries
};

struct SplitOptions {
    SplitMode mode{SplitMode::IndividualPages};
    int pagesPerChunk{1};
    std::vector<std::string> customRanges;
    std::string filePrefix{"split_"};
    security::SecureString password;
};

struct SplitResult {
    std::vector<std::filesystem::path> generatedFiles;
    int totalPagesProcessed{0};
};

class DocumentAssemblyService {
public:
    explicit DocumentAssemblyService(std::shared_ptr<pdf_api::IPdfEngine> engine);
    ~DocumentAssemblyService() = default;

    // Merge multiple documents and page ranges into a single new output PDF
    pdf_api::EngineResult<MergeResult> MergeDocuments(
        const std::vector<MergeSource>& sources,
        const std::filesystem::path& outputPath
    );

    // Split a document into multiple files based on the chosen strategy
    pdf_api::EngineResult<SplitResult> SplitDocument(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& outputDirectory,
        const SplitOptions& options
    );

    // Extract selected pages to a new output file
    pdf_api::EngineResult<void> ExtractPages(
        const std::filesystem::path& sourcePath,
        const std::string& pageRange,
        const std::filesystem::path& outputPath,
        const security::SecureString& password = {}
    );

    // Reorder pages of an existing document into a new output file
    pdf_api::EngineResult<void> ReorderDocument(
        const std::filesystem::path& sourcePath,
        const std::vector<int>& newPageOrder,
        const std::filesystem::path& outputPath,
        const security::SecureString& password = {}
    );

private:
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

} // namespace pdfinity::services
