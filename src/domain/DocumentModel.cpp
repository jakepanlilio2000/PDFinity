// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "DocumentModel.hpp"

namespace pdfinity::domain {

DocumentModel::DocumentModel(pdf_api::DocumentId id, std::shared_ptr<pdf_api::IPdfEngine> engine, std::filesystem::path path)
    : m_id(id), m_engine(std::move(engine)), m_path(std::move(path)) {
    if (!m_path.empty()) {
        std::error_code ec;
        m_fileSizeBytes = std::filesystem::file_size(m_path, ec);
    }
    RefreshDocumentInfo();
}

DocumentModel::~DocumentModel() {
    Release();
}

DocumentModel::DocumentModel(DocumentModel&& other) noexcept
    : m_id(other.m_id),
      m_engine(std::move(other.m_engine)),
      m_path(std::move(other.m_path)),
      m_fileSizeBytes(other.m_fileSizeBytes),
      m_pageCount(other.m_pageCount),
      m_metadata(std::move(other.m_metadata)),
      m_permissions(std::move(other.m_permissions)) {
    other.m_id = pdf_api::InvalidDocumentId;
}

DocumentModel& DocumentModel::operator=(DocumentModel&& other) noexcept {
    if (this != &other) {
        Release();
        m_id = other.m_id;
        m_engine = std::move(other.m_engine);
        m_path = std::move(other.m_path);
        m_fileSizeBytes = other.m_fileSizeBytes;
        m_pageCount = other.m_pageCount;
        m_metadata = std::move(other.m_metadata);
        m_permissions = std::move(other.m_permissions);

        other.m_id = pdf_api::InvalidDocumentId;
    }
    return *this;
}

void DocumentModel::Release() {
    if (m_id != pdf_api::InvalidDocumentId && m_engine) {
        m_engine->CloseDocument(m_id);
        m_id = pdf_api::InvalidDocumentId;
    }
}

void DocumentModel::RefreshDocumentInfo() {
    if (!IsValid()) return;

    m_pageCount = m_engine->GetPageCount(m_id);

    auto metaRes = m_engine->GetMetadata(m_id);
    if (metaRes.has_value()) {
        m_metadata = metaRes.value;
    }

    auto permRes = m_engine->GetPermissions(m_id);
    if (permRes.has_value()) {
        m_permissions = permRes.value;
    }
}

pdf_api::EngineResult<pdf_api::PdfSize> DocumentModel::GetPageSize(int pageIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<pdf_api::PdfSize>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetPageSize(m_id, pageIndex);
}

pdf_api::EngineResult<pdf_api::RenderedImage> DocumentModel::RenderPage(int pageIndex, const pdf_api::RenderOptions& options) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<pdf_api::RenderedImage>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->RenderPage(m_id, pageIndex, options);
}

pdf_api::EngineResult<void> DocumentModel::SaveCopy(const std::filesystem::path& targetPath) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SaveCopy(m_id, targetPath);
}

pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>> DocumentModel::GetBookmarks() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetBookmarks(m_id);
}

pdf_api::EngineResult<std::string> DocumentModel::ExtractPageText(int pageIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->ExtractPageText(m_id, pageIndex);
}

pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>> DocumentModel::SearchPageText(
    int pageIndex,
    const std::string& query,
    bool matchCase,
    bool matchWholeWord
) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SearchPageText(m_id, pageIndex, query, matchCase, matchWholeWord);
}

pdf_api::EngineResult<std::string> DocumentModel::GetBoundedText(int pageIndex, const pdf_api::PdfRect& bounds) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetBoundedText(m_id, pageIndex, bounds);
}

pdf_api::EngineResult<void> DocumentModel::InsertBlankPage(int pageIndex, double width, double height) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->InsertBlankPage(m_id, pageIndex, width, height);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<void> DocumentModel::DeletePages(const std::vector<int>& pageIndices) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->DeletePages(m_id, pageIndices);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<void> DocumentModel::RotatePages(const std::vector<int>& pageIndices, int rotationDeltaDegrees) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->RotatePages(m_id, pageIndices, rotationDeltaDegrees);
}

pdf_api::EngineResult<void> DocumentModel::CropPage(int pageIndex, const pdf_api::PdfRect& cropBox) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->CropPage(m_id, pageIndex, cropBox);
}

pdf_api::EngineResult<void> DocumentModel::ResizePage(int pageIndex, double width, double height) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->ResizePage(m_id, pageIndex, width, height);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>> DocumentModel::GetAnnotations(int pageIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetAnnotations(m_id, pageIndex);
}

pdf_api::EngineResult<int> DocumentModel::AddAnnotation(int pageIndex, const pdf_api::AnnotationData& annot) {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->AddAnnotation(m_id, pageIndex, annot);
}

pdf_api::EngineResult<void> DocumentModel::UpdateAnnotation(int pageIndex, int annotIndex, const pdf_api::AnnotationData& annot) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->UpdateAnnotation(m_id, pageIndex, annotIndex, annot);
}

pdf_api::EngineResult<void> DocumentModel::DeleteAnnotation(int pageIndex, int annotIndex) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->DeleteAnnotation(m_id, pageIndex, annotIndex);
}

pdf_api::EngineResult<pdf_api::FormType> DocumentModel::GetFormType() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<pdf_api::FormType>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetFormType(m_id);
}

bool DocumentModel::HasForm() const {
    auto t = GetFormType();
    return t.has_value() && t.value != pdf_api::FormType::None;
}

bool DocumentModel::HasXfa() const {
    auto t = GetFormType();
    return t.has_value() && (t.value == pdf_api::FormType::XfaFull || t.value == pdf_api::FormType::XfaForeground);
}

pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> DocumentModel::GetFormFields(int pageIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetFormFields(m_id, pageIndex);
}

pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> DocumentModel::GetAllFormFields() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetAllFormFields(m_id);
}

pdf_api::EngineResult<void> DocumentModel::SetFormFieldValue(int pageIndex, int annotIndex, const std::string& value) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SetFormFieldValue(m_id, pageIndex, annotIndex, value);
}

pdf_api::EngineResult<void> DocumentModel::SetFormFieldChecked(int pageIndex, int annotIndex, bool checked) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SetFormFieldChecked(m_id, pageIndex, annotIndex, checked);
}

pdf_api::EngineResult<void> DocumentModel::SetFormFieldOption(int pageIndex, int annotIndex, int optionIndex) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SetFormFieldOption(m_id, pageIndex, annotIndex, optionIndex);
}

pdf_api::EngineResult<void> DocumentModel::ResetFormFields() {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->ResetFormFields(m_id);
}

pdf_api::EngineResult<void> DocumentModel::FlattenDocument(bool forPrint) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->FlattenDocument(m_id, forPrint);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>> DocumentModel::GetPageObjects(int pageIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetPageObjects(m_id, pageIndex);
}

pdf_api::EngineResult<int> DocumentModel::InsertPageText(
    int pageIndex,
    const std::string& text,
    double x,
    double y,
    const std::string& fontName,
    double fontSize,
    const pdf_api::PdfColor& color
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->InsertPageText(m_id, pageIndex, text, x, y, fontName, fontSize, color);
}

pdf_api::EngineResult<int> DocumentModel::InsertPageImage(
    int pageIndex,
    const std::vector<uint8_t>& bgraPixels,
    int pixelWidth,
    int pixelHeight,
    double x,
    double y,
    double targetWidth,
    double targetHeight
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->InsertPageImage(m_id, pageIndex, bgraPixels, pixelWidth, pixelHeight, x, y, targetWidth, targetHeight);
}

pdf_api::EngineResult<int> DocumentModel::InsertPageShape(
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const pdf_api::PdfColor& strokeColor,
    const pdf_api::PdfColor& fillColor,
    double strokeWidth
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->InsertPageShape(m_id, pageIndex, rect, strokeColor, fillColor, strokeWidth);
}

pdf_api::EngineResult<void> DocumentModel::DeletePageObject(int pageIndex, int objectIndex) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->DeletePageObject(m_id, pageIndex, objectIndex);
}

pdf_api::EngineResult<void> DocumentModel::TransformPageObject(
    int pageIndex,
    int objectIndex,
    double dx,
    double dy,
    double scaleX,
    double scaleY
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->TransformPageObject(m_id, pageIndex, objectIndex, dx, dy, scaleX, scaleY);
}

pdf_api::EngineResult<void> DocumentModel::UpdatePageTextObject(
    int pageIndex,
    int objectIndex,
    const std::string& newText,
    double newFontSize,
    const pdf_api::PdfColor& newColor
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->UpdatePageTextObject(m_id, pageIndex, objectIndex, newText, newFontSize, newColor);
}

pdf_api::EngineResult<void> DocumentModel::ReorderPageObject(int pageIndex, int fromIndex, int toIndex) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->ReorderPageObject(m_id, pageIndex, fromIndex, toIndex);
}

pdf_api::EngineResult<int> DocumentModel::GetSignatureCount() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetSignatureCount(m_id);
}

pdf_api::EngineResult<pdf_api::SignatureValidationResult> DocumentModel::GetSignatureData(int index) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetSignatureData(m_id, index);
}

pdf_api::EngineResult<void> DocumentModel::ApplyVisualSignature(const pdf_api::VisualSignatureData& visData) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->ApplyVisualSignature(m_id, visData);
}

pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis> DocumentModel::AnalyzeRedactionTarget(
    int pageIndex,
    const pdf_api::PdfRect& rect
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->AnalyzeRedactionTarget(m_id, pageIndex, rect);
}

pdf_api::EngineResult<void> DocumentModel::ApplyRedactions(const std::vector<pdf_api::RedactionMark>& marks) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->ApplyRedactions(m_id, marks);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<void> DocumentModel::SanitizeDocument(const pdf_api::SanitizationOptions& options) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->SanitizeDocument(m_id, options);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<int> DocumentModel::OptimizeDocument(const pdf_api::OptimizationOptions& options) {
    if (!IsValid()) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->OptimizeDocument(m_id, options);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>> DocumentModel::GetPageBoxes() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetPageBoxes(m_id);
}

pdf_api::EngineResult<std::vector<pdf_api::FontInfo>> DocumentModel::GetDocumentFonts() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::FontInfo>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetDocumentFonts(m_id);
}

pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>> DocumentModel::GetDocumentImages() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetDocumentImages(m_id);
}

pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>> DocumentModel::GetDocumentAttachments() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetDocumentAttachments(m_id);
}

pdf_api::EngineResult<std::vector<uint8_t>> DocumentModel::ExtractAttachmentData(int attachmentIndex) const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->ExtractAttachmentData(m_id, attachmentIndex);
}

pdf_api::EngineResult<void> DocumentModel::AddAttachment(
    const std::string& name,
    const std::string& description,
    const std::vector<uint8_t>& data
) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    auto res = m_engine->AddAttachment(m_id, name, description, data);
    if (res.has_value()) {
        RefreshDocumentInfo();
    }
    return res;
}

pdf_api::EngineResult<bool> DocumentModel::IsDocumentTagged() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<bool>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->IsDocumentTagged(m_id);
}

pdf_api::EngineResult<std::string> DocumentModel::GetRawXmpMetadata() const {
    if (!IsValid()) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->GetRawXmpMetadata(m_id);
}

pdf_api::EngineResult<void> DocumentModel::SetRawXmpMetadata(const std::string& xmpXml) {
    if (!IsValid()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document handle");
    }
    return m_engine->SetRawXmpMetadata(m_id, xmpXml);
}

} // namespace pdfinity::domain
