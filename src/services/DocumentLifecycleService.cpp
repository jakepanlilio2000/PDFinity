// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentLifecycleService.hpp"
#include "security/PathSanitizer.hpp"
#include "application/Logger.hpp"

namespace pdfinity::services {

DocumentLifecycleService::DocumentLifecycleService(std::shared_ptr<pdf_api::IPdfEngine> engine)
    : m_engine(std::move(engine)) {}

pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>> DocumentLifecycleService::OpenDocument(
    const std::filesystem::path& path,
    const security::SecureString& password
) {
    if (!m_engine) {
        return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Fail(
            pdf_api::EngineError::Unknown, "PDF Engine is null"
        );
    }

    auto sanitizedPath = security::PathSanitizer::Canonicalize(path);
    if (!sanitizedPath) {
        LOG_WARN("Lifecycle", "Rejected unsafe document path: " + path.string());
        return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Fail(
            pdf_api::EngineError::FileNotFound, "Unsafe or invalid file path"
        );
    }

    auto openRes = m_engine->OpenDocument(sanitizedPath.value(), password);
    if (!openRes.has_value()) {
        LOG_WARN("Lifecycle", "Engine failed to open document: " + std::string(pdf_api::GetEngineErrorMessage(openRes.error)));
        return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Fail(
            openRes.error, openRes.errorMessage
        );
    }

    auto docModel = std::make_unique<domain::DocumentModel>(openRes.value, m_engine, sanitizedPath.value());
    LOG_INFO("Lifecycle", "Document model constructed successfully for: " + sanitizedPath.value().filename().string());
    return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Ok(std::move(docModel));
}

pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>> DocumentLifecycleService::OpenMemory(
    std::span<const uint8_t> buffer,
    const security::SecureString& password
) {
    if (!m_engine) {
        return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Fail(
            pdf_api::EngineError::Unknown, "PDF Engine is null"
        );
    }

    auto openRes = m_engine->OpenMemory(buffer, password);
    if (!openRes.has_value()) {
        return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Fail(
            openRes.error, openRes.errorMessage
        );
    }

    auto docModel = std::make_unique<domain::DocumentModel>(openRes.value, m_engine);
    return pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>>::Ok(std::move(docModel));
}

pdf_api::EngineResult<void> DocumentLifecycleService::SaveCopy(
    const domain::DocumentModel& document,
    const std::filesystem::path& targetPath
) {
    if (!document.IsValid()) {
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::Unknown, "Cannot save invalid document"
        );
    }

    auto sanitizedTarget = security::PathSanitizer::Canonicalize(targetPath);
    if (!sanitizedTarget) {
        LOG_WARN("Lifecycle", "Rejected unsafe save target path: " + targetPath.string());
        return pdf_api::EngineResult<void>::Fail(
            pdf_api::EngineError::SaveFailed, "Invalid target path"
        );
    }

    return document.SaveCopy(sanitizedTarget.value());
}

} // namespace pdfinity::services
