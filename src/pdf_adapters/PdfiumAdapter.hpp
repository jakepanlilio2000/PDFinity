// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "pdf_api/IPdfEngine.hpp"
#include <fpdfview.h>
#include <fpdf_formfill.h>
#include <mutex>
#include <unordered_map>
#include <atomic>

namespace pdfinity::pdf_adapters {

class PdfiumAdapter : public pdf_api::IPdfEngine {
public:
    PdfiumAdapter();
    ~PdfiumAdapter() override;

    bool Initialize() override;
    void Shutdown() override;

    pdf_api::EngineResult<pdf_api::DocumentId> OpenDocument(
        const std::filesystem::path& path,
        const security::SecureString& password = {}
    ) override;

    pdf_api::EngineResult<pdf_api::DocumentId> OpenMemory(
        std::span<const uint8_t> buffer,
        const security::SecureString& password = {}
    ) override;

    void CloseDocument(pdf_api::DocumentId id) override;

    int GetPageCount(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<pdf_api::PdfSize> GetPageSize(pdf_api::DocumentId id, int pageIndex) override;
    pdf_api::EngineResult<pdf_api::DocumentMetadata> GetMetadata(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<pdf_api::DocumentPermissions> GetPermissions(pdf_api::DocumentId id) override;

    pdf_api::EngineResult<pdf_api::RenderedImage> RenderPage(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::RenderOptions& options
    ) override;

    pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>> GetBookmarks(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::string> ExtractPageText(pdf_api::DocumentId id, int pageIndex) override;
    pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>> SearchPageText(
        pdf_api::DocumentId id,
        int pageIndex,
        const std::string& query,
        bool matchCase,
        bool matchWholeWord
    ) override;
    pdf_api::EngineResult<std::string> GetBoundedText(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::PdfRect& bounds
    ) override;

    pdf_api::EngineResult<void> SaveCopy(
        pdf_api::DocumentId id,
        const std::filesystem::path& targetPath
    ) override;

    // Page-level Document Operations
    pdf_api::EngineResult<pdf_api::DocumentId> CreateNewDocument() override;
    pdf_api::EngineResult<void> ImportPages(
        pdf_api::DocumentId destId,
        pdf_api::DocumentId srcId,
        const std::vector<int>& pageIndices,
        int insertAtIndex
    ) override;
    pdf_api::EngineResult<void> InsertBlankPage(
        pdf_api::DocumentId id,
        int pageIndex,
        double width,
        double height
    ) override;
    pdf_api::EngineResult<void> DeletePages(
        pdf_api::DocumentId id,
        const std::vector<int>& pageIndices
    ) override;
    pdf_api::EngineResult<void> RotatePages(
        pdf_api::DocumentId id,
        const std::vector<int>& pageIndices,
        int rotationDeltaDegrees
    ) override;
    pdf_api::EngineResult<void> CropPage(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::PdfRect& cropBox
    ) override;
    pdf_api::EngineResult<void> ResizePage(
        pdf_api::DocumentId id,
        int pageIndex,
        double width,
        double height
    ) override;

    // Annotation Operations
    pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>> GetAnnotations(
        pdf_api::DocumentId id,
        int pageIndex
    ) override;
    pdf_api::EngineResult<int> AddAnnotation(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::AnnotationData& annot
    ) override;
    pdf_api::EngineResult<void> UpdateAnnotation(
        pdf_api::DocumentId id,
        int pageIndex,
        int annotIndex,
        const pdf_api::AnnotationData& annot
    ) override;
    pdf_api::EngineResult<void> DeleteAnnotation(
        pdf_api::DocumentId id,
        int pageIndex,
        int annotIndex
    ) override;

    // Form and Interactive Field Operations
    pdf_api::EngineResult<pdf_api::FormType> GetFormType(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> GetFormFields(
        pdf_api::DocumentId id,
        int pageIndex
    ) override;
    pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> GetAllFormFields(
        pdf_api::DocumentId id
    ) override;
    pdf_api::EngineResult<void> SetFormFieldValue(
        pdf_api::DocumentId id,
        int pageIndex,
        int annotIndex,
        const std::string& value
    ) override;
    pdf_api::EngineResult<void> SetFormFieldChecked(
        pdf_api::DocumentId id,
        int pageIndex,
        int annotIndex,
        bool checked
    ) override;
    pdf_api::EngineResult<void> SetFormFieldOption(
        pdf_api::DocumentId id,
        int pageIndex,
        int annotIndex,
        int optionIndex
    ) override;
    pdf_api::EngineResult<void> ResetFormFields(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<void> FlattenDocument(pdf_api::DocumentId id, bool forPrint = false) override;

    // Page Content Object Operations
    pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>> GetPageObjects(
        pdf_api::DocumentId id,
        int pageIndex
    ) override;
    pdf_api::EngineResult<int> InsertPageText(
        pdf_api::DocumentId id,
        int pageIndex,
        const std::string& text,
        double x,
        double y,
        const std::string& fontName = "Helvetica",
        double fontSize = 12.0,
        const pdf_api::PdfColor& color = {0, 0, 0, 255}
    ) override;
    pdf_api::EngineResult<int> InsertPageImage(
        pdf_api::DocumentId id,
        int pageIndex,
        const std::vector<uint8_t>& bgraPixels,
        int pixelWidth,
        int pixelHeight,
        double x,
        double y,
        double targetWidth,
        double targetHeight
    ) override;
    pdf_api::EngineResult<int> InsertPageShape(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::PdfRect& rect,
        const pdf_api::PdfColor& strokeColor,
        const pdf_api::PdfColor& fillColor,
        double strokeWidth
    ) override;
    pdf_api::EngineResult<void> DeletePageObject(
        pdf_api::DocumentId id,
        int pageIndex,
        int objectIndex
    ) override;
    pdf_api::EngineResult<void> TransformPageObject(
        pdf_api::DocumentId id,
        int pageIndex,
        int objectIndex,
        double dx,
        double dy,
        double scaleX = 1.0,
        double scaleY = 1.0
    ) override;
    pdf_api::EngineResult<void> UpdatePageTextObject(
        pdf_api::DocumentId id,
        int pageIndex,
        int objectIndex,
        const std::string& newText,
        double newFontSize,
        const pdf_api::PdfColor& newColor
    ) override;
    pdf_api::EngineResult<void> ReorderPageObject(
        pdf_api::DocumentId id,
        int pageIndex,
        int fromIndex,
        int toIndex
    ) override;

    // Digital Signatures & Security
    pdf_api::EngineResult<int> GetSignatureCount(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<pdf_api::SignatureValidationResult> GetSignatureData(
        pdf_api::DocumentId id,
        int index
    ) override;
    pdf_api::EngineResult<void> ApplyVisualSignature(
        pdf_api::DocumentId id,
        const pdf_api::VisualSignatureData& visData
    ) override;

    // Secure Redaction & Document Sanitization
    pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis> AnalyzeRedactionTarget(
        pdf_api::DocumentId id,
        int pageIndex,
        const pdf_api::PdfRect& rect
    ) override;
    pdf_api::EngineResult<void> ApplyRedactions(
        pdf_api::DocumentId id,
        const std::vector<pdf_api::RedactionMark>& marks
    ) override;
    pdf_api::EngineResult<void> SanitizeDocument(
        pdf_api::DocumentId id,
        const pdf_api::SanitizationOptions& options
    ) override;
    pdf_api::EngineResult<int> OptimizeDocument(
        pdf_api::DocumentId id,
        const pdf_api::OptimizationOptions& options
    ) override;

    // Document Intelligence, Inspection & Standards Validation
    pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>> GetPageBoxes(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::vector<pdf_api::FontInfo>> GetDocumentFonts(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>> GetDocumentImages(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>> GetDocumentAttachments(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::vector<uint8_t>> ExtractAttachmentData(pdf_api::DocumentId id, int attachmentIndex) override;
    pdf_api::EngineResult<void> AddAttachment(
        pdf_api::DocumentId id,
        const std::string& name,
        const std::string& description,
        const std::vector<uint8_t>& data
    ) override;
    pdf_api::EngineResult<bool> IsDocumentTagged(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<std::string> GetRawXmpMetadata(pdf_api::DocumentId id) override;
    pdf_api::EngineResult<void> SetRawXmpMetadata(pdf_api::DocumentId id, const std::string& xmpXml) override;

    void SetLimits(size_t maxFileSizeBytes, int maxPageCount) override;

private:
    struct DocumentRecord {
        FPDF_DOCUMENT doc{nullptr};
        FPDF_FORMHANDLE formHandle{nullptr};
        FPDF_FORMFILLINFO formInfo{};
        std::vector<uint8_t> memoryBuffer; // Keeps memory alive for OpenMemory
        std::filesystem::path sourcePath;
        bool isEncrypted{false};
    };

    FPDF_DOCUMENT GetNativeDoc(pdf_api::DocumentId id);
    FPDF_FORMHANDLE GetNativeFormHandle(pdf_api::DocumentId id);
    void EnsureFormFillEnv(DocumentRecord& record);
    std::string ExtractMetaText(FPDF_DOCUMENT doc, const char* tag);
    void TraverseBookmarks(FPDF_DOCUMENT doc, void* parentBookmark, std::vector<pdf_api::BookmarkItem>& list);

    std::mutex m_mutex;
    std::unordered_map<pdf_api::DocumentId, DocumentRecord> m_documents;
    std::atomic<pdf_api::DocumentId> m_nextId{1};
    bool m_initialized{false};

    size_t m_maxFileSizeBytes{500 * 1024 * 1024}; // Default 500 MB limit
    int m_maxPageCount{10000};                     // Default 10,000 page limit
};

} // namespace pdfinity::pdf_adapters
