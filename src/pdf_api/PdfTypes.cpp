// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfTypes.hpp"

namespace pdfinity::pdf_api {

const char* GetEngineErrorMessage(EngineError error) {
    switch (error) {
        case EngineError::None: return "Success";
        case EngineError::FileNotFound: return "The specified PDF file was not found.";
        case EngineError::FileTooLarge: return "The PDF file exceeds the configured maximum file size limit.";
        case EngineError::PageCountLimitExceeded: return "The document page count exceeds the configured processing ceiling.";
        case EngineError::InvalidFormat: return "The file is not a valid PDF document or header is corrupted.";
        case EngineError::CorruptedData: return "The PDF stream or cross-reference table is corrupted.";
        case EngineError::PasswordRequired: return "The PDF document is password-protected. Password required.";
        case EngineError::IncorrectPassword: return "The provided document password is incorrect.";
        case EngineError::UnsupportedEncryption: return "The document uses an unsupported cryptographic security handler.";
        case EngineError::UnsupportedFeature: return "The document contains an unsupported PDF feature.";
        case EngineError::PageNotFound: return "The requested page index does not exist.";
        case EngineError::RenderFailed: return "Failed to render the document page.";
        case EngineError::SaveFailed: return "Failed to save the PDF document.";
        case EngineError::OutOfMemory: return "Insufficient memory to complete the requested operation.";
        case EngineError::DependencyMissing: return "A required external engine or executable dependency is not installed or not in PATH.";
        case EngineError::Unknown:
        default: return "An unknown PDF engine error occurred.";
    }
}

} // namespace pdfinity::pdf_api
