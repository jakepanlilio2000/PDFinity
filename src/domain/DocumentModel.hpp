// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/IPdfEngine.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace pdfinity::domain {

class DocumentModel {
public:
    DocumentModel(pdf_api::DocumentId id, std::shared_ptr<pdf_api::IPdfEngine> engine, std::filesystem::path path = {});
    ~DocumentModel();

    DocumentModel(const DocumentModel&) = delete;
    DocumentModel& operator=(const DocumentModel&) = delete;

    DocumentModel(DocumentModel&& other) noexcept;
    DocumentModel& operator=(DocumentModel&& other) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return m_id != pdf_api::InvalidDocumentId && m_engine != nullptr; }
    [[nodiscard]] std::shared_ptr<pdf_api::IPdfEngine> GetEngine() const noexcept { return m_engine; }
    [[nodiscard]] pdf_api::DocumentId GetId() const noexcept { return m_id; }
    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return m_path; }
    [[nodiscard]] int GetPageCount() const noexcept { return m_pageCount; }
    [[nodiscard]] uint64_t GetFileSize() const noexcept { return m_fileSizeBytes; }
    [[nodiscard]] const pdf_api::DocumentMetadata& GetMetadata() const noexcept { return m_metadata; }
    [[nodiscard]] const pdf_api::DocumentPermissions& GetPermissions() const noexcept { return m_permissions; }

    pdf_api::EngineResult<pdf_api::PdfSize> GetPageSize(int pageIndex) const;
    pdf_api::EngineResult<pdf_api::RenderedImage> RenderPage(int pageIndex, const pdf_api::RenderOptions& options) const;
    pdf_api::EngineResult<void> SaveCopy(const std::filesystem::path& targetPath) const;

    pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>> GetBookmarks() const;
    pdf_api::EngineResult<std::string> ExtractPageText(int pageIndex) const;
    pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>> SearchPageText(
        int pageIndex,
        const std::string& query,
        bool matchCase = false,
        bool matchWholeWord = false
    ) const;
    pdf_api::EngineResult<std::string> GetBoundedText(int pageIndex, const pdf_api::PdfRect& bounds) const;

    // Page-level Document Operations
    pdf_api::EngineResult<void> InsertBlankPage(int pageIndex, double width, double height);
    pdf_api::EngineResult<void> DeletePages(const std::vector<int>& pageIndices);
    pdf_api::EngineResult<void> RotatePages(const std::vector<int>& pageIndices, int rotationDeltaDegrees);
    pdf_api::EngineResult<void> CropPage(int pageIndex, const pdf_api::PdfRect& cropBox);
    pdf_api::EngineResult<void> ResizePage(int pageIndex, double width, double height);

    // Annotations
    pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>> GetAnnotations(int pageIndex) const;
    pdf_api::EngineResult<int> AddAnnotation(int pageIndex, const pdf_api::AnnotationData& annot);
    pdf_api::EngineResult<void> UpdateAnnotation(int pageIndex, int annotIndex, const pdf_api::AnnotationData& annot);
    pdf_api::EngineResult<void> DeleteAnnotation(int pageIndex, int annotIndex);

    // Forms and Interactive Fields
    [[nodiscard]] pdf_api::EngineResult<pdf_api::FormType> GetFormType() const;
    [[nodiscard]] bool HasForm() const;
    [[nodiscard]] bool HasXfa() const;
    pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> GetFormFields(int pageIndex) const;
    pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> GetAllFormFields() const;
    pdf_api::EngineResult<void> SetFormFieldValue(int pageIndex, int annotIndex, const std::string& value);
    pdf_api::EngineResult<void> SetFormFieldChecked(int pageIndex, int annotIndex, bool checked);
    pdf_api::EngineResult<void> SetFormFieldOption(int pageIndex, int annotIndex, int optionIndex);
    pdf_api::EngineResult<void> ResetFormFields();
    pdf_api::EngineResult<void> FlattenDocument(bool forPrint = false);

    // Page Content Editing
    pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>> GetPageObjects(int pageIndex) const;
    pdf_api::EngineResult<int> InsertPageText(
        int pageIndex,
        const std::string& text,
        double x,
        double y,
        const std::string& fontName = "Helvetica",
        double fontSize = 12.0,
        const pdf_api::PdfColor& color = {0, 0, 0, 255}
    );
    pdf_api::EngineResult<int> InsertPageImage(
        int pageIndex,
        const std::vector<uint8_t>& bgraPixels,
        int pixelWidth,
        int pixelHeight,
        double x,
        double y,
        double targetWidth,
        double targetHeight
    );
    pdf_api::EngineResult<int> InsertPageShape(
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const pdf_api::PdfColor& strokeColor,
        const pdf_api::PdfColor& fillColor,
        double strokeWidth
    );
    pdf_api::EngineResult<void> DeletePageObject(int pageIndex, int objectIndex);
    pdf_api::EngineResult<void> TransformPageObject(
        int pageIndex,
        int objectIndex,
        double dx,
        double dy,
        double scaleX = 1.0,
        double scaleY = 1.0
    );
    pdf_api::EngineResult<void> UpdatePageTextObject(
        int pageIndex,
        int objectIndex,
        const std::string& newText,
        double newFontSize,
        const pdf_api::PdfColor& newColor
    );
    pdf_api::EngineResult<void> ReorderPageObject(int pageIndex, int fromIndex, int toIndex);

    // Digital Signatures & Security
    pdf_api::EngineResult<int> GetSignatureCount() const;
    pdf_api::EngineResult<pdf_api::SignatureValidationResult> GetSignatureData(int index) const;
    pdf_api::EngineResult<void> ApplyVisualSignature(const pdf_api::VisualSignatureData& visData);

    // Secure Redaction & Document Sanitization
    pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis> AnalyzeRedactionTarget(
        int pageIndex,
        const pdf_api::PdfRect& rect
    );
    pdf_api::EngineResult<void> ApplyRedactions(const std::vector<pdf_api::RedactionMark>& marks);
    pdf_api::EngineResult<void> SanitizeDocument(const pdf_api::SanitizationOptions& options);
    pdf_api::EngineResult<int> OptimizeDocument(const pdf_api::OptimizationOptions& options);

    // Document Intelligence, Inspection & Standards Validation
    pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>> GetPageBoxes() const;
    pdf_api::EngineResult<std::vector<pdf_api::FontInfo>> GetDocumentFonts() const;
    pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>> GetDocumentImages() const;
    pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>> GetDocumentAttachments() const;
    pdf_api::EngineResult<std::vector<uint8_t>> ExtractAttachmentData(int attachmentIndex) const;
    pdf_api::EngineResult<void> AddAttachment(
        const std::string& name,
        const std::string& description,
        const std::vector<uint8_t>& data
    );
    pdf_api::EngineResult<bool> IsDocumentTagged() const;
    pdf_api::EngineResult<std::string> GetRawXmpMetadata() const;
    pdf_api::EngineResult<void> SetRawXmpMetadata(const std::string& xmpXml);

    void RefreshDocumentInfo();

private:
    void Release();

    pdf_api::DocumentId m_id{pdf_api::InvalidDocumentId};
    std::shared_ptr<pdf_api::IPdfEngine> m_engine;
    std::filesystem::path m_path;
    uint64_t m_fileSizeBytes{0};
    int m_pageCount{0};
    pdf_api::DocumentMetadata m_metadata;
    pdf_api::DocumentPermissions m_permissions;
};

} // namespace pdfinity::domain
