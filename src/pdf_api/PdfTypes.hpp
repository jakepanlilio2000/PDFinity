// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace pdfinity::pdf_api {

using DocumentId = uint64_t;
constexpr DocumentId InvalidDocumentId = 0;

enum class EngineError {
    None,
    FileNotFound,
    FileTooLarge,
    PageCountLimitExceeded,
    InvalidFormat,
    CorruptedData,
    PasswordRequired,
    IncorrectPassword,
    UnsupportedEncryption,
    UnsupportedFeature,
    PageNotFound,
    RenderFailed,
    SaveFailed,
    OutOfMemory,
    DependencyMissing,
    Unknown
};

struct PdfPoint {
    double x{0.0};
    double y{0.0};
};

struct PdfRect {
    double x{0.0};
    double y{0.0};
    double width{0.0};
    double height{0.0};
};

struct PdfSize {
    double width{0.0};
    double height{0.0};
};

struct PdfColor {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
};

enum class AnnotationType {
    Unknown,
    Highlight,
    Underline,
    Strikeout,
    Squiggly,
    TextNote,    // Sticky note
    FreehandInk,
    Square,      // Rectangle
    Circle,      // Ellipse
    Line,
    Arrow,
    Stamp,
    FreeText     // Text box / Callout
};

struct AnnotationData {
    int index{-1};                      // 0-based index on page
    int pageIndex{0};                   // 0-based page index
    AnnotationType type{AnnotationType::Unknown};
    PdfRect rect{0.0, 0.0, 0.0, 0.0};   // Bounding box in PDF points
    PdfColor color{255, 255, 0, 128};   // Stroke / highlight color
    PdfColor interiorColor{0, 0, 0, 0}; // Fill color
    float strokeWidth{2.0f};
    std::string contents;               // Text content, comment, or note
    std::string author;                 // Author (T)
    std::string subject;                // Subject (Subj)
    std::string creationDate;
    std::string modificationDate;
    int flags{0};
    bool isVisible{true};
    std::vector<PdfRect> quadPoints;    // Attachment points for text markup
    std::vector<std::vector<PdfPoint>> inkStrokes; // Paths for freehand ink
    std::string stampName{"APPROVED"};  // Standard stamp name
};

enum class FormType {
    None = 0,
    AcroForm = 1,
    XfaFull = 2,
    XfaForeground = 3
};

enum class FormFieldType {
    Unknown = 0,
    PushButton = 1,
    CheckBox = 2,
    RadioButton = 3,
    ComboBox = 4,
    ListBox = 5,
    TextField = 6,
    Signature = 7,
    Xfa = 8
};

struct FormFieldData {
    int pageIndex{0};
    int annotIndex{-1};
    std::string name;
    std::string altName; // Tooltip / alternate description
    FormFieldType type{FormFieldType::Unknown};
    std::string value;
    std::string defaultValue;
    std::string exportValue;
    bool isChecked{false};
    std::vector<std::string> options;
    std::vector<int> selectedIndices;
    bool isReadOnly{false};
    bool isRequired{false};
    int flags{0};
    PdfRect rect{0.0, 0.0, 0.0, 0.0};
};

enum class PageObjectType {
    Unknown = 0,
    Text = 1,
    Path = 2,
    Image = 3,
    Shading = 4,
    FormXObject = 5
};

struct PageObjectData {
    int pageIndex{0};
    int objectIndex{-1};
    PageObjectType type{PageObjectType::Unknown};
    PdfRect bounds{0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> matrix{1.0, 0.0, 0.0, 1.0, 0.0, 0.0};

    // Text properties
    std::string text;
    std::string fontName;
    std::string fontFamily;
    double fontSize{12.0};
    bool isEmbeddedFont{false};
    bool isSubsetFont{false};
    int fontWeight{400};
    int textRenderMode{0};
    PdfColor fillColor{0, 0, 0, 255};
    PdfColor strokeColor{0, 0, 0, 0};

    // Image properties
    int pixelWidth{0};
    int pixelHeight{0};
    int bitsPerComponent{8};
    std::string colorSpace{"DeviceRGB"};

    // Path properties
    double strokeWidth{1.0};
    int pathDrawMode{0};
    int segmentCount{0};

    // Capabilities and safety flags
    bool canEditText{false};
    bool canTransform{true};
    bool canDelete{true};
    std::string fontWarning;
};

enum class ContentEditMode {
    NativeContentEdit = 0,   // Directly alters FPDF_PAGEOBJECT in base content stream
    Reconstruction = 1,      // Content replacement with standard font substitution
    OverlayPlacement = 2     // Interactive Annotation overlay (never native content)
};

struct LayoutCollisionWarning {
    bool hasCollision{false};
    bool hasOverflow{false};
    std::string warningMessage;
    int conflictingObjectIndex{-1};
    PdfRect expandedBounds{0.0, 0.0, 0.0, 0.0};
};

struct DocumentMetadata {
    std::string title;
    std::string author;
    std::string subject;
    std::string keywords;
    std::string creator;
    std::string producer;
    std::string creationDate;
    std::string modificationDate;
    std::string pdfVersion{"1.7"};
};

struct DocumentPermissions {
    bool isEncrypted{false};
    bool canPrint{true};
    bool canPrintHighQuality{true};
    bool canModifyContents{true};
    bool canCopyText{true};
    bool canAnnotate{true};
    bool canFillForms{true};
    bool canExtractAccessibility{true};
    bool canAssemble{true};
    std::string securityHandler;
};

// Signatures & Security
enum class SignatureType {
    VisualAppearanceOnly,
    CryptographicDetachedPkcs7
};

struct VisualSignatureData {
    int pageIndex{0};
    PdfRect rect{100.0, 100.0, 200.0, 80.0};
    std::vector<std::vector<PdfPoint>> inkPaths;
    std::vector<uint8_t> imageBgra;
    int imageWidth{0};
    int imageHeight{0};
    std::string signerName;
    std::string title;
    std::string dateText;
    bool isCryptographicBound{false}; // Strictly false for standalone visual signatures
};

struct CertificateInfo {
    std::string subject;
    std::string issuer;
    std::string serialNumber;
    std::string validFrom;
    std::string validTo;
    bool isExpired{false};
    bool isSelfSigned{false};
    std::string keyAlgorithm{"RSA-2048"};
    std::string thumbprintSha256;
};

struct SignatureValidationResult {
    int signatureIndex{0};
    std::string fieldName;
    std::string filter{"Adobe.PPKLite"};
    std::string subFilter{"adbe.pkcs7.detached"};
    std::string signingTime;
    std::string reason;
    std::string location;
    std::string contactInfo;
    std::vector<int> byteRange;
    bool hasValidDigest{false};
    bool isTampered{false};
    bool hasModificationsAfterSigning{false};
    CertificateInfo certificate;
    bool isCertValid{false};
    bool isChainTrusted{false};
    std::string revocationStatus{"Offline validation: OCSP/CRL revocation checking was skipped (offline mode)."};
    std::string trustSummary;
};

struct DocumentEncryptionConfig {
    std::string userPassword;
    std::string ownerPassword;
    DocumentPermissions permissions;
    std::string algorithm{"AES-256"}; // "AES-256" or "AES-128"
    int keyLengthBits{256};
};

struct CryptographicSignOptions {
    std::string certThumbprint;
    std::string reason;
    std::string location;
    std::string contactInfo;
    bool includeVisualAppearance{false};
    VisualSignatureData visualData;
};

struct RenderOptions {
    double scale{1.0};
    int dpi{96};
    int rotationDegrees{0}; // 0, 90, 180, 270
    bool renderAnnotations{true};
    bool renderFormFields{true};
    bool grayscale{false};
};

struct RenderedImage {
    std::vector<uint8_t> pixels; // BGRA or RGBA
    int width{0};
    int height{0};
    int stride{0};
    bool isBgra{true};
};

struct BookmarkItem {
    std::string title;
    int targetPageIndex{-1};
    std::vector<BookmarkItem> children;
};

struct TextSearchResult {
    int pageIndex{0};
    int charIndex{0};
    int charCount{0};
    std::vector<PdfRect> rects;
};

// Redaction & Sanitization
enum class RedactionStatus {
    Marked,      // Visually identified, not yet applied; underlying content remains in document
    Applied,     // Underlying content permanently removed; burn-in opaque overlay rendered
    Canceled     // Removed before application
};

struct RedactionMark {
    std::string id;
    int pageIndex{0};
    PdfRect rect{0.0, 0.0, 0.0, 0.0};
    PdfColor overlayColor{0, 0, 0, 255};        // Default solid black #000000
    PdfColor markBorderColor{220, 38, 38, 255}; // Red border in marked state (#DC2626)
    std::string overlayText;                    // Optional e.g. "[REDACTED]" or exemption code "(b)(4)"
    PdfColor textColor{255, 255, 255, 255};     // White text on black box
    std::string reason;                         // e.g. "PII", "Confidential", "FOIA Exemption"
    RedactionStatus status{RedactionStatus::Marked};
};

struct RedactionTargetAnalysis {
    int pageIndex{0};
    PdfRect redactionRect{0.0, 0.0, 0.0, 0.0};
    int intersectingTextObjects{0};
    int intersectingImageObjects{0};
    int intersectingPathObjects{0};
    int intersectingAnnotations{0};
    std::vector<std::string> detectedTextSnippets;
    bool hasUnsupportedContent{false};
    std::vector<std::string> unsupportedWarnings;
};

enum class SanitizationProfile {
    RedactionOnly,       // Apply marked redactions only, keep document metadata intact
    StandardSanitize,    // Redactions + scrub core metadata + remove attachments + remove JavaScript
    HighAssuranceClean   // Redactions + scrub all metadata/XMP + attachments + JS + OCG hidden layers + clean rebuild
};

struct SanitizationOptions {
    SanitizationProfile profile{SanitizationProfile::StandardSanitize};
    bool scrubInfoMetadata{true};        // /Title, /Author, /Subject, /Keywords, /Creator, /Producer
    bool scrubXmpMetadata{true};         // /Metadata XMP stream in Catalog
    bool removeAttachments{true};        // Embedded document and page file attachments
    bool removeJavaScript{true};         // Document OpenAction and Name dictionary JavaScript
    bool removeHiddenLayers{false};      // /OCProperties /OCGs
    bool removeAllAnnotations{false};    // Comments, sticky notes, markups
    bool removeFormFields{false};        // Interactive form fields
    bool removeBookmarks{false};         // Outline tree
    bool purgeDeletedContent{true};      // Clean save without incremental revisions
};

struct RedactionVerificationReport {
    bool isSuccess{true};
    int totalRedactionsApplied{0};
    int pagesModified{0};
    int textObjectsRemoved{0};
    int imageObjectsMaskedOrRemoved{0};
    int metadataFieldsScrubbed{0};
    int attachmentsRemoved{0};
    int javaScriptActionsRemoved{0};
    bool hasUnsupportedContent{false};
    std::vector<std::string> warnings;
    std::vector<std::string> residualRisks;
    std::string reportTimestamp;
    std::string sourceFilePath;
    std::string destinationFilePath;
};

enum class PageLayoutMode {
    SinglePage,
    Continuous,
    FacingPages
};

enum class ZoomMode {
    CustomPercent,
    FitWidth,
    FitPage
};

// =============================================================================
// Document Transformation, OCR, Conversion & Optimization Types
// =============================================================================

struct OcrWordBox {
    std::string text;
    PdfRect rect;
    float confidence{1.0f};
};

struct OcrPageResult {
    int pageIndex{0};
    std::vector<OcrWordBox> words;
    std::string fullText;
    float meanConfidence{1.0f};
};

struct OcrOptions {
    std::string language{"eng"};
    int targetDpi{300};
    bool makeSearchablePdf{true};
    bool preserveOriginalImage{true};
    std::vector<int> pageIndices{};
};

struct OcrEngineStatus {
    bool isAvailable{false};
    std::string engineName{"Tesseract OCR"};
    std::string engineVersion;
    std::string executablePath;
    std::vector<std::string> installedLanguages;
    std::string installGuidance;
};

enum class ImagePageFit {
    MatchImageSize,
    FitA4,
    FitLetter
};

struct ImageToPdfOptions {
    ImagePageFit pageFit{ImagePageFit::MatchImageSize};
    double marginPoints{0.0};
    int jpegQuality{90};
};

enum class ExportImageFormat {
    PNG,
    JPEG,
    TIFF,
    BMP
};

struct PdfToImageOptions {
    ExportImageFormat format{ExportImageFormat::PNG};
    int dpi{150};
    int jpegQuality{90};
    std::vector<int> pageIndices{};
};

struct PdfToHtmlOptions {
    bool embedImages{true};
    bool singlePageFile{true};
    std::string title;
};

struct ExternalEngineStatus {
    std::string engineName;
    bool isAvailable{false};
    std::string executablePath;
    std::string version;
    std::string installGuidance;
};

struct OfficeConversionOptions {
    int timeoutSeconds{60};
};

enum class CompressionPreset {
    PrepressLossless,
    RecommendedStandard,
    MaximumScreenWeb,
    Custom
};

struct OptimizationOptions {
    CompressionPreset preset{CompressionPreset::RecommendedStandard};
    int targetImageDpi{150};
    int jpegQuality{80};
    bool stripMetadata{true};
    bool removeAnnotations{false};
    bool cleanUnusedStreams{true};
};

struct OptimizationReport {
    size_t originalSizeBytes{0};
    size_t optimizedSizeBytes{0};
    double compressionRatio{0.0};
    int imagesResampled{0};
    bool isSuccess{false};
    std::string errorMessage;
};

enum class JobStatus {
    Queued,
    Running,
    Completed,
    Failed,
    Cancelled
};

struct JobProgress {
    int current{0};
    int total{0};
    std::string message;
};

// ============================================================================
// Document Intelligence, Standards Validation & Conformance
// ============================================================================

struct PageBoxInfo {
    int pageIndex{0};
    PdfRect mediaBox;
    PdfRect cropBox;
    PdfRect bleedBox;
    PdfRect trimBox;
    PdfRect artBox;
    int rotation{0};
};

struct FontInfo {
    std::string baseFontName;
    std::string familyName;
    bool isEmbedded{false};
    bool isSubset{false};
    int flags{0};
    int weight{400};
    std::string fontType{"TrueType"};
    std::vector<int> pagesUsed;
};

struct ImageInspectionInfo {
    int pageIndex{0};
    int objIndex{0};
    int pixelWidth{0};
    int pixelHeight{0};
    double effectiveDpiX{0.0};
    double effectiveDpiY{0.0};
    int bitsPerPixel{8};
    std::string colorSpace{"DeviceRGB"};
    bool hasIccProfile{false};
    int markedContentId{-1};
    std::string filter;
};

struct AttachmentInfo {
    int index{0};
    std::string name;
    std::string description;
    std::string mimeType;
    size_t sizeBytes{0};
    std::string relationship;
};

struct DocumentIntelligenceReport {
    DocumentMetadata metadata;
    std::string rawXmpPacket;
    std::vector<FontInfo> fonts;
    std::vector<ImageInspectionInfo> images;
    std::vector<PageBoxInfo> pageBoxes;
    std::vector<AttachmentInfo> attachments;
    std::vector<BookmarkItem> bookmarks;
    bool hasLayers{false};
    std::vector<std::string> layerNames;
    bool isTaggedPdf{false};
    std::string language;
};

enum class ComparisonMode {
    SideBySide,
    VisualOverlay,
    TextDiffOnly
};

enum class DiffType {
    Unchanged,
    Added,
    Deleted,
    Modified
};

struct TextDiffLine {
    DiffType type{DiffType::Unchanged};
    int pageIndexA{-1};
    int pageIndexB{-1};
    std::string textA;
    std::string textB;
};

struct PageComparisonResult {
    int pageIndex{0};
    double visualDifferencePercent{0.0};
    int visualDifferencePixelCount{0};
    int addedLinesCount{0};
    int deletedLinesCount{0};
    int modifiedLinesCount{0};
};

struct DocumentComparisonReport {
    std::string pathDocA;
    std::string pathDocB;
    int pageCountA{0};
    int pageCountB{0};
    double overallVisualDifferencePercent{0.0};
    std::vector<PageComparisonResult> pageResults;
    std::vector<TextDiffLine> textDiffs;
    bool isIdentical{false};
};

enum class PreflightSeverity {
    Info,
    Warning,
    Error
};

enum class PreflightCategory {
    Font,
    Image,
    PageBox,
    Encryption,
    Structure,
    Metadata
};

struct PreflightFinding {
    PreflightSeverity severity{PreflightSeverity::Info};
    PreflightCategory category{PreflightCategory::Font};
    int pageIndex{-1}; // -1 for document-wide
    std::string title;
    std::string description;
    std::string remediation;
    bool isHeuristic{false};
};

struct PreflightReport {
    bool isPassed{true};
    int totalErrors{0};
    int totalWarnings{0};
    int totalInfos{0};
    std::vector<PreflightFinding> findings;
    std::string summary;
    std::string disclaimer;
};

enum class PdfAProfile {
    None,
    PdfA1b,
    PdfA2b
};

struct PdfARuleCheck {
    std::string ruleId;
    std::string clause;
    std::string description;
    bool passed{false};
    std::string failureDetail;
};

struct PdfAValidationReport {
    PdfAProfile targetProfile{PdfAProfile::PdfA1b};
    bool isCompliant{false};
    std::string profileClaimed;
    std::vector<PdfARuleCheck> ruleChecks;
    std::string disclaimer;
};

struct PdfAConversionOptions {
    PdfAProfile targetProfile{PdfAProfile::PdfA1b};
    bool embedMissingMetadata{true};
    bool stripProhibitedActions{true};
    bool injectOutputIntent{true};
    std::string defaultLanguage{"en"};
};

struct AccessibilityFinding {
    PreflightSeverity severity{PreflightSeverity::Warning};
    int pageIndex{-1};
    std::string ruleId;
    std::string issue;
    std::string remediation;
};

struct AccessibilityReport {
    bool isTagged{false};
    bool hasStructTree{false};
    bool hasLanguageSpecified{false};
    std::string documentLanguage;
    int totalImagesWithoutAltText{0};
    int totalHeadingsFound{0};
    bool passedBasicAccessibility{false};
    std::vector<AccessibilityFinding> findings;
    std::string legalDisclaimer;
};

template<typename T>
struct EngineResult {
    bool success{false};
    T value{};
    EngineError error{EngineError::None};
    std::string errorMessage;

    [[nodiscard]] bool has_value() const noexcept { return success; }
    [[nodiscard]] explicit operator bool() const noexcept { return success; }

    static EngineResult Ok(T val) {
        EngineResult r;
        r.success = true;
        r.value = std::move(val);
        r.error = EngineError::None;
        return r;
    }

    static EngineResult Fail(EngineError err, std::string msg = "") {
        EngineResult r;
        r.success = false;
        r.error = err;
        r.errorMessage = std::move(msg);
        return r;
    }
};

template<>
struct EngineResult<void> {
    bool success{false};
    EngineError error{EngineError::None};
    std::string errorMessage;

    [[nodiscard]] bool has_value() const noexcept { return success; }
    [[nodiscard]] explicit operator bool() const noexcept { return success; }

    static EngineResult Ok() {
        EngineResult r;
        r.success = true;
        r.error = EngineError::None;
        return r;
    }

    static EngineResult Fail(EngineError err, std::string msg = "") {
        EngineResult r;
        r.success = false;
        r.error = err;
        r.errorMessage = std::move(msg);
        return r;
    }
};

const char* GetEngineErrorMessage(EngineError error);

} // namespace pdfinity::pdf_api
