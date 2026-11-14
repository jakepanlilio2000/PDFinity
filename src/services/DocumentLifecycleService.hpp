// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/IPdfEngine.hpp"
#include "domain/DocumentModel.hpp"
#include <memory>
#include <filesystem>
#include <algorithm>

namespace pdfinity::services {

class DocumentLifecycleService {
public:
    explicit DocumentLifecycleService(std::shared_ptr<pdf_api::IPdfEngine> engine);
    ~DocumentLifecycleService() = default;

    pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>> OpenDocument(
        const std::filesystem::path& path,
        const security::SecureString& password = {}
    );

    pdf_api::EngineResult<std::unique_ptr<domain::DocumentModel>> OpenMemory(
        std::span<const uint8_t> buffer,
        const security::SecureString& password = {}
    );

    pdf_api::EngineResult<void> SaveCopy(
        const domain::DocumentModel& document,
        const std::filesystem::path& targetPath
    );

    std::shared_ptr<pdf_api::IPdfEngine> GetEngine() const { return m_engine; }

private:
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
};

} // namespace pdfinity::services
