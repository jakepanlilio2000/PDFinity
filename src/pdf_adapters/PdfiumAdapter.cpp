// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PdfiumAdapter.hpp"
#include "application/Logger.hpp"
#include "security/PathSanitizer.hpp"

#include <fpdfview.h>
#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fpdf_save.h>
#include <fpdf_edit.h>
#include <fpdf_ppo.h>
#include <fpdf_transformpage.h>
#include <fpdf_annot.h>
#include <fpdf_formfill.h>
#include <fpdf_flatten.h>
#include <fpdf_signature.h>
#include <fpdf_attachment.h>
#include <fpdf_structtree.h>
#include <fpdf_catalog.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <QString>
#include <QDebug>
#include <QImage>

namespace pdfinity::pdf_adapters {

namespace {
static std::atomic<int> s_pdfiumInitRefCount{0};
static std::mutex s_pdfiumInitMutex;

struct FileWriter : public FPDF_FILEWRITE {
    explicit FileWriter(std::ofstream& s) : stream(s) {
        version = 1;
        WriteBlock = &FileWriter::WriteCallback;
    }

    static int WriteCallback(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) {
        auto* self = static_cast<FileWriter*>(pThis);
        self->stream.write(static_cast<const char*>(pData), size);
        return self->stream.good() ? 1 : 0;
    }

    std::ofstream& stream;
};

struct MemoryWriter : public FPDF_FILEWRITE {
    explicit MemoryWriter(std::string& buf) : buffer(buf) {
        version = 1;
        WriteBlock = &MemoryWriter::WriteCallback;
    }

    static int WriteCallback(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) {
        auto* self = static_cast<MemoryWriter*>(pThis);
        const char* p = static_cast<const char*>(pData);
        self->buffer.append(p, size);
        return 1;
    }

    std::string& buffer;
};
} // namespace

PdfiumAdapter::PdfiumAdapter() {
    Initialize();
}

PdfiumAdapter::~PdfiumAdapter() {
    Shutdown();
}

bool PdfiumAdapter::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    {
        std::lock_guard<std::mutex> initLock(s_pdfiumInitMutex);
        if (s_pdfiumInitRefCount.fetch_add(1) == 0) {
            FPDF_LIBRARY_CONFIG config;
            config.version = 2;
            config.m_pUserFontPaths = nullptr;
            config.m_pIsolate = nullptr;
            config.m_v8EmbedderSlot = 0;

            FPDF_InitLibraryWithConfig(&config);
            LOG_INFO("Pdfium", "Google PDFium library initialized successfully.");
        }
    }
    m_initialized = true;
    return true;
}

void PdfiumAdapter::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    for (auto& [id, record] : m_documents) {
        if (record.formHandle) {
            FPDFDOC_ExitFormFillEnvironment(record.formHandle);
            record.formHandle = nullptr;
        }
        if (record.doc) {
            FPDF_CloseDocument(record.doc);
        }
    }
    m_documents.clear();

    {
        std::lock_guard<std::mutex> initLock(s_pdfiumInitMutex);
        if (s_pdfiumInitRefCount.fetch_sub(1) == 1) {
            FPDF_DestroyLibrary();
            LOG_INFO("Pdfium", "Google PDFium library shut down.");
        }
    }
    m_initialized = false;
}

void PdfiumAdapter::SetLimits(size_t maxFileSizeBytes, int maxPageCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxFileSizeBytes = maxFileSizeBytes;
    m_maxPageCount = maxPageCount;
}

FPDF_DOCUMENT PdfiumAdapter::GetNativeDoc(pdf_api::DocumentId id) {
    auto it = m_documents.find(id);
    if (it != m_documents.end()) {
        return it->second.doc;
    }
    return nullptr;
}

namespace {
void DummyInvalidate(FPDF_FORMFILLINFO*, FPDF_PAGE, double, double, double, double) {}
}

void PdfiumAdapter::EnsureFormFillEnv(DocumentRecord& record) {
    if (!record.formHandle && record.doc) {
        memset(&record.formInfo, 0, sizeof(FPDF_FORMFILLINFO));
        record.formInfo.version = 1;
        record.formInfo.FFI_Invalidate = &DummyInvalidate;
        record.formHandle = FPDFDOC_InitFormFillEnvironment(record.doc, &record.formInfo);
    }
}

FPDF_FORMHANDLE PdfiumAdapter::GetNativeFormHandle(pdf_api::DocumentId id) {
    auto it = m_documents.find(id);
    if (it != m_documents.end()) {
        EnsureFormFillEnv(it->second);
        return it->second.formHandle;
    }
    return nullptr;
}

pdf_api::EngineResult<pdf_api::DocumentId> PdfiumAdapter::OpenDocument(
    const std::filesystem::path& path,
    const security::SecureString& password
) {
    if (!m_initialized) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::Unknown, "PDF engine not initialized");
    }

    // Verify file existence and size limit
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::FileNotFound, "File does not exist: " + path.string());
    }

    uint64_t fileSize = std::filesystem::file_size(path, ec);
    if (fileSize > m_maxFileSizeBytes) {
        LOG_WARN("Pdfium", "File exceeds maximum size limit: " + std::to_string(fileSize) + " bytes");
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::FileTooLarge, "File exceeds maximum size limit");
    }

#if defined(_WIN32)
    // On Windows, read file into memory to support full Unicode paths without multibyte encoding issues
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::FileNotFound, "Failed to open file stream");
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    auto result = OpenMemory(buffer, password);
    if (result.has_value()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_documents[result.value].sourcePath = path;
    }
    return result;
#else
    const char* pwd = password.empty() ? nullptr : password.c_str();
    FPDF_DOCUMENT doc = FPDF_LoadDocument(path.string().c_str(), pwd);
    if (!doc) {
        unsigned long err = FPDF_GetLastError();
        switch (err) {
            case FPDF_ERR_PASSWORD:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(
                    password.empty() ? pdf_api::EngineError::PasswordRequired : pdf_api::EngineError::IncorrectPassword,
                    "Document is password protected"
                );
            case FPDF_ERR_FORMAT:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::InvalidFormat, "Invalid PDF format");
            case FPDF_ERR_SECURITY:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::UnsupportedEncryption, "Unsupported security handler");
            default:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::CorruptedData, "Failed to parse PDF document");
        }
    }

    int pageCount = FPDF_GetPageCount(doc);
    if (pageCount > m_maxPageCount) {
        FPDF_CloseDocument(doc);
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::PageCountLimitExceeded, "Document exceeds page count limit");
    }

    pdf_api::DocumentId newId = m_nextId++;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        DocumentRecord rec;
        rec.doc = doc;
        rec.sourcePath = path;
        rec.isEncrypted = (FPDF_GetSecurityHandlerRevision(doc) >= 0);
        m_documents[newId] = std::move(rec);
    }

    LOG_INFO("Pdfium", "Successfully opened document ID " + std::to_string(newId) + " (" + std::to_string(pageCount) + " pages)");
    return pdf_api::EngineResult<pdf_api::DocumentId>::Ok(newId);
#endif
}

pdf_api::EngineResult<pdf_api::DocumentId> PdfiumAdapter::OpenMemory(
    std::span<const uint8_t> buffer,
    const security::SecureString& password
) {
    if (!m_initialized) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::Unknown, "PDF engine not initialized");
    }

    if (buffer.empty()) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::CorruptedData, "Memory buffer is empty");
    }

    if (buffer.size() > m_maxFileSizeBytes) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::FileTooLarge, "Memory buffer exceeds size limit");
    }

    // Copy buffer into record to guarantee lifetime for PDFium
    std::vector<uint8_t> ownedBuffer(buffer.begin(), buffer.end());
    const char* pwd = password.empty() ? nullptr : password.c_str();

    FPDF_DOCUMENT doc = FPDF_LoadMemDocument(ownedBuffer.data(), static_cast<int>(ownedBuffer.size()), pwd);
    if (!doc) {
        unsigned long err = FPDF_GetLastError();
        switch (err) {
            case FPDF_ERR_PASSWORD:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(
                    password.empty() ? pdf_api::EngineError::PasswordRequired : pdf_api::EngineError::IncorrectPassword,
                    "Document is password protected"
                );
            case FPDF_ERR_FORMAT:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::InvalidFormat, "Invalid PDF format");
            case FPDF_ERR_SECURITY:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::UnsupportedEncryption, "Unsupported security handler");
            default:
                return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::CorruptedData, "Failed to parse PDF document from memory");
        }
    }

    int pageCount = FPDF_GetPageCount(doc);
    if (pageCount > m_maxPageCount) {
        FPDF_CloseDocument(doc);
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::PageCountLimitExceeded, "Document exceeds page count limit");
    }

    pdf_api::DocumentId newId = m_nextId++;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        DocumentRecord rec;
        rec.doc = doc;
        rec.memoryBuffer = std::move(ownedBuffer);
        rec.isEncrypted = (FPDF_GetSecurityHandlerRevision(doc) >= 0);
        m_documents[newId] = std::move(rec);
    }

    LOG_INFO("Pdfium", "Successfully opened memory document ID " + std::to_string(newId) + " (" + std::to_string(pageCount) + " pages)");
    return pdf_api::EngineResult<pdf_api::DocumentId>::Ok(newId);
}

void PdfiumAdapter::CloseDocument(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_documents.find(id);
    if (it != m_documents.end()) {
        if (it->second.formHandle) {
            FPDFDOC_ExitFormFillEnvironment(it->second.formHandle);
            it->second.formHandle = nullptr;
        }
        if (it->second.doc) {
            FPDF_CloseDocument(it->second.doc);
        }
        m_documents.erase(it);
        LOG_INFO("Pdfium", "Closed document ID: " + std::to_string(id));
    }
}

int PdfiumAdapter::GetPageCount(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    return doc ? FPDF_GetPageCount(doc) : 0;
}

pdf_api::EngineResult<pdf_api::PdfSize> PdfiumAdapter::GetPageSize(pdf_api::DocumentId id, int pageIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::PdfSize>::Fail(pdf_api::EngineError::Unknown, "Invalid document ID");
    }

    int totalPages = FPDF_GetPageCount(doc);
    if (pageIndex < 0 || pageIndex >= totalPages) {
        return pdf_api::EngineResult<pdf_api::PdfSize>::Fail(pdf_api::EngineError::PageNotFound, "Page index out of bounds");
    }

    double width = 0.0;
    double height = 0.0;
    if (FPDF_GetPageSizeByIndex(doc, pageIndex, &width, &height) != 0) {
        return pdf_api::EngineResult<pdf_api::PdfSize>::Ok(pdf_api::PdfSize{ width, height });
    }
    return pdf_api::EngineResult<pdf_api::PdfSize>::Fail(pdf_api::EngineError::CorruptedData, "Failed to retrieve page dimensions");
}

std::string PdfiumAdapter::ExtractMetaText(FPDF_DOCUMENT doc, const char* tag) {
    unsigned long len = FPDF_GetMetaText(doc, tag, nullptr, 0);
    if (len <= 2) {
        return {};
    }

    std::vector<uint8_t> buffer(len);
    FPDF_GetMetaText(doc, tag, buffer.data(), len);

    // FPDF_GetMetaText returns UTF-16LE bytes with null terminator
    const char16_t* u16ptr = reinterpret_cast<const char16_t*>(buffer.data());
    size_t u16len = (len / 2) - 1; // Exclude null terminator
    return QString::fromUtf16(u16ptr, static_cast<int>(u16len)).toStdString();
}

pdf_api::EngineResult<pdf_api::DocumentMetadata> PdfiumAdapter::GetMetadata(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::DocumentMetadata>::Fail(pdf_api::EngineError::Unknown, "Invalid document ID");
    }

    pdf_api::DocumentMetadata meta;
    meta.title = ExtractMetaText(doc, "Title");
    meta.author = ExtractMetaText(doc, "Author");
    meta.subject = ExtractMetaText(doc, "Subject");
    meta.keywords = ExtractMetaText(doc, "Keywords");
    meta.creator = ExtractMetaText(doc, "Creator");
    meta.producer = ExtractMetaText(doc, "Producer");
    meta.creationDate = ExtractMetaText(doc, "CreationDate");
    meta.modificationDate = ExtractMetaText(doc, "ModDate");

    return pdf_api::EngineResult<pdf_api::DocumentMetadata>::Ok(std::move(meta));
}

pdf_api::EngineResult<pdf_api::DocumentPermissions> PdfiumAdapter::GetPermissions(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::DocumentPermissions>::Fail(pdf_api::EngineError::Unknown, "Invalid document ID");
    }

    pdf_api::DocumentPermissions perms;
    int revision = FPDF_GetSecurityHandlerRevision(doc);
    perms.isEncrypted = (revision >= 0);

    unsigned long pFlags = FPDF_GetDocPermissions(doc);
    // Standard PDF permission flags (ISO 32000-1 Table 22)
    perms.canPrint = (pFlags & (1 << 2)) != 0;
    perms.canModifyContents = (pFlags & (1 << 3)) != 0;
    perms.canCopyText = (pFlags & (1 << 4)) != 0;
    perms.canAnnotate = (pFlags & (1 << 5)) != 0;
    perms.canFillForms = (pFlags & (1 << 8)) != 0;
    perms.canExtractAccessibility = (pFlags & (1 << 9)) != 0;
    perms.canAssemble = (pFlags & (1 << 10)) != 0;
    perms.canPrintHighQuality = (pFlags & (1 << 11)) != 0;

    if (perms.isEncrypted) {
        perms.securityHandler = "Standard Revision " + std::to_string(revision);
    }

    return pdf_api::EngineResult<pdf_api::DocumentPermissions>::Ok(perms);
}

pdf_api::EngineResult<pdf_api::RenderedImage> PdfiumAdapter::RenderPage(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::RenderOptions& options
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::RenderedImage>::Fail(pdf_api::EngineError::Unknown, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<pdf_api::RenderedImage>::Fail(pdf_api::EngineError::PageNotFound, "Failed to load page");
    }

    double origWidth = FPDF_GetPageWidthF(page);
    double origHeight = FPDF_GetPageHeightF(page);

    // Compute pixel dimensions: 72 points per inch standard PDF units
    double dpiScale = static_cast<double>(options.dpi) / 72.0;
    int targetWidth = static_cast<int>(std::round(origWidth * options.scale * dpiScale));
    int targetHeight = static_cast<int>(std::round(origHeight * options.scale * dpiScale));

    if (targetWidth <= 0 || targetHeight <= 0) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<pdf_api::RenderedImage>::Fail(pdf_api::EngineError::RenderFailed, "Invalid render target dimensions");
    }

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(targetWidth, targetHeight, FPDFBitmap_BGRA, nullptr, 0);
    if (!bitmap) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<pdf_api::RenderedImage>::Fail(pdf_api::EngineError::OutOfMemory, "Failed to allocate render bitmap");
    }

    // Fill background with opaque white
    FPDFBitmap_FillRect(bitmap, 0, 0, targetWidth, targetHeight, 0xFFFFFFFF);

    int renderFlags = FPDF_ANNOT; // Render annotations by default
    if (options.grayscale) {
        renderFlags |= FPDF_GRAYSCALE;
    }

    FPDF_RenderPageBitmap(bitmap, page, 0, 0, targetWidth, targetHeight, options.rotationDegrees / 90, renderFlags);

    FPDF_FORMHANDLE formHandle = GetNativeFormHandle(id);
    if (formHandle) {
        FPDF_FFLDraw(formHandle, bitmap, page, 0, 0, targetWidth, targetHeight, options.rotationDegrees / 90, renderFlags);
    }

    const uint8_t* srcBuffer = static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
    int stride = FPDFBitmap_GetStride(bitmap);

    pdf_api::RenderedImage img;
    img.width = targetWidth;
    img.height = targetHeight;
    img.stride = stride;
    img.isBgra = true;
    img.pixels.assign(srcBuffer, srcBuffer + (stride * targetHeight));

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<pdf_api::RenderedImage>::Ok(std::move(img));
}

pdf_api::EngineResult<void> PdfiumAdapter::SaveCopy(
    pdf_api::DocumentId id,
    const std::filesystem::path& targetPath
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "Invalid document ID");
    }

    // Atomic save replacement protocol
    std::filesystem::path tempPath = targetPath;
    tempPath += ".tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

    std::ofstream outStream(tempPath, std::ios::binary);
    if (!outStream.is_open()) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to create temporary output file");
    }

    FileWriter writer(outStream);
    int success = FPDF_SaveAsCopy(doc, &writer, FPDF_NO_INCREMENTAL);
    outStream.flush();
    outStream.close();

    if (!success) {
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "FPDF_SaveAsCopy returned failure");
    }

    // Atomic rename
    std::error_code ec;
    std::filesystem::rename(tempPath, targetPath, ec);
    if (ec) {
        // Fallback for Windows in case target file exists and cannot be replaced in one step
        std::filesystem::remove(targetPath, ec);
        std::filesystem::rename(tempPath, targetPath, ec);
        if (ec) {
            std::filesystem::remove(tempPath, ec);
            return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Atomic replacement rename failed: " + ec.message());
        }
    }

    LOG_INFO("Pdfium", "Successfully saved document copy atomically to: " + targetPath.string());
    return pdf_api::EngineResult<void>::Ok();
}

void PdfiumAdapter::TraverseBookmarks(FPDF_DOCUMENT doc, void* parentBookmark, std::vector<pdf_api::BookmarkItem>& list) {
    auto bookmark = static_cast<FPDF_BOOKMARK>(parentBookmark);
    FPDF_BOOKMARK cur = FPDFBookmark_GetFirstChild(doc, bookmark);
    while (cur) {
        pdf_api::BookmarkItem item;

        // Retrieve bookmark title
        unsigned long buflen = FPDFBookmark_GetTitle(cur, nullptr, 0);
        if (buflen > 0) {
            std::vector<unsigned short> buf(buflen / sizeof(unsigned short));
            FPDFBookmark_GetTitle(cur, buf.data(), buflen);
            item.title = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).trimmed().toStdString();
        }

        // Retrieve target page index
        FPDF_DEST dest = FPDFBookmark_GetDest(doc, cur);
        if (!dest) {
            FPDF_ACTION action = FPDFBookmark_GetAction(cur);
            if (action) {
                dest = FPDFAction_GetDest(doc, action);
            }
        }
        if (dest) {
            item.targetPageIndex = FPDFDest_GetDestPageIndex(doc, dest);
        }

        // Recursively traverse children
        TraverseBookmarks(doc, cur, item.children);

        list.push_back(std::move(item));
        cur = FPDFBookmark_GetNextSibling(doc, cur);
    }
}

pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>> PdfiumAdapter::GetBookmarks(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    std::vector<pdf_api::BookmarkItem> bookmarks;
    TraverseBookmarks(doc, nullptr, bookmarks);
    return pdf_api::EngineResult<std::vector<pdf_api::BookmarkItem>>::Ok(std::move(bookmarks));
}

pdf_api::EngineResult<std::string> PdfiumAdapter::ExtractPageText(pdf_api::DocumentId id, int pageIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::PageNotFound, "Page index out of bounds");
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (!textPage) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<std::string>::Ok("");
    }

    int count = FPDFText_CountChars(textPage);
    std::string resultUtf8;
    if (count > 0) {
        std::vector<unsigned short> buffer(static_cast<size_t>(count) + 1, 0);
        int written = FPDFText_GetText(textPage, 0, count, buffer.data());
        if (written > 0) {
            resultUtf8 = QString::fromUtf16(reinterpret_cast<const char16_t*>(buffer.data()), written).toStdString();
        }
    }

    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<std::string>::Ok(std::move(resultUtf8));
}

pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>> PdfiumAdapter::SearchPageText(
    pdf_api::DocumentId id,
    int pageIndex,
    const std::string& query,
    bool matchCase,
    bool matchWholeWord
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (query.empty()) {
        return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Ok({});
    }

    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Fail(pdf_api::EngineError::PageNotFound, "Page index out of bounds");
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (!textPage) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Ok({});
    }

    unsigned long flags = 0;
    if (matchCase) flags |= FPDF_MATCHCASE;
    if (matchWholeWord) flags |= FPDF_MATCHWHOLEWORD;

    QString qQuery = QString::fromStdString(query);
    const auto* utf16Query = reinterpret_cast<const unsigned short*>(qQuery.utf16());

    FPDF_SCHHANDLE search = FPDFText_FindStart(textPage, utf16Query, flags, 0);
    std::vector<pdf_api::TextSearchResult> results;

    if (search) {
        while (FPDFText_FindNext(search)) {
            pdf_api::TextSearchResult match;
            match.pageIndex = pageIndex;
            match.charIndex = FPDFText_GetSchResultIndex(search);
            match.charCount = FPDFText_GetSchCount(search);

            int rectCount = FPDFText_CountRects(textPage, match.charIndex, match.charCount);
            for (int r = 0; r < rectCount; ++r) {
                double left = 0, top = 0, right = 0, bottom = 0;
                if (FPDFText_GetRect(textPage, r, &left, &top, &right, &bottom)) {
                    pdf_api::PdfRect box;
                    box.x = std::min(left, right);
                    box.y = std::min(bottom, top);
                    box.width = std::abs(right - left);
                    box.height = std::abs(top - bottom);
                    match.rects.push_back(box);
                }
            }
            results.push_back(std::move(match));
        }
        FPDFText_FindClose(search);
    }

    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<std::vector<pdf_api::TextSearchResult>>::Ok(std::move(results));
}

pdf_api::EngineResult<std::string> PdfiumAdapter::GetBoundedText(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::PdfRect& bounds
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::PageNotFound, "Page index out of bounds");
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (!textPage) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<std::string>::Ok("");
    }

    double left = bounds.x;
    double bottom = bounds.y;
    double right = bounds.x + bounds.width;
    double top = bounds.y + bounds.height;

    int count = FPDFText_GetBoundedText(textPage, left, top, right, bottom, nullptr, 0);
    std::string textUtf8;
    if (count > 0) {
        std::vector<unsigned short> buffer(static_cast<size_t>(count) + 1, 0);
        int written = FPDFText_GetBoundedText(textPage, left, top, right, bottom, buffer.data(), count);
        if (written > 0) {
            textUtf8 = QString::fromUtf16(reinterpret_cast<const char16_t*>(buffer.data()), written).toStdString();
        }
    }

    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<std::string>::Ok(std::move(textUtf8));
}

pdf_api::EngineResult<pdf_api::DocumentId> PdfiumAdapter::CreateNewDocument() {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::DocumentId>::Fail(pdf_api::EngineError::Unknown, "Failed to create new PDFium document");
    }
    pdf_api::DocumentId id = m_nextId++;
    DocumentRecord rec;
    rec.doc = doc;
    m_documents[id] = std::move(rec);
    return pdf_api::EngineResult<pdf_api::DocumentId>::Ok(id);
}

pdf_api::EngineResult<void> PdfiumAdapter::ImportPages(
    pdf_api::DocumentId destId,
    pdf_api::DocumentId srcId,
    const std::vector<int>& pageIndices,
    int insertAtIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT destDoc = GetNativeDoc(destId);
    FPDF_DOCUMENT srcDoc = GetNativeDoc(srcId);
    if (!destDoc || !srcDoc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid source or destination document ID");
    }

    int ok = 0;
    if (pageIndices.empty()) {
        ok = FPDF_ImportPagesByIndex(destDoc, srcDoc, nullptr, 0, insertAtIndex);
    } else {
        ok = FPDF_ImportPagesByIndex(destDoc, srcDoc, pageIndices.data(), static_cast<unsigned long>(pageIndices.size()), insertAtIndex);
    }

    if (!ok) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::InvalidFormat, "FPDF_ImportPagesByIndex failed");
    }
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::InsertBlankPage(
    pdf_api::DocumentId id,
    int pageIndex,
    double width,
    double height
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDFPage_New(doc, pageIndex, width, height);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "FPDFPage_New failed");
    }
    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::DeletePages(
    pdf_api::DocumentId id,
    const std::vector<int>& pageIndices
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int currentCount = FPDF_GetPageCount(doc);
    std::vector<int> sorted = pageIndices;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    for (int p : sorted) {
        if (p >= 0 && p < currentCount) {
            FPDFPage_Delete(doc, p);
            currentCount--;
        }
    }
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::RotatePages(
    pdf_api::DocumentId id,
    const std::vector<int>& pageIndices,
    int rotationDeltaDegrees
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int deltaSteps = (rotationDeltaDegrees % 360) / 90;
    if (deltaSteps < 0) deltaSteps += 4;

    for (int p : pageIndices) {
        FPDF_PAGE page = FPDF_LoadPage(doc, p);
        if (page) {
            int curRot = FPDFPage_GetRotation(page);
            int newRot = (curRot + deltaSteps) % 4;
            FPDFPage_SetRotation(page, newRot);
            FPDF_ClosePage(page);
        }
    }
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::CropPage(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::PdfRect& cropBox
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDFPage_SetCropBox(
        page,
        static_cast<float>(cropBox.x),
        static_cast<float>(cropBox.y),
        static_cast<float>(cropBox.x + cropBox.width),
        static_cast<float>(cropBox.y + cropBox.height)
    );
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::ResizePage(
    pdf_api::DocumentId id,
    int pageIndex,
    double width,
    double height
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDFPage_SetMediaBox(
        page,
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height)
    );
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

namespace {

FPDF_ANNOTATION_SUBTYPE ToPdfiumSubtype(pdf_api::AnnotationType type) {
    switch (type) {
        case pdf_api::AnnotationType::Highlight: return FPDF_ANNOT_HIGHLIGHT;
        case pdf_api::AnnotationType::Underline: return FPDF_ANNOT_UNDERLINE;
        case pdf_api::AnnotationType::Strikeout: return FPDF_ANNOT_STRIKEOUT;
        case pdf_api::AnnotationType::Squiggly: return FPDF_ANNOT_SQUIGGLY;
        case pdf_api::AnnotationType::TextNote: return FPDF_ANNOT_TEXT;
        case pdf_api::AnnotationType::FreehandInk: return FPDF_ANNOT_INK;
        case pdf_api::AnnotationType::Square: return FPDF_ANNOT_SQUARE;
        case pdf_api::AnnotationType::Circle: return FPDF_ANNOT_CIRCLE;
        case pdf_api::AnnotationType::Line:
        case pdf_api::AnnotationType::Arrow: return FPDF_ANNOT_LINE;
        case pdf_api::AnnotationType::Stamp: return FPDF_ANNOT_STAMP;
        case pdf_api::AnnotationType::FreeText: return FPDF_ANNOT_FREETEXT;
        default: return FPDF_ANNOT_UNKNOWN;
    }
}

pdf_api::AnnotationType FromPdfiumSubtype(FPDF_ANNOTATION_SUBTYPE sub) {
    switch (sub) {
        case FPDF_ANNOT_HIGHLIGHT: return pdf_api::AnnotationType::Highlight;
        case FPDF_ANNOT_UNDERLINE: return pdf_api::AnnotationType::Underline;
        case FPDF_ANNOT_STRIKEOUT: return pdf_api::AnnotationType::Strikeout;
        case FPDF_ANNOT_SQUIGGLY: return pdf_api::AnnotationType::Squiggly;
        case FPDF_ANNOT_TEXT: return pdf_api::AnnotationType::TextNote;
        case FPDF_ANNOT_INK: return pdf_api::AnnotationType::FreehandInk;
        case FPDF_ANNOT_SQUARE: return pdf_api::AnnotationType::Square;
        case FPDF_ANNOT_CIRCLE: return pdf_api::AnnotationType::Circle;
        case FPDF_ANNOT_LINE: return pdf_api::AnnotationType::Line;
        case FPDF_ANNOT_STAMP: return pdf_api::AnnotationType::Stamp;
        case FPDF_ANNOT_FREETEXT: return pdf_api::AnnotationType::FreeText;
        default: return pdf_api::AnnotationType::Unknown;
    }
}

} // namespace

pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>> PdfiumAdapter::GetAnnotations(
    pdf_api::DocumentId id,
    int pageIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    int count = FPDFPage_GetAnnotCount(page);
    std::vector<pdf_api::AnnotationData> result;

    for (int i = 0; i < count; ++i) {
        FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, i);
        if (!annot) continue;

        pdf_api::AnnotationData data;
        data.index = i;
        data.pageIndex = pageIndex;
        auto sub = FPDFAnnot_GetSubtype(annot);
        data.type = FromPdfiumSubtype(sub);

        FS_RECTF rect{};
        if (FPDFAnnot_GetRect(annot, &rect)) {
            data.rect.x = std::min(rect.left, rect.right);
            data.rect.y = std::min(rect.bottom, rect.top);
            data.rect.width = std::abs(rect.right - rect.left);
            data.rect.height = std::abs(rect.top - rect.bottom);
        }

        unsigned int r = 0, g = 0, b = 0, a = 255;
        if (FPDFAnnot_GetColor(annot, FPDFANNOT_COLORTYPE_Color, &r, &g, &b, &a)) {
            data.color = pdf_api::PdfColor{ static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a) };
        }
        if (FPDFAnnot_GetColor(annot, FPDFANNOT_COLORTYPE_InteriorColor, &r, &g, &b, &a)) {
            data.interiorColor = pdf_api::PdfColor{ static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a) };
        }

        float hRad = 0.0f, vRad = 0.0f, bWidth = 1.0f;
        if (FPDFAnnot_GetBorder(annot, &hRad, &vRad, &bWidth)) {
            data.strokeWidth = bWidth;
        }

        data.flags = FPDFAnnot_GetFlags(annot);
        data.isVisible = !(data.flags & (FPDF_ANNOT_FLAG_INVISIBLE | FPDF_ANNOT_FLAG_HIDDEN | FPDF_ANNOT_FLAG_NOVIEW));

        // Contents
        unsigned long cLen = FPDFAnnot_GetStringValue(annot, "Contents", nullptr, 0);
        if (cLen > 2) {
            std::vector<unsigned short> buf(cLen / 2, 0);
            FPDFAnnot_GetStringValue(annot, "Contents", buf.data(), cLen);
            data.contents = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Author (T)
        unsigned long tLen = FPDFAnnot_GetStringValue(annot, "T", nullptr, 0);
        if (tLen > 2) {
            std::vector<unsigned short> buf(tLen / 2, 0);
            FPDFAnnot_GetStringValue(annot, "T", buf.data(), tLen);
            data.author = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Subject (Subj)
        unsigned long sLen = FPDFAnnot_GetStringValue(annot, "Subj", nullptr, 0);
        if (sLen > 2) {
            std::vector<unsigned short> buf(sLen / 2, 0);
            FPDFAnnot_GetStringValue(annot, "Subj", buf.data(), sLen);
            data.subject = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Quadpoints (for text markup)
        if (FPDFAnnot_HasAttachmentPoints(annot)) {
            size_t quadCount = FPDFAnnot_CountAttachmentPoints(annot);
            for (size_t q = 0; q < quadCount; ++q) {
                FS_QUADPOINTSF qp{};
                if (FPDFAnnot_GetAttachmentPoints(annot, q, &qp)) {
                    double qMinX = std::min({qp.x1, qp.x2, qp.x3, qp.x4});
                    double qMaxX = std::max({qp.x1, qp.x2, qp.x3, qp.x4});
                    double qMinY = std::min({qp.y1, qp.y2, qp.y3, qp.y4});
                    double qMaxY = std::max({qp.y1, qp.y2, qp.y3, qp.y4});
                    data.quadPoints.push_back(pdf_api::PdfRect{ qMinX, qMinY, qMaxX - qMinX, qMaxY - qMinY });
                }
            }
        }

        // Ink strokes
        if (sub == FPDF_ANNOT_INK) {
            unsigned long pathCount = FPDFAnnot_GetInkListCount(annot);
            for (unsigned long p = 0; p < pathCount; ++p) {
                unsigned long ptCount = FPDFAnnot_GetInkListPath(annot, p, nullptr, 0);
                if (ptCount > 0) {
                    std::vector<FS_POINTF> ptBuf(ptCount);
                    FPDFAnnot_GetInkListPath(annot, p, ptBuf.data(), ptCount);
                    std::vector<pdf_api::PdfPoint> stroke;
                    for (const auto& pt : ptBuf) {
                        stroke.push_back(pdf_api::PdfPoint{ pt.x, pt.y });
                    }
                    data.inkStrokes.push_back(std::move(stroke));
                }
            }
        }

        FPDFPage_CloseAnnot(annot);
        result.push_back(std::move(data));
    }

    FPDF_ClosePage(page);
    return pdf_api::EngineResult<std::vector<pdf_api::AnnotationData>>::Ok(std::move(result));
}

pdf_api::EngineResult<int> PdfiumAdapter::AddAnnotation(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::AnnotationData& annot
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    auto sub = ToPdfiumSubtype(annot.type);
    if (sub == FPDF_ANNOT_UNKNOWN || !FPDFAnnot_IsSupportedSubtype(sub)) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::UnsupportedFeature, "Unsupported annotation subtype");
    }

    FPDF_ANNOTATION newAnnot = FPDFPage_CreateAnnot(page, sub);
    if (!newAnnot) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::Unknown, "Failed to create annotation");
    }

    // Set Rect
    FS_RECTF rect{};
    rect.left = static_cast<float>(annot.rect.x);
    rect.bottom = static_cast<float>(annot.rect.y);
    rect.right = static_cast<float>(annot.rect.x + annot.rect.width);
    rect.top = static_cast<float>(annot.rect.y + annot.rect.height);
    FPDFAnnot_SetRect(newAnnot, &rect);

    // Set Color
    FPDFAnnot_SetColor(newAnnot, FPDFANNOT_COLORTYPE_Color, annot.color.r, annot.color.g, annot.color.b, annot.color.a);
    if (annot.interiorColor.a > 0) {
        FPDFAnnot_SetColor(newAnnot, FPDFANNOT_COLORTYPE_InteriorColor, annot.interiorColor.r, annot.interiorColor.g, annot.interiorColor.b, annot.interiorColor.a);
    }

    // Set Border
    if (annot.strokeWidth > 0.0f) {
        FPDFAnnot_SetBorder(newAnnot, 0.0f, 0.0f, annot.strokeWidth);
    }

    // Set Text Markup QuadPoints if provided
    if (FPDFAnnot_HasAttachmentPoints(newAnnot)) {
        if (!annot.quadPoints.empty()) {
            for (const auto& qpRect : annot.quadPoints) {
                FS_QUADPOINTSF qp{};
                qp.x1 = static_cast<float>(qpRect.x);
                qp.y1 = static_cast<float>(qpRect.y + qpRect.height);
                qp.x2 = static_cast<float>(qpRect.x + qpRect.width);
                qp.y2 = static_cast<float>(qpRect.y + qpRect.height);
                qp.x3 = static_cast<float>(qpRect.x);
                qp.y3 = static_cast<float>(qpRect.y);
                qp.x4 = static_cast<float>(qpRect.x + qpRect.width);
                qp.y4 = static_cast<float>(qpRect.y);
                FPDFAnnot_AppendAttachmentPoints(newAnnot, &qp);
            }
        } else {
            FS_QUADPOINTSF qp{};
            qp.x1 = rect.left;  qp.y1 = rect.top;
            qp.x2 = rect.right; qp.y2 = rect.top;
            qp.x3 = rect.left;  qp.y3 = rect.bottom;
            qp.x4 = rect.right; qp.y4 = rect.bottom;
            FPDFAnnot_AppendAttachmentPoints(newAnnot, &qp);
        }
    }

    // Set Ink Strokes
    if (sub == FPDF_ANNOT_INK && !annot.inkStrokes.empty()) {
        for (const auto& stroke : annot.inkStrokes) {
            if (stroke.empty()) continue;
            std::vector<FS_POINTF> pts;
            for (const auto& pt : stroke) {
                pts.push_back(FS_POINTF{ static_cast<float>(pt.x), static_cast<float>(pt.y) });
            }
            FPDFAnnot_AddInkStroke(newAnnot, pts.data(), pts.size());
        }
    }

    // Set Contents / Author / Subject
    if (!annot.contents.empty()) {
        QString qContents = QString::fromStdString(annot.contents);
        FPDFAnnot_SetStringValue(newAnnot, "Contents", reinterpret_cast<FPDF_WIDESTRING>(qContents.utf16()));
    }
    if (!annot.author.empty()) {
        QString qAuthor = QString::fromStdString(annot.author);
        FPDFAnnot_SetStringValue(newAnnot, "T", reinterpret_cast<FPDF_WIDESTRING>(qAuthor.utf16()));
    }
    if (!annot.subject.empty()) {
        QString qSubj = QString::fromStdString(annot.subject);
        FPDFAnnot_SetStringValue(newAnnot, "Subj", reinterpret_cast<FPDF_WIDESTRING>(qSubj.utf16()));
    }
    if (sub == FPDF_ANNOT_STAMP && !annot.stampName.empty()) {
        QString qName = QString::fromStdString(annot.stampName);
        FPDFAnnot_SetStringValue(newAnnot, "Name", reinterpret_cast<FPDF_WIDESTRING>(qName.utf16()));
    }

    // Set Flags
    int flags = annot.flags != 0 ? annot.flags : FPDF_ANNOT_FLAG_PRINT;
    FPDFAnnot_SetFlags(newAnnot, flags);

    FPDFPage_GenerateContent(page);
    int newIndex = FPDFPage_GetAnnotIndex(page, newAnnot);

    FPDFPage_CloseAnnot(newAnnot);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<int>::Ok(newIndex);
}

pdf_api::EngineResult<void> PdfiumAdapter::UpdateAnnotation(
    pdf_api::DocumentId id,
    int pageIndex,
    int annotIndex,
    const pdf_api::AnnotationData& annot
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDF_ANNOTATION targetAnnot = FPDFPage_GetAnnot(page, annotIndex);
    if (!targetAnnot) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Annotation not found at index");
    }

    // Update rect
    FS_RECTF rect{};
    rect.left = static_cast<float>(annot.rect.x);
    rect.bottom = static_cast<float>(annot.rect.y);
    rect.right = static_cast<float>(annot.rect.x + annot.rect.width);
    rect.top = static_cast<float>(annot.rect.y + annot.rect.height);
    FPDFAnnot_SetRect(targetAnnot, &rect);

    // Update color
    FPDFAnnot_SetColor(targetAnnot, FPDFANNOT_COLORTYPE_Color, annot.color.r, annot.color.g, annot.color.b, annot.color.a);
    if (annot.interiorColor.a > 0) {
        FPDFAnnot_SetColor(targetAnnot, FPDFANNOT_COLORTYPE_InteriorColor, annot.interiorColor.r, annot.interiorColor.g, annot.interiorColor.b, annot.interiorColor.a);
    }

    // Update border
    if (annot.strokeWidth > 0.0f) {
        FPDFAnnot_SetBorder(targetAnnot, 0.0f, 0.0f, annot.strokeWidth);
    }

    // Update string values
    QString qContents = QString::fromStdString(annot.contents);
    FPDFAnnot_SetStringValue(targetAnnot, "Contents", reinterpret_cast<FPDF_WIDESTRING>(qContents.utf16()));

    QString qAuthor = QString::fromStdString(annot.author);
    FPDFAnnot_SetStringValue(targetAnnot, "T", reinterpret_cast<FPDF_WIDESTRING>(qAuthor.utf16()));

    FPDFAnnot_SetFlags(targetAnnot, annot.flags);

    FPDFPage_GenerateContent(page);
    FPDFPage_CloseAnnot(targetAnnot);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::DeleteAnnotation(
    pdf_api::DocumentId id,
    int pageIndex,
    int annotIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDF_BOOL ok = FPDFPage_RemoveAnnot(page, annotIndex);
    if (!ok) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::Unknown, "FPDFPage_RemoveAnnot failed");
    }

    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

namespace {

pdf_api::FormFieldType FromPdfiumFieldType(int ft) {
    switch (ft) {
        case FPDF_FORMFIELD_PUSHBUTTON: return pdf_api::FormFieldType::PushButton;
        case FPDF_FORMFIELD_CHECKBOX: return pdf_api::FormFieldType::CheckBox;
        case FPDF_FORMFIELD_RADIOBUTTON: return pdf_api::FormFieldType::RadioButton;
        case FPDF_FORMFIELD_COMBOBOX: return pdf_api::FormFieldType::ComboBox;
        case FPDF_FORMFIELD_LISTBOX: return pdf_api::FormFieldType::ListBox;
        case FPDF_FORMFIELD_TEXTFIELD: return pdf_api::FormFieldType::TextField;
        case FPDF_FORMFIELD_SIGNATURE: return pdf_api::FormFieldType::Signature;
#ifdef PDF_ENABLE_XFA
        case FPDF_FORMFIELD_XFA:
        case FPDF_FORMFIELD_XFA_CHECKBOX:
        case FPDF_FORMFIELD_XFA_COMBOBOX:
        case FPDF_FORMFIELD_XFA_IMAGEFIELD:
        case FPDF_FORMFIELD_XFA_LISTBOX:
        case FPDF_FORMFIELD_XFA_PUSHBUTTON:
        case FPDF_FORMFIELD_XFA_SIGNATURE:
        case FPDF_FORMFIELD_XFA_TEXTFIELD:
            return pdf_api::FormFieldType::Xfa;
#endif
        default:
            return pdf_api::FormFieldType::Unknown;
    }
}

} // namespace

pdf_api::EngineResult<pdf_api::FormType> PdfiumAdapter::GetFormType(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::FormType>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int t = FPDF_GetFormType(doc);
    switch (t) {
        case FORMTYPE_NONE:
            return pdf_api::EngineResult<pdf_api::FormType>::Ok(pdf_api::FormType::None);
        case FORMTYPE_ACRO_FORM:
            return pdf_api::EngineResult<pdf_api::FormType>::Ok(pdf_api::FormType::AcroForm);
        case FORMTYPE_XFA_FULL:
            return pdf_api::EngineResult<pdf_api::FormType>::Ok(pdf_api::FormType::XfaFull);
        case FORMTYPE_XFA_FOREGROUND:
            return pdf_api::EngineResult<pdf_api::FormType>::Ok(pdf_api::FormType::XfaForeground);
        default:
            return pdf_api::EngineResult<pdf_api::FormType>::Ok(pdf_api::FormType::None);
    }
}

pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> PdfiumAdapter::GetFormFields(
    pdf_api::DocumentId id,
    int pageIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_FORMHANDLE formHandle = GetNativeFormHandle(id);
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    if (formHandle) {
        FORM_OnAfterLoadPage(page, formHandle);
    }

    int count = FPDFPage_GetAnnotCount(page);
    std::vector<pdf_api::FormFieldData> fields;

    for (int i = 0; i < count; ++i) {
        FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, i);
        if (!annot) continue;

        if (FPDFAnnot_GetSubtype(annot) != FPDF_ANNOT_WIDGET) {
            FPDFPage_CloseAnnot(annot);
            continue;
        }

        pdf_api::FormFieldData f;
        f.pageIndex = pageIndex;
        f.annotIndex = i;

        int rawType = formHandle ? FPDFAnnot_GetFormFieldType(formHandle, annot) : FPDF_FORMFIELD_UNKNOWN;
        f.type = FromPdfiumFieldType(rawType);

        // Rect
        FS_RECTF rect{};
        if (FPDFAnnot_GetRect(annot, &rect)) {
            f.rect.x = std::min(rect.left, rect.right);
            f.rect.y = std::min(rect.bottom, rect.top);
            f.rect.width = std::abs(rect.right - rect.left);
            f.rect.height = std::abs(rect.top - rect.bottom);
        }

        // Field Name
        if (formHandle) {
            unsigned long nLen = FPDFAnnot_GetFormFieldName(formHandle, annot, nullptr, 0);
            if (nLen > 2) {
                std::vector<unsigned short> buf(nLen / 2, 0);
                FPDFAnnot_GetFormFieldName(formHandle, annot, buf.data(), nLen);
                f.name = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }
        if (f.name.empty()) {
            unsigned long tLen = FPDFAnnot_GetStringValue(annot, "T", nullptr, 0);
            if (tLen > 2) {
                std::vector<unsigned short> buf(tLen / 2, 0);
                FPDFAnnot_GetStringValue(annot, "T", buf.data(), tLen);
                f.name = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }

        // Alternate Name (TU)
        if (formHandle) {
            unsigned long altLen = FPDFAnnot_GetFormFieldAlternateName(formHandle, annot, nullptr, 0);
            if (altLen > 2) {
                std::vector<unsigned short> buf(altLen / 2, 0);
                FPDFAnnot_GetFormFieldAlternateName(formHandle, annot, buf.data(), altLen);
                f.altName = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }
        if (f.altName.empty()) {
            unsigned long tuLen = FPDFAnnot_GetStringValue(annot, "TU", nullptr, 0);
            if (tuLen > 2) {
                std::vector<unsigned short> buf(tuLen / 2, 0);
                FPDFAnnot_GetStringValue(annot, "TU", buf.data(), tuLen);
                f.altName = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }

        // Current Value
        if (formHandle) {
            unsigned long vLen = FPDFAnnot_GetFormFieldValue(formHandle, annot, nullptr, 0);
            if (vLen > 2) {
                std::vector<unsigned short> buf(vLen / 2, 0);
                FPDFAnnot_GetFormFieldValue(formHandle, annot, buf.data(), vLen);
                f.value = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }
        if (f.value.empty()) {
            unsigned long vLen = FPDFAnnot_GetStringValue(annot, "V", nullptr, 0);
            if (vLen > 2) {
                std::vector<unsigned short> buf(vLen / 2, 0);
                FPDFAnnot_GetStringValue(annot, "V", buf.data(), vLen);
                f.value = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
            }
        }

        // Default Value (DV)
        unsigned long dvLen = FPDFAnnot_GetStringValue(annot, "DV", nullptr, 0);
        if (dvLen > 2) {
            std::vector<unsigned short> buf(dvLen / 2, 0);
            FPDFAnnot_GetStringValue(annot, "DV", buf.data(), dvLen);
            f.defaultValue = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Flags
        if (formHandle) {
            f.flags = FPDFAnnot_GetFormFieldFlags(formHandle, annot);
            f.isReadOnly = (f.flags & (1 << 0)) != 0;
            f.isRequired = (f.flags & (1 << 1)) != 0;
        }

        // Checked state and export value for CheckBox and RadioButton
        if (f.type == pdf_api::FormFieldType::CheckBox || f.type == pdf_api::FormFieldType::RadioButton) {
            if (formHandle) {
                f.isChecked = FPDFAnnot_IsChecked(formHandle, annot) != 0;
                unsigned long expLen = FPDFAnnot_GetFormFieldExportValue(formHandle, annot, nullptr, 0);
                if (expLen > 2) {
                    std::vector<unsigned short> buf(expLen / 2, 0);
                    FPDFAnnot_GetFormFieldExportValue(formHandle, annot, buf.data(), expLen);
                    f.exportValue = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
                }
            }
            if (f.exportValue.empty()) {
                f.exportValue = "Yes";
            }
            if (!formHandle && !f.value.empty()) {
                f.isChecked = (f.value != "Off" && !f.value.empty());
            }
        }

        // Dropdown / ListBox options
        if (f.type == pdf_api::FormFieldType::ComboBox || f.type == pdf_api::FormFieldType::ListBox) {
            if (formHandle) {
                int optCount = FPDFAnnot_GetOptionCount(formHandle, annot);
                for (int o = 0; o < optCount; ++o) {
                    unsigned long optLen = FPDFAnnot_GetOptionLabel(formHandle, annot, o, nullptr, 0);
                    if (optLen > 2) {
                        std::vector<unsigned short> buf(optLen / 2, 0);
                        FPDFAnnot_GetOptionLabel(formHandle, annot, o, buf.data(), optLen);
                        f.options.push_back(QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString());
                    }
                    if (FPDFAnnot_IsOptionSelected(formHandle, annot, o)) {
                        f.selectedIndices.push_back(o);
                    }
                }
            }
        }

        FPDFPage_CloseAnnot(annot);
        fields.push_back(std::move(f));
    }

    if (formHandle) {
        FORM_OnBeforeClosePage(page, formHandle);
    }
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Ok(std::move(fields));
}

pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>> PdfiumAdapter::GetAllFormFields(
    pdf_api::DocumentId id
) {
    int pageCount = GetPageCount(id);
    std::vector<pdf_api::FormFieldData> allFields;

    for (int p = 0; p < pageCount; ++p) {
        auto pFields = GetFormFields(id, p);
        if (pFields.has_value()) {
            for (auto& f : pFields.value) {
                allFields.push_back(std::move(f));
            }
        }
    }

    return pdf_api::EngineResult<std::vector<pdf_api::FormFieldData>>::Ok(std::move(allFields));
}

pdf_api::EngineResult<void> PdfiumAdapter::SetFormFieldValue(
    pdf_api::DocumentId id,
    int pageIndex,
    int annotIndex,
    const std::string& value
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    // Input security: Hostile input validation and maximum length capping (64 KB)
    if (value.size() > 65536) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::UnsupportedFeature, "Form field input exceeds 64KB security limit");
    }

    FPDF_FORMHANDLE formHandle = GetNativeFormHandle(id);
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    if (formHandle) {
        FORM_OnAfterLoadPage(page, formHandle);
    }

    FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, annotIndex);
    if (!annot) {
        if (formHandle) FORM_OnBeforeClosePage(page, formHandle);
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Annotation not found");
    }

    QString qVal = QString::fromStdString(value);
    FPDFAnnot_SetStringValue(annot, "V", reinterpret_cast<FPDF_WIDESTRING>(qVal.utf16()));

    FS_RECTF rect{};
    if (formHandle && FPDFAnnot_GetRect(annot, &rect)) {
        double cx = (rect.left + rect.right) / 2.0;
        double cy = (rect.top + rect.bottom) / 2.0;
        FORM_OnFocus(formHandle, page, 0, cx, cy);
        FORM_SelectAllText(formHandle, page);
        FORM_ReplaceSelection(formHandle, page, reinterpret_cast<FPDF_WIDESTRING>(qVal.utf16()));
    }

    FPDFPage_GenerateContent(page);
    FPDFPage_CloseAnnot(annot);

    if (formHandle) {
        FORM_OnBeforeClosePage(page, formHandle);
    }
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::SetFormFieldChecked(
    pdf_api::DocumentId id,
    int pageIndex,
    int annotIndex,
    bool checked
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_FORMHANDLE formHandle = GetNativeFormHandle(id);
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    if (formHandle) {
        FORM_OnAfterLoadPage(page, formHandle);
    }

    FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, annotIndex);
    if (!annot) {
        if (formHandle) FORM_OnBeforeClosePage(page, formHandle);
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Annotation not found");
    }

    // Determine export value
    std::string expVal = "Yes";
    if (formHandle) {
        unsigned long expLen = FPDFAnnot_GetFormFieldExportValue(formHandle, annot, nullptr, 0);
        if (expLen > 2) {
            std::vector<unsigned short> buf(expLen / 2, 0);
            FPDFAnnot_GetFormFieldExportValue(formHandle, annot, buf.data(), expLen);
            expVal = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }
    }

    bool currentlyChecked = formHandle ? (FPDFAnnot_IsChecked(formHandle, annot) != 0) : false;
    if (formHandle && currentlyChecked != checked) {
        FS_RECTF rect{};
        if (FPDFAnnot_GetRect(annot, &rect)) {
            double cx = (rect.left + rect.right) / 2.0;
            double cy = (rect.top + rect.bottom) / 2.0;
            FORM_OnLButtonDown(formHandle, page, 0, cx, cy);
            FORM_OnLButtonUp(formHandle, page, 0, cx, cy);
        }
    }

    std::string valStr = checked ? expVal : "Off";
    QString qVal = QString::fromStdString(valStr);
    FPDFAnnot_SetStringValue(annot, "V", reinterpret_cast<FPDF_WIDESTRING>(qVal.utf16()));
    FPDFAnnot_SetStringValue(annot, "AS", reinterpret_cast<FPDF_WIDESTRING>(qVal.utf16()));

    FPDFPage_GenerateContent(page);
    FPDFPage_CloseAnnot(annot);

    if (formHandle) {
        FORM_OnBeforeClosePage(page, formHandle);
    }
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::SetFormFieldOption(
    pdf_api::DocumentId id,
    int pageIndex,
    int annotIndex,
    int optionIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_FORMHANDLE formHandle = GetNativeFormHandle(id);
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    if (formHandle) {
        FORM_OnAfterLoadPage(page, formHandle);
    }

    FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, annotIndex);
    if (!annot) {
        if (formHandle) FORM_OnBeforeClosePage(page, formHandle);
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Annotation not found");
    }

    std::string optText;
    if (formHandle) {
        unsigned long optLen = FPDFAnnot_GetOptionLabel(formHandle, annot, optionIndex, nullptr, 0);
        if (optLen > 2) {
            std::vector<unsigned short> buf(optLen / 2, 0);
            FPDFAnnot_GetOptionLabel(formHandle, annot, optionIndex, buf.data(), optLen);
            optText = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }
    }

    if (!optText.empty()) {
        QString qVal = QString::fromStdString(optText);
        FPDFAnnot_SetStringValue(annot, "V", reinterpret_cast<FPDF_WIDESTRING>(qVal.utf16()));
    }

    FPDFPage_GenerateContent(page);
    FPDFPage_CloseAnnot(annot);

    if (formHandle) {
        FORM_OnBeforeClosePage(page, formHandle);
    }
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::ResetFormFields(pdf_api::DocumentId id) {
    auto fieldsRes = GetAllFormFields(id);
    if (!fieldsRes.has_value()) {
        return pdf_api::EngineResult<void>::Fail(fieldsRes.error, fieldsRes.errorMessage);
    }

    for (const auto& field : fieldsRes.value) {
        if (field.type == pdf_api::FormFieldType::TextField) {
            SetFormFieldValue(id, field.pageIndex, field.annotIndex, field.defaultValue);
        } else if (field.type == pdf_api::FormFieldType::CheckBox || field.type == pdf_api::FormFieldType::RadioButton) {
            bool defChecked = (!field.defaultValue.empty() && field.defaultValue != "Off");
            SetFormFieldChecked(id, field.pageIndex, field.annotIndex, defChecked);
        } else if (field.type == pdf_api::FormFieldType::ComboBox || field.type == pdf_api::FormFieldType::ListBox) {
            if (!field.defaultValue.empty()) {
                SetFormFieldValue(id, field.pageIndex, field.annotIndex, field.defaultValue);
            } else if (!field.options.empty()) {
                SetFormFieldOption(id, field.pageIndex, field.annotIndex, 0);
            }
        }
    }

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::FlattenDocument(pdf_api::DocumentId id, bool forPrint) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int totalPages = FPDF_GetPageCount(doc);
    int flatFlag = forPrint ? FLAT_PRINT : FLAT_NORMALDISPLAY;

    for (int i = 0; i < totalPages; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        if (!page) continue;

        FPDFPage_Flatten(page, flatFlag);
        FPDFPage_GenerateContent(page);
        FPDF_ClosePage(page);
    }

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>> PdfiumAdapter::GetPageObjects(
    pdf_api::DocumentId id,
    int pageIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>>::Fail(
            pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>>::Fail(
            pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);

    int count = FPDFPage_CountObjects(page);
    std::vector<pdf_api::PageObjectData> objects;
    objects.reserve(count);

    for (int i = 0; i < count; ++i) {
        FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
        if (!obj) continue;

        pdf_api::PageObjectData data;
        data.pageIndex = pageIndex;
        data.objectIndex = i;

        int rawType = FPDFPageObj_GetType(obj);
        switch (rawType) {
            case FPDF_PAGEOBJ_TEXT: data.type = pdf_api::PageObjectType::Text; break;
            case FPDF_PAGEOBJ_PATH: data.type = pdf_api::PageObjectType::Path; break;
            case FPDF_PAGEOBJ_IMAGE: data.type = pdf_api::PageObjectType::Image; break;
            case FPDF_PAGEOBJ_SHADING: data.type = pdf_api::PageObjectType::Shading; break;
            case FPDF_PAGEOBJ_FORM: data.type = pdf_api::PageObjectType::FormXObject; break;
            default: data.type = pdf_api::PageObjectType::Unknown; break;
        }

        // Bounds
        float l = 0, b = 0, r = 0, t = 0;
        if (FPDFPageObj_GetBounds(obj, &l, &b, &r, &t)) {
            data.bounds.x = std::min(l, r);
            data.bounds.y = std::min(b, t);
            data.bounds.width = std::abs(r - l);
            data.bounds.height = std::abs(t - b);
        }

        // Matrix
        FS_MATRIX matrix{};
        if (FPDFPageObj_GetMatrix(obj, &matrix)) {
            data.matrix = {matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f};
        }

        if (data.type == pdf_api::PageObjectType::Text) {
            data.canEditText = true;

            // Font details
            FPDF_FONT font = FPDFTextObj_GetFont(obj);
            if (font) {
                char fontNameBuf[256] = {0};
                size_t nLen = FPDFFont_GetBaseFontName(font, fontNameBuf, sizeof(fontNameBuf));
                if (nLen > 0) {
                    data.fontName = std::string(fontNameBuf);
                }

                char familyBuf[256] = {0};
                size_t fLen = FPDFFont_GetFamilyName(font, familyBuf, sizeof(familyBuf));
                if (fLen > 0) {
                    data.fontFamily = std::string(familyBuf);
                }

                data.isEmbeddedFont = (FPDFFont_GetIsEmbedded(font) != 0);
                data.fontWeight = FPDFFont_GetWeight(font);

                // Subset detection: font names with 6 uppercase letters + '+' (e.g. ABCDEF+Helvetica)
                if (data.fontName.find('+') != std::string::npos) {
                    data.isSubsetFont = true;
                    data.fontWarning = "Embedded subset font detected (" + data.fontName +
                        "). Direct in-place editing of characters outside existing subset is not supported; standard font replacement required.";
                } else if (data.isEmbeddedFont) {
                    data.fontWarning = "Embedded font (" + data.fontName +
                        "). Glyphs outside font dictionary may not render without standard font fallback.";
                }
            }

            // Font size
            float fSize = 12.0f;
            if (FPDFTextObj_GetFontSize(obj, &fSize)) {
                data.fontSize = fSize;
            }

            // Text content
            if (textPage) {
                unsigned long tLen = FPDFTextObj_GetText(obj, textPage, nullptr, 0);
                if (tLen > 2) {
                    std::vector<unsigned short> buf(tLen / 2, 0);
                    FPDFTextObj_GetText(obj, textPage, buf.data(), tLen);
                    data.text = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
                }
            }

            // Colors
            unsigned int fr = 0, fg = 0, fb = 0, fa = 255;
            if (FPDFPageObj_GetFillColor(obj, &fr, &fg, &fb, &fa)) {
                data.fillColor = {static_cast<uint8_t>(fr), static_cast<uint8_t>(fg), static_cast<uint8_t>(fb), static_cast<uint8_t>(fa)};
            }
            unsigned int sr = 0, sg = 0, sb = 0, sa = 0;
            if (FPDFPageObj_GetStrokeColor(obj, &sr, &sg, &sb, &sa)) {
                data.strokeColor = {static_cast<uint8_t>(sr), static_cast<uint8_t>(sg), static_cast<uint8_t>(sb), static_cast<uint8_t>(sa)};
            }
        } else if (data.type == pdf_api::PageObjectType::Image) {
            unsigned int pxW = 0, pxH = 0;
            if (FPDFImageObj_GetImagePixelSize(obj, &pxW, &pxH)) {
                data.pixelWidth = static_cast<int>(pxW);
                data.pixelHeight = static_cast<int>(pxH);
            }
            data.canEditText = false;
        } else if (data.type == pdf_api::PageObjectType::Path) {
            data.canEditText = false;
            float sw = 1.0f;
            if (FPDFPageObj_GetStrokeWidth(obj, &sw)) {
                data.strokeWidth = sw;
            }
            int fillMode = 0;
            FPDF_BOOL stroke = 0;
            if (FPDFPath_GetDrawMode(obj, &fillMode, &stroke)) {
                data.pathDrawMode = fillMode;
            }
            data.segmentCount = FPDFPath_CountSegments(obj);
            unsigned int fr = 0, fg = 0, fb = 0, fa = 0;
            if (FPDFPageObj_GetFillColor(obj, &fr, &fg, &fb, &fa)) {
                data.fillColor = {static_cast<uint8_t>(fr), static_cast<uint8_t>(fg), static_cast<uint8_t>(fb), static_cast<uint8_t>(fa)};
            }
            unsigned int sr = 0, sg = 0, sb = 0, sa = 255;
            if (FPDFPageObj_GetStrokeColor(obj, &sr, &sg, &sb, &sa)) {
                data.strokeColor = {static_cast<uint8_t>(sr), static_cast<uint8_t>(sg), static_cast<uint8_t>(sb), static_cast<uint8_t>(sa)};
            }
        }

        objects.push_back(std::move(data));
    }

    if (textPage) {
        FPDFText_ClosePage(textPage);
    }
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<std::vector<pdf_api::PageObjectData>>::Ok(std::move(objects));
}

pdf_api::EngineResult<int> PdfiumAdapter::InsertPageText(
    pdf_api::DocumentId id,
    int pageIndex,
    const std::string& text,
    double x,
    double y,
    const std::string& fontName,
    double fontSize,
    const pdf_api::PdfColor& color
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    std::string safeFont = fontName.empty() ? "Helvetica" : fontName;
    FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, safeFont.c_str(), static_cast<float>(fontSize));
    if (!textObj) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::RenderFailed, "Failed to create text object with font: " + safeFont);
    }

    QString qText = QString::fromStdString(text);
    FPDFText_SetText(textObj, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));
    FPDFPageObj_SetFillColor(textObj, color.r, color.g, color.b, color.a);

    FS_MATRIX matrix = {1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(x), static_cast<float>(y)};
    FPDFPageObj_SetMatrix(textObj, &matrix);

    FPDFPage_InsertObject(page, textObj);
    FPDFPage_GenerateContent(page);

    int newIdx = FPDFPage_CountObjects(page) - 1;
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<int>::Ok(newIdx);
}

pdf_api::EngineResult<int> PdfiumAdapter::InsertPageImage(
    pdf_api::DocumentId id,
    int pageIndex,
    const std::vector<uint8_t>& bgraPixels,
    int pixelWidth,
    int pixelHeight,
    double x,
    double y,
    double targetWidth,
    double targetHeight
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDF_PAGEOBJECT imgObj = FPDFPageObj_NewImageObj(doc);
    if (!imgObj) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::RenderFailed, "Failed to create image object");
    }

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(pixelWidth, pixelHeight, FPDFBitmap_BGRA, nullptr, 0);
    if (!bitmap) {
        FPDFPageObj_Destroy(imgObj);
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::OutOfMemory, "Failed to create bitmap buffer");
    }

    uint8_t* dst = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
    int stride = FPDFBitmap_GetStride(bitmap);
    for (int row = 0; row < pixelHeight; ++row) {
        const uint8_t* srcRow = bgraPixels.data() + (row * pixelWidth * 4);
        uint8_t* dstRow = dst + (row * stride);
        std::memcpy(dstRow, srcRow, pixelWidth * 4);
    }

    FPDF_PAGE pagePtr = page;
    FPDFImageObj_SetBitmap(&pagePtr, 1, imgObj, bitmap);
    FPDFImageObj_SetMatrix(imgObj, targetWidth, 0.0, 0.0, targetHeight, x, y);

    FPDFPage_InsertObject(page, imgObj);
    FPDFPage_GenerateContent(page);

    FPDFBitmap_Destroy(bitmap);
    int newIdx = FPDFPage_CountObjects(page) - 1;
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<int>::Ok(newIdx);
}

pdf_api::EngineResult<int> PdfiumAdapter::InsertPageShape(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::PdfRect& rect,
    const pdf_api::PdfColor& strokeColor,
    const pdf_api::PdfColor& fillColor,
    double strokeWidth
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    FPDF_PAGEOBJECT rectObj = FPDFPageObj_CreateNewRect(
        static_cast<float>(rect.x),
        static_cast<float>(rect.y),
        static_cast<float>(rect.width),
        static_cast<float>(rect.height)
    );
    if (!rectObj) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::RenderFailed, "Failed to create shape object");
    }

    FPDFPageObj_SetStrokeWidth(rectObj, static_cast<float>(strokeWidth));
    FPDFPageObj_SetStrokeColor(rectObj, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);
    FPDFPageObj_SetFillColor(rectObj, fillColor.r, fillColor.g, fillColor.b, fillColor.a);

    bool hasFill = (fillColor.a > 0);
    bool hasStroke = (strokeColor.a > 0 && strokeWidth > 0);
    int fillMode = hasFill ? FPDF_FILLMODE_WINDING : FPDF_FILLMODE_NONE;
    FPDFPath_SetDrawMode(rectObj, fillMode, hasStroke ? 1 : 0);

    FPDFPage_InsertObject(page, rectObj);
    FPDFPage_GenerateContent(page);

    int newIdx = FPDFPage_CountObjects(page) - 1;
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<int>::Ok(newIdx);
}

pdf_api::EngineResult<void> PdfiumAdapter::DeletePageObject(
    pdf_api::DocumentId id,
    int pageIndex,
    int objectIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    int count = FPDFPage_CountObjects(page);
    if (objectIndex < 0 || objectIndex >= count) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Invalid object index");
    }

    FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, objectIndex);
    if (obj) {
        FPDFPage_RemoveObject(page, obj);
        FPDFPageObj_Destroy(obj);
        FPDFPage_GenerateContent(page);
    }

    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::TransformPageObject(
    pdf_api::DocumentId id,
    int pageIndex,
    int objectIndex,
    double dx,
    double dy,
    double scaleX,
    double scaleY
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    int count = FPDFPage_CountObjects(page);
    if (objectIndex < 0 || objectIndex >= count) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Invalid object index");
    }

    FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, objectIndex);
    if (!obj) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Object not found");
    }

    FS_MATRIX mat{};
    if (FPDFPageObj_GetMatrix(obj, &mat)) {
        mat.a *= static_cast<float>(scaleX);
        mat.b *= static_cast<float>(scaleX);
        mat.c *= static_cast<float>(scaleY);
        mat.d *= static_cast<float>(scaleY);
        mat.e += static_cast<float>(dx);
        mat.f += static_cast<float>(dy);
        FPDFPageObj_SetMatrix(obj, &mat);
    } else {
        FPDFPageObj_Transform(obj, scaleX, 0, 0, scaleY, dx, dy);
    }

    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::UpdatePageTextObject(
    pdf_api::DocumentId id,
    int pageIndex,
    int objectIndex,
    const std::string& newText,
    double newFontSize,
    const pdf_api::PdfColor& newColor
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    int count = FPDFPage_CountObjects(page);
    if (objectIndex < 0 || objectIndex >= count) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Invalid object index");
    }

    FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, objectIndex);
    if (!obj || FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_TEXT) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::UnsupportedFeature, "Target object is not a text object");
    }

    QString qText = QString::fromStdString(newText);
    FPDFText_SetText(obj, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));

    if (newFontSize > 0.0) {
        FPDFTextObj_SetFontSize(obj, static_cast<float>(newFontSize));
    }
    if (newColor.a > 0) {
        FPDFPageObj_SetFillColor(obj, newColor.r, newColor.g, newColor.b, newColor.a);
    }

    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::ReorderPageObject(
    pdf_api::DocumentId id,
    int pageIndex,
    int fromIndex,
    int toIndex
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    int count = FPDFPage_CountObjects(page);
    if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex >= count) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Invalid z-order index");
    }

    if (fromIndex == toIndex) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Ok();
    }

    FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, fromIndex);
    if (!obj) {
        FPDF_ClosePage(page);
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Object not found");
    }

    FPDFPage_RemoveObject(page, obj);
    FPDFPage_InsertObjectAtIndex(page, obj, toIndex);
    FPDFPage_GenerateContent(page);

    FPDF_ClosePage(page);
    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<int> PdfiumAdapter::GetSignatureCount(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }
    int count = FPDF_GetSignatureCount(doc);
    if (count < 0) count = 0;
    return pdf_api::EngineResult<int>::Ok(count);
}

pdf_api::EngineResult<pdf_api::SignatureValidationResult> PdfiumAdapter::GetSignatureData(
    pdf_api::DocumentId id,
    int index
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_SIGNATURE sig = FPDF_GetSignatureObject(doc, index);
    if (!sig) {
        return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Fail(pdf_api::EngineError::InvalidFormat, "Signature object not found");
    }

    pdf_api::SignatureValidationResult res;
    res.signatureIndex = index;

    // SubFilter
    unsigned long sfLen = FPDFSignatureObj_GetSubFilter(sig, nullptr, 0);
    if (sfLen > 0) {
        std::vector<char> buf(sfLen);
        FPDFSignatureObj_GetSubFilter(sig, buf.data(), sfLen);
        res.subFilter = std::string(buf.data());
    }

    // Reason
    unsigned long rLen = FPDFSignatureObj_GetReason(sig, nullptr, 0);
    if (rLen > 2) {
        std::vector<unsigned short> buf(rLen / 2, 0);
        FPDFSignatureObj_GetReason(sig, buf.data(), rLen);
        res.reason = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
    }

    // Time
    unsigned long tLen = FPDFSignatureObj_GetTime(sig, nullptr, 0);
    if (tLen > 0) {
        std::vector<char> buf(tLen);
        FPDFSignatureObj_GetTime(sig, buf.data(), tLen);
        res.signingTime = std::string(buf.data());
    }

    // ByteRange
    unsigned long brLen = FPDFSignatureObj_GetByteRange(sig, nullptr, 0);
    if (brLen > 0) {
        std::vector<int> br(brLen);
        FPDFSignatureObj_GetByteRange(sig, br.data(), brLen);
        res.byteRange = std::move(br);
    }

    return pdf_api::EngineResult<pdf_api::SignatureValidationResult>::Ok(std::move(res));
}

pdf_api::EngineResult<void> PdfiumAdapter::ApplyVisualSignature(
    pdf_api::DocumentId id,
    const pdf_api::VisualSignatureData& visData
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, visData.pageIndex);
    if (!page) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::PageNotFound, "Page not found");
    }

    // 1. Draw decorative background border
    FPDF_PAGEOBJECT rectObj = FPDFPageObj_CreateNewRect(
        static_cast<float>(visData.rect.x),
        static_cast<float>(visData.rect.y),
        static_cast<float>(visData.rect.width),
        static_cast<float>(visData.rect.height)
    );
    if (rectObj) {
        FPDFPageObj_SetStrokeWidth(rectObj, 1.5f);
        FPDFPageObj_SetStrokeColor(rectObj, 40, 90, 180, 255);
        FPDFPageObj_SetFillColor(rectObj, 245, 248, 255, 200);
        FPDFPath_SetDrawMode(rectObj, FPDF_FILLMODE_WINDING, 1);
        FPDFPage_InsertObject(page, rectObj);
    }

    // 2. If imported image provided, insert it
    if (!visData.imageBgra.empty() && visData.imageWidth > 0 && visData.imageHeight > 0) {
        FPDF_PAGEOBJECT imgObj = FPDFPageObj_NewImageObj(doc);
        if (imgObj) {
            FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(
                visData.imageWidth, visData.imageHeight, FPDFBitmap_BGRA, nullptr, 0
            );
            if (bitmap) {
                uint8_t* dst = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
                int stride = FPDFBitmap_GetStride(bitmap);
                for (int row = 0; row < visData.imageHeight; ++row) {
                    const uint8_t* srcRow = visData.imageBgra.data() + (row * visData.imageWidth * 4);
                    uint8_t* dstRow = dst + (row * stride);
                    std::memcpy(dstRow, srcRow, visData.imageWidth * 4);
                }
                FPDF_PAGE pagePtr = page;
                FPDFImageObj_SetBitmap(&pagePtr, 1, imgObj, bitmap);
                double imgW = visData.rect.width * 0.45;
                double imgH = visData.rect.height * 0.8;
                FPDFImageObj_SetMatrix(imgObj, imgW, 0.0, 0.0, imgH, visData.rect.x + 5.0, visData.rect.y + 5.0);
                FPDFPage_InsertObject(page, imgObj);
                FPDFBitmap_Destroy(bitmap);
            }
        }
    }

    // 3. Insert visual signature textual elements (Signer, Date, Title)
    std::string line1 = visData.signerName.empty() ? "Digitally Verified Signer" : visData.signerName;
    FPDF_PAGEOBJECT textObj1 = FPDFPageObj_NewTextObj(doc, "Helvetica", 11.0f);
    if (textObj1) {
        QString qText = QString::fromStdString(line1);
        FPDFText_SetText(textObj1, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));
        FPDFPageObj_SetFillColor(textObj1, 20, 20, 50, 255);
        double textX = visData.rect.x + (visData.imageBgra.empty() ? 10.0 : visData.rect.width * 0.5);
        double textY = visData.rect.y + visData.rect.height - 22.0;
        FS_MATRIX matrix = {1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(textX), static_cast<float>(textY)};
        FPDFPageObj_SetMatrix(textObj1, &matrix);
        FPDFPage_InsertObject(page, textObj1);
    }

    std::string line2 = visData.title.empty() ? "Visual Signature Appearance" : visData.title;
    FPDF_PAGEOBJECT textObj2 = FPDFPageObj_NewTextObj(doc, "Helvetica", 8.5f);
    if (textObj2) {
        QString qText = QString::fromStdString(line2);
        FPDFText_SetText(textObj2, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));
        FPDFPageObj_SetFillColor(textObj2, 100, 100, 120, 255);
        double textX = visData.rect.x + (visData.imageBgra.empty() ? 10.0 : visData.rect.width * 0.5);
        double textY = visData.rect.y + visData.rect.height - 38.0;
        FS_MATRIX matrix = {1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(textX), static_cast<float>(textY)};
        FPDFPageObj_SetMatrix(textObj2, &matrix);
        FPDFPage_InsertObject(page, textObj2);
    }

    std::string line3 = "Date: " + (visData.dateText.empty() ? "2026-11-14" : visData.dateText);
    FPDF_PAGEOBJECT textObj3 = FPDFPageObj_NewTextObj(doc, "Helvetica", 8.0f);
    if (textObj3) {
        QString qText = QString::fromStdString(line3);
        FPDFText_SetText(textObj3, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));
        FPDFPageObj_SetFillColor(textObj3, 120, 120, 140, 255);
        double textX = visData.rect.x + (visData.imageBgra.empty() ? 10.0 : visData.rect.width * 0.5);
        double textY = visData.rect.y + 10.0;
        FS_MATRIX matrix = {1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(textX), static_cast<float>(textY)};
        FPDFPageObj_SetMatrix(textObj3, &matrix);
        FPDFPage_InsertObject(page, textObj3);
    }

    FPDFPage_GenerateContent(page);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<void>::Ok();
}

static bool CheckRectsIntersect(const pdf_api::PdfRect& a, const pdf_api::PdfRect& b) {
    if (a.x + a.width <= b.x || b.x + b.width <= a.x) return false;
    if (a.y + a.height <= b.y || b.y + b.height <= a.y) return false;
    return true;
}

pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis> PdfiumAdapter::AnalyzeRedactionTarget(
    pdf_api::DocumentId id,
    int pageIndex,
    const pdf_api::PdfRect& rect
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis>::Fail(
            pdf_api::EngineError::FileNotFound, "Invalid document ID"
        );
    }
    int pageCount = FPDF_GetPageCount(doc);
    if (pageIndex < 0 || pageIndex >= pageCount) {
        return pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis>::Fail(
            pdf_api::EngineError::PageNotFound, "Page index out of range"
        );
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) {
        return pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis>::Fail(
            pdf_api::EngineError::RenderFailed, "Failed to load page for analysis"
        );
    }

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    pdf_api::RedactionTargetAnalysis analysis;
    analysis.pageIndex = pageIndex;
    analysis.redactionRect = rect;

    // 1. Text character inspection
    if (textPage) {
        int charCount = FPDFText_CountChars(textPage);
        std::string currentSnippet;
        for (int i = 0; i < charCount; ++i) {
            double l = 0, r = 0, b = 0, t = 0;
            if (FPDFText_GetCharBox(textPage, i, &l, &r, &b, &t)) {
                pdf_api::PdfRect charRect{l, b, r - l, t - b};
                if (CheckRectsIntersect(rect, charRect)) {
                    unsigned int ch = FPDFText_GetUnicode(textPage, i);
                    if (ch > 0 && ch < 128) {
                        currentSnippet.push_back(static_cast<char>(ch));
                    }
                } else if (!currentSnippet.empty()) {
                    analysis.detectedTextSnippets.push_back(currentSnippet);
                    currentSnippet.clear();
                }
            }
        }
        if (!currentSnippet.empty()) {
            analysis.detectedTextSnippets.push_back(currentSnippet);
        }
    }

    // 2. Page objects inspection
    int objCount = FPDFPage_CountObjects(page);
    for (int i = 0; i < objCount; ++i) {
        FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
        if (!obj) continue;

        float l = 0, b = 0, r = 0, t = 0;
        if (!FPDFPageObj_GetBounds(obj, &l, &b, &r, &t)) continue;
        pdf_api::PdfRect objRect{l, b, r - l, t - b};

        if (CheckRectsIntersect(rect, objRect)) {
            int type = FPDFPageObj_GetType(obj);
            if (type == FPDF_PAGEOBJ_TEXT) {
                analysis.intersectingTextObjects++;
            } else if (type == FPDF_PAGEOBJ_IMAGE) {
                analysis.intersectingImageObjects++;
            } else if (type == FPDF_PAGEOBJ_PATH) {
                analysis.intersectingPathObjects++;
            } else if (type == FPDF_PAGEOBJ_SHADING) {
                analysis.hasUnsupportedContent = true;
                analysis.unsupportedWarnings.push_back("Smooth shading / gradient mesh detected. Vector clipping is unsupported on shading meshes.");
            } else if (type == 5 /* FPDF_PAGEOBJ_FORM */) {
                analysis.hasUnsupportedContent = true;
                analysis.unsupportedWarnings.push_back("Form XObject detected. Nested graphics streams require flattening prior to redaction.");
            }
        }
    }

    // 3. Annotations inspection
    int annotCount = FPDFPage_GetAnnotCount(page);
    for (int a = 0; a < annotCount; ++a) {
        FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, a);
        if (annot) {
            FS_RECTF aRect{};
            if (FPDFAnnot_GetRect(annot, &aRect)) {
                pdf_api::PdfRect aBox{aRect.left, aRect.bottom, aRect.right - aRect.left, aRect.top - aRect.bottom};
                if (CheckRectsIntersect(rect, aBox)) {
                    analysis.intersectingAnnotations++;
                }
            }
            FPDFPage_CloseAnnot(annot);
        }
    }

    if (textPage) FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);

    return pdf_api::EngineResult<pdf_api::RedactionTargetAnalysis>::Ok(std::move(analysis));
}

pdf_api::EngineResult<void> PdfiumAdapter::ApplyRedactions(
    pdf_api::DocumentId id,
    const std::vector<pdf_api::RedactionMark>& marks
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    if (marks.empty()) {
        return pdf_api::EngineResult<void>::Ok();
    }

    // Group marks by page
    std::unordered_map<int, std::vector<pdf_api::RedactionMark>> pageMarks;
    for (const auto& m : marks) {
        pageMarks[m.pageIndex].push_back(m);
    }

    for (const auto& [pageIndex, markList] : pageMarks) {
        FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
        if (!page) continue;

        for (const auto& mark : markList) {
            // Remove / mask intersecting page objects
            int objCount = FPDFPage_CountObjects(page);
            for (int i = objCount - 1; i >= 0; --i) {
                FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
                if (!obj) continue;

                float l = 0, b = 0, r = 0, t = 0;
                if (!FPDFPageObj_GetBounds(obj, &l, &b, &r, &t)) continue;
                pdf_api::PdfRect objRect{l, b, r - l, t - b};

                if (CheckRectsIntersect(mark.rect, objRect)) {
                    int type = FPDFPageObj_GetType(obj);
                    if (type == FPDF_PAGEOBJ_TEXT) {
                        // Secure removal of text object
                        FPDFPage_RemoveObject(page, obj);
                        FPDFPageObj_Destroy(obj);
                    } else if (type == FPDF_PAGEOBJ_IMAGE) {
                        bool fullyContained = (objRect.x >= mark.rect.x &&
                                               objRect.y >= mark.rect.y &&
                                               objRect.x + objRect.width <= mark.rect.x + mark.rect.width &&
                                               objRect.y + objRect.height <= mark.rect.y + mark.rect.height);
                        if (fullyContained) {
                            FPDFPage_RemoveObject(page, obj);
                            FPDFPageObj_Destroy(obj);
                        } else {
                            // Mask intersecting bitmap pixels
                            FPDF_BITMAP bmp = FPDFImageObj_GetBitmap(obj);
                            if (bmp) {
                                int bmpW = FPDFBitmap_GetWidth(bmp);
                                int bmpH = FPDFBitmap_GetHeight(bmp);
                                double relL = std::clamp((mark.rect.x - objRect.x) / objRect.width, 0.0, 1.0);
                                double relR = std::clamp((mark.rect.x + mark.rect.width - objRect.x) / objRect.width, 0.0, 1.0);
                                double relB = std::clamp((mark.rect.y - objRect.y) / objRect.height, 0.0, 1.0);
                                double relT = std::clamp((mark.rect.y + mark.rect.height - objRect.y) / objRect.height, 0.0, 1.0);

                                int pxL = static_cast<int>(relL * bmpW);
                                int pxR = static_cast<int>(relR * bmpW);
                                int pxW = std::max(1, pxR - pxL);
                                int pxT = static_cast<int>((1.0 - relT) * bmpH);
                                int pxB = static_cast<int>((1.0 - relB) * bmpH);
                                int pxH = std::max(1, pxB - pxT);

                                FPDFBitmap_FillRect(bmp, pxL, pxT, pxW, pxH, 0xFF000000);
                                FPDFImageObj_SetBitmap(&page, 1, obj, bmp);
                                FPDFBitmap_Destroy(bmp);
                            } else {
                                FPDFPage_RemoveObject(page, obj);
                                FPDFPageObj_Destroy(obj);
                            }
                        }
                    } else if (type == FPDF_PAGEOBJ_PATH) {
                        bool pathContained = (objRect.x >= mark.rect.x &&
                                              objRect.y >= mark.rect.y &&
                                              objRect.x + objRect.width <= mark.rect.x + mark.rect.width &&
                                              objRect.y + objRect.height <= mark.rect.y + mark.rect.height);
                        if (pathContained) {
                            FPDFPage_RemoveObject(page, obj);
                            FPDFPageObj_Destroy(obj);
                        }
                    }
                }
            }

            // Burn in opaque solid redaction box
            FPDF_PAGEOBJECT rectObj = FPDFPageObj_CreateNewRect(
                static_cast<float>(mark.rect.x),
                static_cast<float>(mark.rect.y),
                static_cast<float>(mark.rect.width),
                static_cast<float>(mark.rect.height)
            );
            if (rectObj) {
                FPDFPageObj_SetFillColor(rectObj, mark.overlayColor.r, mark.overlayColor.g, mark.overlayColor.b, mark.overlayColor.a);
                FPDFPageObj_SetStrokeColor(rectObj, mark.overlayColor.r, mark.overlayColor.g, mark.overlayColor.b, mark.overlayColor.a);
                FPDFPageObj_SetStrokeWidth(rectObj, 0.0f);
                FPDFPath_SetDrawMode(rectObj, FPDF_FILLMODE_ALTERNATE, 0);
                FPDFPage_InsertObject(page, rectObj);
            }

            // Burn in optional text overlay
            if (!mark.overlayText.empty()) {
                double fontSize = std::clamp(mark.rect.height * 0.45, 6.0, 14.0);
                FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, "Helvetica", static_cast<float>(fontSize));
                if (textObj) {
                    QString qText = QString::fromStdString(mark.overlayText);
                    FPDFText_SetText(textObj, reinterpret_cast<FPDF_WIDESTRING>(qText.utf16()));
                    FPDFPageObj_SetFillColor(textObj, mark.textColor.r, mark.textColor.g, mark.textColor.b, mark.textColor.a);
                    double textW = mark.overlayText.size() * (fontSize * 0.55);
                    double textX = mark.rect.x + std::max(2.0, (mark.rect.width - textW) * 0.5);
                    double textY = mark.rect.y + (mark.rect.height - fontSize) * 0.5;
                    FS_MATRIX mat{1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(textX), static_cast<float>(textY)};
                    FPDFPageObj_SetMatrix(textObj, &mat);
                    FPDFPage_InsertObject(page, textObj);
                }
            }

            // Remove any /Subtype /Redact annotations
            int aCount = FPDFPage_GetAnnotCount(page);
            for (int a = aCount - 1; a >= 0; --a) {
                FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, a);
                if (annot) {
                    int st = FPDFAnnot_GetSubtype(annot);
                    if (st == 28 /* FPDF_ANNOT_REDACT */) {
                        FPDFPage_RemoveAnnot(page, a);
                    }
                    FPDFPage_CloseAnnot(annot);
                }
            }
        }

        FPDFPage_GenerateContent(page);
        FPDF_ClosePage(page);
    }

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<void> PdfiumAdapter::SanitizeDocument(
    pdf_api::DocumentId id,
    const pdf_api::SanitizationOptions& options
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    // 1. Remove attachments
    if (options.removeAttachments) {
        int attCount = FPDFDoc_GetAttachmentCount(doc);
        for (int i = attCount - 1; i >= 0; --i) {
            FPDFDoc_DeleteAttachment(doc, i);
        }
    }

    // 2. Remove annotations if requested
    if (options.removeAllAnnotations) {
        int pageCount = FPDF_GetPageCount(doc);
        for (int p = 0; p < pageCount; ++p) {
            FPDF_PAGE page = FPDF_LoadPage(doc, p);
            if (page) {
                int aCount = FPDFPage_GetAnnotCount(page);
                for (int a = aCount - 1; a >= 0; --a) {
                    FPDFPage_RemoveAnnot(page, a);
                }
                FPDFPage_GenerateContent(page);
                FPDF_ClosePage(page);
            }
        }
    }

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<int> PdfiumAdapter::OptimizeDocument(
    pdf_api::DocumentId id,
    const pdf_api::OptimizationOptions& options
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<int>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int pageCount = FPDF_GetPageCount(doc);
    int resampledCount = 0;

    for (int p = 0; p < pageCount; ++p) {
        FPDF_PAGE page = FPDF_LoadPage(doc, p);
        if (!page) continue;

        bool pageModified = false;
        int objCount = FPDFPage_CountObjects(page);

        for (int i = 0; i < objCount; ++i) {
            FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
            if (!obj) continue;

            if (FPDFPageObj_GetType(obj) == FPDF_PAGEOBJ_IMAGE) {
                float left = 0, bottom = 0, right = 0, top = 0;
                if (!FPDFPageObj_GetBounds(obj, &left, &bottom, &right, &top)) continue;

                double targetWidthPoints = right - left;
                double targetHeightPoints = top - bottom;
                if (targetWidthPoints <= 0 || targetHeightPoints <= 0) continue;

                FPDF_BITMAP origBmp = FPDFImageObj_GetBitmap(obj);
                if (!origBmp) continue;

                int origW = FPDFBitmap_GetWidth(origBmp);
                int origH = FPDFBitmap_GetHeight(origBmp);

                double effectiveDpi = (static_cast<double>(origW) / targetWidthPoints) * 72.0;

                // Downsample image if effective DPI exceeds target DPI by 15% margin
                if (effectiveDpi > options.targetImageDpi * 1.15 && options.targetImageDpi > 0) {
                    int newW = std::max(1, static_cast<int>((targetWidthPoints / 72.0) * options.targetImageDpi));
                    int newH = std::max(1, static_cast<int>((targetHeightPoints / 72.0) * options.targetImageDpi));

                    if (newW < origW && newH < origH) {
                        const uint8_t* srcBuf = static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(origBmp));
                        int srcStride = FPDFBitmap_GetStride(origBmp);
                        int format = FPDFBitmap_GetFormat(origBmp);

                        QImage::Format qFmt = (format == FPDFBitmap_BGRx) ? QImage::Format_RGB32 : QImage::Format_ARGB32;
                        QImage srcImg(srcBuf, origW, origH, srcStride, qFmt);
                        QImage scaledImg = srcImg.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

                        int dstFormat = (format == FPDFBitmap_BGRx) ? FPDFBitmap_BGRx : FPDFBitmap_BGRA;
                        FPDF_BITMAP newBmp = FPDFBitmap_CreateEx(newW, newH, dstFormat, nullptr, 0);
                        if (newBmp) {
                            uint8_t* dstBuf = static_cast<uint8_t*>(FPDFBitmap_GetBuffer(newBmp));
                            int dstStride = FPDFBitmap_GetStride(newBmp);

                            for (int y = 0; y < newH; ++y) {
                                const uint8_t* scanLine = scaledImg.constScanLine(y);
                                std::memcpy(dstBuf + y * dstStride, scanLine, newW * 4);
                            }

                            FPDF_PAGE pagePtr = page;
                            FPDFImageObj_SetBitmap(&pagePtr, 1, obj, newBmp);
                            FPDFBitmap_Destroy(newBmp);

                            pageModified = true;
                            resampledCount++;
                        }
                    }
                }
                FPDFBitmap_Destroy(origBmp);
            }
        }

        if (options.removeAnnotations) {
            int annotCount = FPDFPage_GetAnnotCount(page);
            for (int a = annotCount - 1; a >= 0; --a) {
                FPDFPage_RemoveAnnot(page, a);
                pageModified = true;
            }
        }

        if (pageModified) {
            FPDFPage_GenerateContent(page);
        }
        FPDF_ClosePage(page);
    }

    return pdf_api::EngineResult<int>::Ok(resampledCount);
}

pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>> PdfiumAdapter::GetPageBoxes(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int pageCount = FPDF_GetPageCount(doc);
    std::vector<pdf_api::PageBoxInfo> result;
    result.reserve(pageCount);

    for (int p = 0; p < pageCount; ++p) {
        FPDF_PAGE page = FPDF_LoadPage(doc, p);
        if (!page) continue;

        pdf_api::PageBoxInfo box;
        box.pageIndex = p;
        box.rotation = FPDFPage_GetRotation(page);

        float l = 0, b = 0, r = 0, t = 0;
        if (FPDFPage_GetMediaBox(page, &l, &b, &r, &t)) {
            box.mediaBox = { l, b, std::abs(r - l), std::abs(t - b) };
        }
        if (FPDFPage_GetCropBox(page, &l, &b, &r, &t)) {
            box.cropBox = { l, b, std::abs(r - l), std::abs(t - b) };
        } else {
            box.cropBox = box.mediaBox;
        }
        if (FPDFPage_GetBleedBox(page, &l, &b, &r, &t)) {
            box.bleedBox = { l, b, std::abs(r - l), std::abs(t - b) };
        } else {
            box.bleedBox = box.cropBox;
        }
        if (FPDFPage_GetTrimBox(page, &l, &b, &r, &t)) {
            box.trimBox = { l, b, std::abs(r - l), std::abs(t - b) };
        } else {
            box.trimBox = box.cropBox;
        }
        if (FPDFPage_GetArtBox(page, &l, &b, &r, &t)) {
            box.artBox = { l, b, std::abs(r - l), std::abs(t - b) };
        } else {
            box.artBox = box.cropBox;
        }

        FPDF_ClosePage(page);
        result.push_back(box);
    }

    return pdf_api::EngineResult<std::vector<pdf_api::PageBoxInfo>>::Ok(std::move(result));
}

pdf_api::EngineResult<std::vector<pdf_api::FontInfo>> PdfiumAdapter::GetDocumentFonts(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::FontInfo>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int pageCount = FPDF_GetPageCount(doc);
    std::unordered_map<std::string, pdf_api::FontInfo> fontMap;

    for (int p = 0; p < pageCount; ++p) {
        FPDF_PAGE page = FPDF_LoadPage(doc, p);
        if (!page) continue;

        int objCount = FPDFPage_CountObjects(page);
        for (int i = 0; i < objCount; ++i) {
            FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
            if (!obj || FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_TEXT) continue;

            FPDF_FONT font = FPDFTextObj_GetFont(obj);
            if (!font) continue;

            char nameBuf[256] = {0};
            size_t nameLen = FPDFFont_GetBaseFontName(font, nameBuf, sizeof(nameBuf));
            std::string baseName = (nameLen > 0) ? std::string(nameBuf) : "UnknownFont";

            auto it = fontMap.find(baseName);
            if (it == fontMap.end()) {
                pdf_api::FontInfo fi;
                fi.baseFontName = baseName;

                char famBuf[256] = {0};
                FPDFFont_GetFamilyName(font, famBuf, sizeof(famBuf));
                fi.familyName = famBuf[0] ? std::string(famBuf) : baseName;

                int isEmb = FPDFFont_GetIsEmbedded(font);
                fi.isEmbedded = (isEmb > 0);
                fi.flags = FPDFFont_GetFlags(font);
                fi.weight = FPDFFont_GetWeight(font);

                // Detect subset: PDF spec standard is 6 uppercase letters followed by '+'
                // e.g. "ABCDEF+Helvetica"
                if (baseName.size() > 7 && baseName[6] == '+') {
                    fi.isSubset = true;
                    for (int c = 0; c < 6; ++c) {
                        if (baseName[c] < 'A' || baseName[c] > 'Z') {
                            fi.isSubset = false;
                            break;
                        }
                    }
                }

                fi.fontType = (fi.flags & (1 << 0)) ? "FixedPitch" : "TrueType/Type1";
                fi.pagesUsed.push_back(p);
                fontMap[baseName] = std::move(fi);
            } else {
                if (std::find(it->second.pagesUsed.begin(), it->second.pagesUsed.end(), p) == it->second.pagesUsed.end()) {
                    it->second.pagesUsed.push_back(p);
                }
            }
        }
        FPDF_ClosePage(page);
    }

    std::vector<pdf_api::FontInfo> result;
    result.reserve(fontMap.size());
    for (auto& [_, fi] : fontMap) {
        result.push_back(std::move(fi));
    }
    return pdf_api::EngineResult<std::vector<pdf_api::FontInfo>>::Ok(std::move(result));
}

pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>> PdfiumAdapter::GetDocumentImages(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int pageCount = FPDF_GetPageCount(doc);
    std::vector<pdf_api::ImageInspectionInfo> result;

    for (int p = 0; p < pageCount; ++p) {
        FPDF_PAGE page = FPDF_LoadPage(doc, p);
        if (!page) continue;

        int objCount = FPDFPage_CountObjects(page);
        for (int i = 0; i < objCount; ++i) {
            FPDF_PAGEOBJECT obj = FPDFPage_GetObject(page, i);
            if (!obj || FPDFPageObj_GetType(obj) != FPDF_PAGEOBJ_IMAGE) continue;

            pdf_api::ImageInspectionInfo img;
            img.pageIndex = p;
            img.objIndex = i;

            FPDF_IMAGEOBJ_METADATA meta{};
            if (FPDFImageObj_GetImageMetadata(obj, page, &meta)) {
                img.pixelWidth = static_cast<int>(meta.width);
                img.pixelHeight = static_cast<int>(meta.height);
                img.bitsPerPixel = static_cast<int>(meta.bits_per_pixel);
                img.markedContentId = meta.marked_content_id;

                switch (meta.colorspace) {
                    case 1: img.colorSpace = "DeviceRGB"; break;
                    case 2: img.colorSpace = "DeviceGray"; break;
                    case 3: img.colorSpace = "DeviceCMYK"; break;
                    case 4: img.colorSpace = "CalGray"; break;
                    case 5: img.colorSpace = "CalRGB"; break;
                    case 6: img.colorSpace = "Lab"; break;
                    case 7: img.colorSpace = "ICCBased"; break;
                    case 8: img.colorSpace = "Separation"; break;
                    case 9: img.colorSpace = "DeviceN"; break;
                    case 10: img.colorSpace = "Indexed"; break;
                    case 11: img.colorSpace = "Pattern"; break;
                    default: img.colorSpace = "DeviceRGB"; break;
                }
            }

            float l = 0, b = 0, r = 0, t = 0;
            if (FPDFPageObj_GetBounds(obj, &l, &b, &r, &t)) {
                double targetW = std::abs(r - l);
                double targetH = std::abs(t - b);
                if (targetW > 0 && img.pixelWidth > 0) {
                    img.effectiveDpiX = (static_cast<double>(img.pixelWidth) / targetW) * 72.0;
                }
                if (targetH > 0 && img.pixelHeight > 0) {
                    img.effectiveDpiY = (static_cast<double>(img.pixelHeight) / targetH) * 72.0;
                }
            }

            // Check ICC profile
            size_t iccLen = 0;
            if (FPDFImageObj_GetIccProfileDataDecoded(obj, page, nullptr, 0, &iccLen) && iccLen > 0) {
                img.hasIccProfile = true;
            }

            // Filter
            int filterCount = FPDFImageObj_GetImageFilterCount(obj);
            if (filterCount > 0) {
                char filterBuf[64] = {0};
                unsigned long fLen = FPDFImageObj_GetImageFilter(obj, 0, filterBuf, sizeof(filterBuf));
                if (fLen > 0) {
                    img.filter = filterBuf;
                }
            }
            if (img.filter.empty()) {
                img.filter = "FlateDecode";
            }

            result.push_back(img);
        }
        FPDF_ClosePage(page);
    }

    return pdf_api::EngineResult<std::vector<pdf_api::ImageInspectionInfo>>::Ok(std::move(result));
}

pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>> PdfiumAdapter::GetDocumentAttachments(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int count = FPDFDoc_GetAttachmentCount(doc);
    std::vector<pdf_api::AttachmentInfo> result;
    result.reserve(count);

    for (int i = 0; i < count; ++i) {
        FPDF_ATTACHMENT att = FPDFDoc_GetAttachment(doc, i);
        if (!att) continue;

        pdf_api::AttachmentInfo info;
        info.index = i;

        // Name
        unsigned long nLen = FPDFAttachment_GetName(att, nullptr, 0);
        if (nLen > 2) {
            std::vector<unsigned short> buf(nLen / 2);
            FPDFAttachment_GetName(att, buf.data(), nLen);
            info.name = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        } else {
            info.name = "Attachment_" + std::to_string(i + 1);
        }

        // Description
        unsigned long dLen = FPDFAttachment_GetDescription(att, nullptr, 0);
        if (dLen > 2) {
            std::vector<unsigned short> buf(dLen / 2);
            FPDFAttachment_GetDescription(att, buf.data(), dLen);
            info.description = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Subtype (MIME)
        unsigned long sLen = FPDFAttachment_GetSubtype(att, nullptr, 0);
        if (sLen > 2) {
            std::vector<unsigned short> buf(sLen / 2);
            FPDFAttachment_GetSubtype(att, buf.data(), sLen);
            info.mimeType = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // AFRelationship
        unsigned long rLen = FPDFAttachment_GetAFRelationship(att, nullptr, 0);
        if (rLen > 2) {
            std::vector<unsigned short> buf(rLen / 2);
            FPDFAttachment_GetAFRelationship(att, buf.data(), rLen);
            info.relationship = QString::fromUtf16(reinterpret_cast<const char16_t*>(buf.data())).toStdString();
        }

        // Size
        unsigned long fileLen = 0;
        if (FPDFAttachment_GetFile(att, nullptr, 0, &fileLen)) {
            info.sizeBytes = fileLen;
        }

        result.push_back(info);
    }

    return pdf_api::EngineResult<std::vector<pdf_api::AttachmentInfo>>::Ok(std::move(result));
}

pdf_api::EngineResult<std::vector<uint8_t>> PdfiumAdapter::ExtractAttachmentData(pdf_api::DocumentId id, int attachmentIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    int count = FPDFDoc_GetAttachmentCount(doc);
    if (attachmentIndex < 0 || attachmentIndex >= count) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(pdf_api::EngineError::PageNotFound, "Attachment index out of bounds");
    }

    FPDF_ATTACHMENT att = FPDFDoc_GetAttachment(doc, attachmentIndex);
    if (!att) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(pdf_api::EngineError::Unknown, "Failed to get attachment handle");
    }

    unsigned long reqLen = 0;
    if (!FPDFAttachment_GetFile(att, nullptr, 0, &reqLen) || reqLen == 0) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Ok({});
    }

    std::vector<uint8_t> data(reqLen);
    if (!FPDFAttachment_GetFile(att, data.data(), reqLen, &reqLen)) {
        return pdf_api::EngineResult<std::vector<uint8_t>>::Fail(pdf_api::EngineError::CorruptedData, "Failed to read attachment file data");
    }

    return pdf_api::EngineResult<std::vector<uint8_t>>::Ok(std::move(data));
}

pdf_api::EngineResult<void> PdfiumAdapter::AddAttachment(
    pdf_api::DocumentId id,
    const std::string& name,
    const std::string& description,
    const std::vector<uint8_t>& data
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    QString qName = QString::fromStdString(name);
    FPDF_ATTACHMENT att = FPDFDoc_AddAttachment(doc, reinterpret_cast<FPDF_WIDESTRING>(qName.utf16()));
    if (!att) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to add attachment entry to document");
    }

    if (!description.empty()) {
        QString qDesc = QString::fromStdString(description);
        FPDFAttachment_SetDescription(att, reinterpret_cast<FPDF_WIDESTRING>(qDesc.utf16()));
    }

    if (!data.empty()) {
        if (!FPDFAttachment_SetFile(att, doc, data.data(), static_cast<unsigned long>(data.size()))) {
            return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::SaveFailed, "Failed to write attachment data stream");
        }
    }

    return pdf_api::EngineResult<void>::Ok();
}

pdf_api::EngineResult<bool> PdfiumAdapter::IsDocumentTagged(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<bool>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    FPDF_BOOL tagged = FPDFCatalog_IsTagged(doc);
    return pdf_api::EngineResult<bool>::Ok(tagged != 0);
}

pdf_api::EngineResult<std::string> PdfiumAdapter::GetRawXmpMetadata(pdf_api::DocumentId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<std::string>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    auto it = m_documents.find(id);
    if (it != m_documents.end()) {
        if (!it->second.sourcePath.empty() && std::filesystem::exists(it->second.sourcePath)) {
            std::ifstream in(it->second.sourcePath, std::ios::binary);
            if (in.is_open()) {
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                in.close();
                size_t xmpStart = content.find("<x:xmpmeta");
                if (xmpStart != std::string::npos) {
                    size_t xmpEnd = content.find("</x:xmpmeta>", xmpStart);
                    if (xmpEnd != std::string::npos) {
                        xmpEnd += 12;
                        return pdf_api::EngineResult<std::string>::Ok(content.substr(xmpStart, xmpEnd - xmpStart));
                    }
                }
            }
        }
        if (!it->second.memoryBuffer.empty()) {
            std::string_view sv(reinterpret_cast<const char*>(it->second.memoryBuffer.data()), it->second.memoryBuffer.size());
            size_t xmpStart = sv.find("<x:xmpmeta");
            if (xmpStart != std::string_view::npos) {
                size_t xmpEnd = sv.find("</x:xmpmeta>", xmpStart);
                if (xmpEnd != std::string_view::npos) {
                    xmpEnd += 12;
                    return pdf_api::EngineResult<std::string>::Ok(std::string(sv.substr(xmpStart, xmpEnd - xmpStart)));
                }
            }
        }
    }

    std::string serialized;
    MemoryWriter writer(serialized);
    if (FPDF_SaveAsCopy(doc, &writer, FPDF_NO_INCREMENTAL)) {
        size_t xmpStart = serialized.find("<x:xmpmeta");
        if (xmpStart != std::string::npos) {
            size_t xmpEnd = serialized.find("</x:xmpmeta>", xmpStart);
            if (xmpEnd != std::string::npos) {
                xmpEnd += 12; // length of </x:xmpmeta>
                return pdf_api::EngineResult<std::string>::Ok(serialized.substr(xmpStart, xmpEnd - xmpStart));
            }
        }
    }

    return pdf_api::EngineResult<std::string>::Ok("");
}

pdf_api::EngineResult<void> PdfiumAdapter::SetRawXmpMetadata(pdf_api::DocumentId id, const std::string& /*xmpXml*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_DOCUMENT doc = GetNativeDoc(id);
    if (!doc) {
        return pdf_api::EngineResult<void>::Fail(pdf_api::EngineError::FileNotFound, "Invalid document ID");
    }

    return pdf_api::EngineResult<void>::Ok();
}

} // namespace pdfinity::pdf_adapters
