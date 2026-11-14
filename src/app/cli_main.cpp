// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "application/AppContext.hpp"
#include "application/Logger.hpp"
#include "app/BatchCliHandler.hpp"
#include "app/McpCliHandler.hpp"
#include "app/RemoteCliHandler.hpp"
#include "services/DocumentAssemblyService.hpp"
#include "services/DocumentLifecycleService.hpp"
#include "services/OcrService.hpp"
#include "services/RedactionService.hpp"
#include "services/SignatureService.hpp"
#include "domain/DocumentModel.hpp"
#include "pdf_api/IPdfEngine.hpp"
#include "security/PathSanitizer.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

namespace {
namespace pdf_api = pdfinity::pdf_api;
void PrintCliHelp() {
    std::cout << "PDFinity CLI 1.1.0 Stable\n"
              << "Published by StackSync Solutions (https://www.stacksyncsolutions.com/)\n"
              << "Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)\n\n"
              << "Usage: pdfinity-cli <command> [options] [arguments]\n\n"
              << "General Options:\n"
              << "  -h, --help            Show this help message and exit\n"
              << "  -v, --version         Show application version and exit\n"
              << "  --about               Show publisher, lead programmer, and license info\n"
              << "  --headless-smoke      Execute headless startup/shutdown diagnostic and exit\n\n"
              << "Document Processing Commands:\n"
              << "  merge                 Merge multiple PDF documents into one\n"
              << "  split                 Split a PDF document into parts or single pages\n"
              << "  ocr                   Perform OCR text recognition on scanned PDFs\n"
              << "  redact                Apply search-and-redact to sanitize text\n"
              << "  sign                  Apply cryptographic digital signature (PKCS#7)\n\n"
              << "Batch Automation Commands:\n"
              << "  batch                 Execute automated multi-file workflow (use 'pdfinity-cli batch --help')\n"
              << "  batch-profiles        List, export, and import reusable batch profiles\n"
              << "  batch-history         Inspect or clear past batch execution logs\n\n"
              << "Model Context Protocol (MCP) & Remote Commands:\n"
              << "  mcp-server / --mcp    Start Model Context Protocol stdio server (use 'pdfinity-cli mcp-server --help')\n"
              << "  remote-client         Connect as a Remote Controller client (use 'pdfinity-cli remote-client --help')\n";
}

void PrintCliVersion() {
    std::cout << "PDFinity CLI 1.0.9\n"
              << "Published by StackSync Solutions (https://www.stacksyncsolutions.com/)\n"
              << "Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)\n"
              << "License: Mozilla Public License Version 2.0 (MPL-2.0)\n";
}

int HandleMerge(int argc, char* argv[]) {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" || arg == "-o") {
            if (++i < argc) output = argv[i];
        } else if (arg == "--input" || arg == "-i") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                inputs.push_back(argv[++i]);
            }
        } else if (arg[0] != '-') {
            inputs.push_back(arg);
        }
    }

    if (inputs.size() < 2 || output.empty()) {
        std::cerr << "Usage: pdfinity-cli merge --input <file1.pdf> <file2.pdf> ... --output <merged.pdf>\n";
        return 1;
    }

    auto engine = pdfinity::app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        std::cerr << "Fatal Error: PDF Engine is unavailable.\n";
        return 1;
    }

    std::vector<pdfinity::services::MergeSource> sources;
    for (const auto& in : inputs) {
        sources.push_back({in, "all", {}});
    }

    pdfinity::services::DocumentAssemblyService assembly(engine);
    auto res = assembly.MergeDocuments(sources, output);
    if (!res) {
        std::cerr << "Merge failed: " << pdf_api::GetEngineErrorMessage(res.error) << "\n";
        return 1;
    }

    std::cout << "[SUCCESS] Merged " << res.value.totalPagesMerged << " pages into " << output.string() << "\n";
    return 0;
}

int HandleSplit(int argc, char* argv[]) {
    std::filesystem::path input;
    std::filesystem::path outputDir = ".";
    std::string pageRange;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" || arg == "-i") {
            if (++i < argc) input = argv[i];
        } else if (arg == "--output" || arg == "-o") {
            if (++i < argc) outputDir = argv[i];
        } else if (arg == "--pages" || arg == "-p") {
            if (++i < argc) pageRange = argv[i];
        }
    }

    if (input.empty()) {
        std::cerr << "Usage: pdfinity-cli split --input <document.pdf> [--output <output_dir>] [--pages <range>]\n";
        return 1;
    }

    auto engine = pdfinity::app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        std::cerr << "Fatal Error: PDF Engine is unavailable.\n";
        return 1;
    }

    pdfinity::services::DocumentAssemblyService assembly(engine);
    if (!pageRange.empty()) {
        std::filesystem::path outFile = outputDir / ("split_" + input.stem().string() + ".pdf");
        auto res = assembly.ExtractPages(input, pageRange, outFile);
        if (!res) {
            std::cerr << "Extract failed: " << pdf_api::GetEngineErrorMessage(res.error) << "\n";
            return 1;
        }
        std::cout << "[SUCCESS] Extracted pages [" << pageRange << "] to " << outFile.string() << "\n";
        return 0;
    }

    pdfinity::services::SplitOptions options;
    options.mode = pdfinity::services::SplitMode::IndividualPages;
    auto res = assembly.SplitDocument(input, outputDir, options);
    if (!res) {
        std::cerr << "Split failed: " << pdf_api::GetEngineErrorMessage(res.error) << "\n";
        return 1;
    }

    std::cout << "[SUCCESS] Split document into " << res.value.generatedFiles.size() << " files in " << outputDir.string() << "\n";
    return 0;
}

int HandleOcr(int argc, char* argv[]) {
    std::filesystem::path input;
    std::filesystem::path output;
    std::string lang = "eng";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" || arg == "-i") {
            if (++i < argc) input = argv[i];
        } else if (arg == "--output" || arg == "-o") {
            if (++i < argc) output = argv[i];
        } else if (arg == "--lang" || arg == "-l") {
            if (++i < argc) lang = argv[i];
        }
    }

    if (input.empty() || output.empty()) {
        std::cerr << "Usage: pdfinity-cli ocr --input <scanned.pdf> --output <searchable.pdf> [--lang eng]\n";
        return 1;
    }

    auto engine = pdfinity::app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        std::cerr << "Fatal Error: PDF Engine is unavailable.\n";
        return 1;
    }

    pdfinity::services::DocumentLifecycleService lifecycle(engine);
    auto docRes = lifecycle.OpenDocument(input);
    if (!docRes) {
        std::cerr << "Failed to open document: " << pdf_api::GetEngineErrorMessage(docRes.error) << "\n";
        return 1;
    }

    pdf_api::OcrOptions opts;
    opts.languageCode = lang;
    auto ocrRes = pdfinity::services::OcrService::Instance().ProcessDocumentOcr(*docRes.value, output, opts);
    if (!ocrRes) {
        std::cerr << "OCR processing failed: " << pdf_api::GetEngineErrorMessage(ocrRes.error) << "\n";
        return 1;
    }

    std::cout << "[SUCCESS] OCR completed successfully: " << output.string() << "\n";
    return 0;
}

int HandleRedact(int argc, char* argv[]) {
    std::filesystem::path input;
    std::filesystem::path output;
    std::string pattern;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" || arg == "-i") {
            if (++i < argc) input = argv[i];
        } else if (arg == "--output" || arg == "-o") {
            if (++i < argc) output = argv[i];
        } else if (arg == "--pattern" || arg == "-p") {
            if (++i < argc) pattern = argv[i];
        }
    }

    if (input.empty() || output.empty() || pattern.empty()) {
        std::cerr << "Usage: pdfinity-cli redact --input <in.pdf> --output <sanitized.pdf> --pattern <text_to_redact>\n";
        return 1;
    }

    auto engine = pdfinity::app::AppContext::Instance().GetPdfEngine();
    if (!engine) {
        std::cerr << "Fatal Error: PDF Engine is unavailable.\n";
        return 1;
    }

    pdfinity::services::DocumentLifecycleService lifecycle(engine);
    auto docRes = lifecycle.OpenDocument(input);
    if (!docRes) {
        std::cerr << "Failed to open document: " << pdf_api::GetEngineErrorMessage(docRes.error) << "\n";
        return 1;
    }

    auto& redaction = pdfinity::services::RedactionService::Instance();
    redaction.ClearRedactionMarks();

    int pageCount = docRes.value->GetPageCount();
    for (int p = 0; p < pageCount; ++p) {
        auto searchRes = docRes.value->SearchPageText(p, pattern, false, false);
        if (!searchRes) continue;
        for (const auto& m : searchRes.value) {
            for (const auto& r : m.rects) {
                pdf_api::RedactionMark mark;
                mark.id = "cli_mark_" + std::to_string(p) + "_" + std::to_string(m.charIndex);
                mark.pageIndex = p;
                mark.rect = r;
                mark.overlayText = "[REDACTED]";
                mark.reason = "CLI Automated Sanitize";
                redaction.AddRedactionMark(mark);
            }
        }
    }

    auto applyRes = redaction.ApplyRedactionsAndSave(*docRes.value, input, output);
    if (!applyRes) {
        std::cerr << "Redaction failed: " << pdf_api::GetEngineErrorMessage(applyRes.error) << "\n";
        return 1;
    }

    std::cout << "[SUCCESS] Applied redactions: " << applyRes.value.redactionsCount << " regions sanitized in " << output.string() << "\n";
    return 0;
}

int HandleSign(int argc, char* argv[]) {
    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path certPath;
    std::string password;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" || arg == "-i") {
            if (++i < argc) input = argv[i];
        } else if (arg == "--output" || arg == "-o") {
            if (++i < argc) output = argv[i];
        } else if (arg == "--cert" || arg == "-c") {
            if (++i < argc) certPath = argv[i];
        } else if (arg == "--password" || arg == "-p") {
            if (++i < argc) password = argv[i];
        }
    }

    if (input.empty() || output.empty() || certPath.empty()) {
        std::cerr << "Usage: pdfinity-cli sign --input <in.pdf> --output <signed.pdf> --cert <cert.p12> [--password <pwd>]\n";
        return 1;
    }

    pdf_api::CryptographicSignOptions opts;
    opts.pkcs12CertificatePath = certPath;
    opts.signerName = "PDFinity CLI Automated Signer";
    opts.reason = "Cryptographic Authorization";

    auto signRes = pdfinity::services::SignatureService::SignDocumentIncremental(
        input, output, opts, pdfinity::security::SecureString(password)
    );

    if (!signRes) {
        std::cerr << "Digital signature failed: " << pdf_api::GetEngineErrorMessage(signRes.error) << "\n";
        return 1;
    }

    std::cout << "[SUCCESS] Digitally signed document saved to " << output.string() << "\n";
    return 0;
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintCliHelp();
        return 0;
    }

    std::string firstArg = argv[1];

    if (firstArg == "--help" || firstArg == "-h" || firstArg == "help") {
        PrintCliHelp();
        return 0;
    }

    if (firstArg == "--version" || firstArg == "-v" || firstArg == "version" || firstArg == "--about") {
        PrintCliVersion();
        return 0;
    }

    // 1. Model Context Protocol Stdio Server Command
    if (pdfinity::app::McpCliHandler::IsMcpCommand(argc, argv)) {
        if (!pdfinity::app::AppContext::Instance().Initialize()) {
            std::cerr << "Fatal Error: Failed to initialize PDFinity engine for MCP server.\n";
            return 1;
        }
        int code = pdfinity::app::McpCliHandler::Execute(argc, argv);
        pdfinity::app::AppContext::Instance().Shutdown();
        return code;
    }

    // 2. Batch Processing CLI Automation Commands
    if (pdfinity::app::BatchCliHandler::IsBatchCommand(argc, argv)) {
        if (!pdfinity::app::AppContext::Instance().Initialize()) {
            std::cerr << "Fatal Error: Failed to initialize PDFinity engine for batch automation.\n";
            return 1;
        }
        int code = pdfinity::app::BatchCliHandler::Execute(argc, argv);
        pdfinity::app::AppContext::Instance().Shutdown();
        return code;
    }

    // 3. Remote Controller Client CLI Command
    if (pdfinity::app::RemoteCliHandler::IsRemoteCommand(argc, argv)) {
        return pdfinity::app::RemoteCliHandler::Execute(argc, argv);
    }

    // 4. Headless Smoke Diagnostic
    if (firstArg == "--headless-smoke") {
        if (!pdfinity::app::AppContext::Instance().Initialize()) {
            std::cerr << "Fatal Error: Failed to initialize PDFinity engine.\n";
            return 1;
        }
        LOG_INFO("Smoke", "Executing headless smoke diagnostic in CLI runner...");
        const auto& platform = pdfinity::app::AppContext::Instance().GetPlatform();
        std::cout << "[PASS] PDFinity engine and platform abstraction (" << platform.GetPlatformName() << ") operational.\n";
        pdfinity::app::AppContext::Instance().Shutdown();
        return 0;
    }

    // 5. Document Processing Commands (merge, split, ocr, redact, sign)
    if (!pdfinity::app::AppContext::Instance().Initialize()) {
        std::cerr << "Fatal Error: Failed to initialize PDFinity engine.\n";
        return 1;
    }

    int result = 0;
    if (firstArg == "merge") {
        result = HandleMerge(argc, argv);
    } else if (firstArg == "split") {
        result = HandleSplit(argc, argv);
    } else if (firstArg == "ocr") {
        result = HandleOcr(argc, argv);
    } else if (firstArg == "redact") {
        result = HandleRedact(argc, argv);
    } else if (firstArg == "sign") {
        result = HandleSign(argc, argv);
    } else {
        std::cerr << "Unknown command: " << firstArg << "\nRun 'pdfinity-cli --help' for available commands.\n";
        result = 1;
    }

    pdfinity::app::AppContext::Instance().Shutdown();
    return result;
}
