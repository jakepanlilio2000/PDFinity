// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "PdfTypes.hpp"
#include "security/SecureString.hpp"
#include <filesystem>
#include <span>

namespace pdfinity::pdf_api {

class IPdfEngine {
public:
    virtual ~IPdfEngine() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual EngineResult<DocumentId> OpenDocument(
        const std::filesystem::path& path,
        const security::SecureString& password = {}
    ) = 0;

    virtual EngineResult<DocumentId> OpenMemory(
        std::span<const uint8_t> buffer,
        const security::SecureString& password = {}
    ) = 0;

    virtual void CloseDocument(DocumentId id) = 0;

    virtual int GetPageCount(DocumentId id) = 0;
    virtual EngineResult<PdfSize> GetPageSize(DocumentId id, int pageIndex) = 0;
    virtual EngineResult<DocumentMetadata> GetMetadata(DocumentId id) = 0;
    virtual EngineResult<DocumentPermissions> GetPermissions(DocumentId id) = 0;

    virtual EngineResult<RenderedImage> RenderPage(
        DocumentId id,
        int pageIndex,
        const RenderOptions& options
    ) = 0;

    virtual EngineResult<std::vector<BookmarkItem>> GetBookmarks(DocumentId id) = 0;
    virtual EngineResult<std::string> ExtractPageText(DocumentId id, int pageIndex) = 0;
    virtual EngineResult<std::vector<TextSearchResult>> SearchPageText(
        DocumentId id,
        int pageIndex,
        const std::string& query,
        bool matchCase,
        bool matchWholeWord
    ) = 0;
    virtual EngineResult<std::string> GetBoundedText(
        DocumentId id,
        int pageIndex,
        const PdfRect& bounds
    ) = 0;

    virtual EngineResult<void> SaveCopy(
        DocumentId id,
        const std::filesystem::path& targetPath
    ) = 0;

    // Page-level Document Operations
    virtual EngineResult<DocumentId> CreateNewDocument() = 0;
    virtual EngineResult<void> ImportPages(
        DocumentId destId,
        DocumentId srcId,
        const std::vector<int>& pageIndices,
        int insertAtIndex
    ) = 0;
    virtual EngineResult<void> InsertBlankPage(
        DocumentId id,
        int pageIndex,
        double width,
        double height
    ) = 0;
    virtual EngineResult<void> DeletePages(
        DocumentId id,
        const std::vector<int>& pageIndices
    ) = 0;
    virtual EngineResult<void> RotatePages(
        DocumentId id,
        const std::vector<int>& pageIndices,
        int rotationDeltaDegrees
    ) = 0;
    virtual EngineResult<void> CropPage(
        DocumentId id,
        int pageIndex,
        const PdfRect& cropBox
    ) = 0;
    virtual EngineResult<void> ResizePage(
        DocumentId id,
        int pageIndex,
        double width,
        double height
    ) = 0;

    // Annotation Operations
    virtual EngineResult<std::vector<AnnotationData>> GetAnnotations(
        DocumentId id,
        int pageIndex
    ) = 0;
    virtual EngineResult<int> AddAnnotation(
        DocumentId id,
        int pageIndex,
        const AnnotationData& annot
    ) = 0;
    virtual EngineResult<void> UpdateAnnotation(
        DocumentId id,
        int pageIndex,
        int annotIndex,
        const AnnotationData& annot
    ) = 0;
    virtual EngineResult<void> DeleteAnnotation(
        DocumentId id,
        int pageIndex,
        int annotIndex
    ) = 0;

    // Form and Interactive Field Operations
    virtual EngineResult<FormType> GetFormType(DocumentId id) = 0;
    virtual EngineResult<std::vector<FormFieldData>> GetFormFields(DocumentId id, int pageIndex) = 0;
    virtual EngineResult<std::vector<FormFieldData>> GetAllFormFields(DocumentId id) = 0;
    virtual EngineResult<void> SetFormFieldValue(
        DocumentId id,
        int pageIndex,
        int annotIndex,
        const std::string& value
    ) = 0;
    virtual EngineResult<void> SetFormFieldChecked(
        DocumentId id,
        int pageIndex,
        int annotIndex,
        bool checked
    ) = 0;
    virtual EngineResult<void> SetFormFieldOption(
        DocumentId id,
        int pageIndex,
        int annotIndex,
        int optionIndex
    ) = 0;
    virtual EngineResult<void> ResetFormFields(DocumentId id) = 0;
    virtual EngineResult<void> FlattenDocument(DocumentId id, bool forPrint = false) = 0;

    // Page Content Object Operations
    virtual EngineResult<std::vector<PageObjectData>> GetPageObjects(DocumentId id, int pageIndex) = 0;
    virtual EngineResult<int> InsertPageText(
        DocumentId id,
        int pageIndex,
        const std::string& text,
        double x,
        double y,
        const std::string& fontName = "Helvetica",
        double fontSize = 12.0,
        const PdfColor& color = {0, 0, 0, 255}
    ) = 0;
    virtual EngineResult<int> InsertPageImage(
        DocumentId id,
        int pageIndex,
        const std::vector<uint8_t>& bgraPixels,
        int pixelWidth,
        int pixelHeight,
        double x,
        double y,
        double targetWidth,
        double targetHeight
    ) = 0;
    virtual EngineResult<int> InsertPageShape(
        DocumentId id,
        int pageIndex,
        const PdfRect& rect,
        const PdfColor& strokeColor,
        const PdfColor& fillColor,
        double strokeWidth
    ) = 0;
    virtual EngineResult<void> DeletePageObject(DocumentId id, int pageIndex, int objectIndex) = 0;
    virtual EngineResult<void> TransformPageObject(
        DocumentId id,
        int pageIndex,
        int objectIndex,
        double dx,
        double dy,
        double scaleX = 1.0,
        double scaleY = 1.0
    ) = 0;
    virtual EngineResult<void> UpdatePageTextObject(
        DocumentId id,
        int pageIndex,
        int objectIndex,
        const std::string& newText,
        double newFontSize,
        const PdfColor& newColor
    ) = 0;
    virtual EngineResult<void> ReorderPageObject(
        DocumentId id,
        int pageIndex,
        int fromIndex,
        int toIndex
    ) = 0;

    // Digital Signatures & Security
    virtual EngineResult<int> GetSignatureCount(DocumentId id) = 0;
    virtual EngineResult<SignatureValidationResult> GetSignatureData(DocumentId id, int index) = 0;
    virtual EngineResult<void> ApplyVisualSignature(DocumentId id, const VisualSignatureData& visData) = 0;

    // Secure Redaction & Document Sanitization
    virtual EngineResult<RedactionTargetAnalysis> AnalyzeRedactionTarget(
        DocumentId id,
        int pageIndex,
        const PdfRect& rect
    ) = 0;
    virtual EngineResult<void> ApplyRedactions(
        DocumentId id,
        const std::vector<RedactionMark>& marks
    ) = 0;
    virtual EngineResult<void> SanitizeDocument(
        DocumentId id,
        const SanitizationOptions& options
    ) = 0;

    // Optimization & Compression
    virtual EngineResult<int> OptimizeDocument(
        DocumentId id,
        const OptimizationOptions& options
    ) = 0;

    // Document Intelligence, Inspection & Standards Validation
    virtual EngineResult<std::vector<PageBoxInfo>> GetPageBoxes(DocumentId id) = 0;
    virtual EngineResult<std::vector<FontInfo>> GetDocumentFonts(DocumentId id) = 0;
    virtual EngineResult<std::vector<ImageInspectionInfo>> GetDocumentImages(DocumentId id) = 0;
    virtual EngineResult<std::vector<AttachmentInfo>> GetDocumentAttachments(DocumentId id) = 0;
    virtual EngineResult<std::vector<uint8_t>> ExtractAttachmentData(DocumentId id, int attachmentIndex) = 0;
    virtual EngineResult<void> AddAttachment(
        DocumentId id,
        const std::string& name,
        const std::string& description,
        const std::vector<uint8_t>& data
    ) = 0;
    virtual EngineResult<bool> IsDocumentTagged(DocumentId id) = 0;
    virtual EngineResult<std::string> GetRawXmpMetadata(DocumentId id) = 0;
    virtual EngineResult<void> SetRawXmpMetadata(DocumentId id, const std::string& xmpXml) = 0;

    virtual void SetLimits(size_t maxFileSizeBytes, int maxPageCount) = 0;
};

} // namespace pdfinity::pdf_api
