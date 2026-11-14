# PDFinity: Standards Validation, Conformance & Document Intelligence Specification

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Standards & Conformance Specification  

---

## 1. Architectural Overview

PDFinity provides comprehensive inspection, comparison, preflight, and standards-aware validation features. The system is engineered around offline operation, reproducible reporting, non-destructive safety, and an uncompromising architectural distinction between **formal standards validation** and **best-effort heuristic inspection**:

```
+-----------------------------------------------------------------------------------------------+
|                                    PDFinity Desktop Shell                                     |
|   +----------------------------+   +------------------------------+   +-------------------+   |
|   | DocumentIntelligenceDialog |   |  ComparisonWorkspaceDialog   |   |PreflightCompliance|   |
|   +--------------+-------------+   +--------------+---------------+   +---------+---------+   |
+------------------|--------------------------------|-----------------------------|-------------+
                   |                                |                             |
+------------------v--------------------------------v-----------------------------v-------------+
|                                     Domain Services Layer                                     |
|   +----------------------------+   +------------------------------+   +-------------------+   |
|   | DocumentIntelligenceService|   |      ComparisonService       |   |  PreflightService |   |
|   +--------------+-------------+   +--------------+---------------+   +---------+---------+   |
|                  |                                |                             |             |
|   +--------------v-------------+                  |               +-------------v---------+   |
|   |   AccessibilityService     |                  |               | PdfAComplianceService |   |
|   | (Tagged PDF / WCAG Rules)  |                  |               | (ISO 19005 Validator) |   |
|   +----------------------------+                  |               +-----------------------+   |
+---------------------------------------------------|-------------------------------------------+
                                                    |
+---------------------------------------------------v-------------------------------------------+
|                                  PDF Engine Abstraction Layer                                 |
|   • PageBoxInfo (Media, Crop, Bleed, Trim, Art)   • FontInfo (Embedded, Subset, Encodings)    |
|   • ImageInspectionInfo (DPI, Colorspace, ICC)    • Attachments (List, Extract, Embed)        |
|   • XMP Packet Stream Inspection & Modification   • Tagged PDF & Structure Tree Roots         |
+-----------------------------------------------------------------------------------------------+
```

---

## 2. Document Intelligence & Structural Inspection

### 2.1 Metadata & XMP Packet Inspection
- Extracts standard Info dictionary entries (`Title`, `Author`, `Subject`, `Keywords`, `Creator`, `Producer`, `CreationDate`, `ModDate`).
- Inspects and displays the raw Extensible Metadata Platform (XMP) XML packet stream (`<x:xmpmeta>`).
- Supports in-place metadata updates and saving to non-destructive output copies.

### 2.2 Font Completeness Audit
- Traverses document page objects and inspects text programs via `FPDFTextObj_GetFont`.
- Detects:
  - Base Font Name and Family Name.
  - Embedding Status (`isEmbedded`): Verifies whether the TrueType, Type 1, or OpenType font program is embedded in the PDF file.
  - Subset Status: Detects standard 6-character uppercase tag followed by `+` (e.g. `ABCDEF+Helvetica`).
  - Font Flags (FixedPitch, Serif, Symbolic, Italic).
  - List of pages where each font is referenced.

### 2.3 Image & Colorspace Inspection
- Inspects raster graphics objects (`FPDF_PAGEOBJ_IMAGE`) via `FPDFImageObj_GetImageMetadata`.
- Reports:
  - Pixel dimensions (`width`, `height`) and Bits Per Pixel (`bits_per_pixel`).
  - Effective horizontal and vertical resolution (`effectiveDpiX`, `effectiveDpiY`) calculated from image pixel dimensions and transformed page bounding box points: `(pixels / points) * 72.0`.
  - Color spaces: DeviceRGB, DeviceCMYK, DeviceGray, CalGray, CalRGB, Lab, ICCBased, Indexed, Separation, DeviceN.
  - ICC Profile presence: Queries decoded ICC stream via `FPDFImageObj_GetIccProfileDataDecoded`.
  - Compression filter identification (e.g. `DCTDecode`, `FlateDecode`).

### 2.4 Page Dimensions & Boxes
- Audits all standard ISO 32000-1 page boundaries per page:
  - **MediaBox:** Defines the physical boundaries of the medium.
  - **CropBox:** Defines the region to which page contents are clipped.
  - **BleedBox:** Defines the clipping region in production print environments.
  - **TrimBox:** Defines intended dimensions of finished page after trimming.
  - **ArtBox:** Defines meaningful content boundary.
- Reports page rotation (`0°`, `90°`, `180°`, `270°`).

### 2.5 Embedded File Attachments & Document Layers
- Lists all embedded file specifications (`FPDFDoc_GetAttachmentCount`, `FPDFAttachment_GetName`, `FPDFAttachment_GetSubtype`, `FPDFAttachment_GetFile`).
- Supports exporting attachments to disk and embedding new file assets into document catalogs.
- Audits Optional Content Groups (`/OCProperties`, `/OCGs`) to detect active document layers.

---

## 3. Document Comparison Engine

`ComparisonService` executes multi-dimensional comparison between two PDF documents:

### 3.1 Synchronized Visual Comparison
- Renders corresponding pages of Document A and Document B at calibrated resolutions (72, 150, 300 DPI).
- Performs pixel-by-pixel differential mapping:
  - Unmodified pixels rendered in muted light grayscale for visual context.
  - Modified/differing pixels highlighted in vibrant red (`#EF4444`).
  - Removed content highlighted in crimson (`#DC2626`).
  - Added content highlighted in emerald green (`#10B981`).
- Computes exact visual difference metrics: total differing pixels and percentage difference over the entire page surface.

### 3.2 Semantic Text Diff Analysis
- Extracts text runs from corresponding pages.
- Compares text line-by-line, classifying each line as:
  - `Unchanged`
  - `Added`
  - `Deleted`
  - `Modified`
- Reports aggregate text delta statistics across the document.

### 3.3 Synchronized Workspace UI
`ComparisonWorkspaceDialog` provides:
- Side-by-Side Synchronized View (scrolling and page flipping locked in unison).
- Visual Difference Overlay View.
- Text Difference Analysis Table.
- Synchronized zoom (25% to 400%) and navigation controls.

---

## 4. Preflight Inspection Engine

`PreflightService` executes automated pre-publishing checks across 5 core risk categories:
1. **Fonts:** Flags unembedded font programs that will cause unpredictable glyph substitution on third-party viewer environments.
2. **Images:** Flags low-resolution raster assets (below configurable DPI thresholds, e.g. 150 DPI) that will appear pixelated in print.
3. **Page Boxes:** Detects invalid, zero-dimension, or inconsistent page boxes (e.g. CropBox exceeding MediaBox).
4. **Encryption:** Flags active document encryption and permission locks.
5. **Metadata:** Flags missing mandatory publication descriptors (Title, Author).

### Categorization & Severities:
- `Error`: Critical structural flaw preventing compliant publishing.
- `Warning`: Potential publishing risk or rendering anomaly.
- `Notice`: Informational optimization suggestion.

---

## 5. ISO 19005 (PDF/A) Conformance Engine

### 5.1 Standards-Based Validation (`PdfAComplianceService::ValidateCompliance`)
Evaluates documents against ISO 19005-1 (PDF/A-1b) and ISO 19005-2 (PDF/A-2b):

| Rule ID | ISO Clause | Specification Requirement | Verification Method |
| :--- | :--- | :--- | :--- |
| **RULE-PDFA-01** | ISO 19005-1: 6.1.4 | Encryption is strictly prohibited. | Verifies `isEncrypted == false`. |
| **RULE-PDFA-02** | ISO 19005-1: 6.3 | All fonts must be embedded. | Audits `isEmbedded` across all referenced fonts. |
| **RULE-PDFA-03** | ISO 19005-1: 6.7 | Catalog XMP metadata must contain valid PDF/A identification schema. | Verifies `<pdfaid:part>` and `<pdfaid:conformance>B</pdfaid:conformance>` in XMP. |
| **RULE-PDFA-04** | ISO 19005-1: 6.2.3 | Device color spaces must be characterized by an OutputIntent. | Verifies presence of `/OutputIntents` dictionary with standard ICC profile. |
| **RULE-PDFA-05** | ISO 19005-1: 6.6.1 | Prohibited interactive elements (JavaScript, Launch Actions, Sound, Movie). | Inspects object streams for executable action dictionaries. |

### 5.2 Automated PDF/A Remediation Converter (`ConvertToPdfA`)
Where structurally viable, transforms existing PDF documents into compliant PDF/A files:
1. Strips prohibited JavaScript, launch actions, and multimedia elements.
2. Formulates and injects a valid XMP identification packet conforming to the target profile (`pdfaid:part=1` or `2`, `pdfaid:conformance=B`).
3. Injects standard sRGB IEC61966-2.1 `/OutputIntent` into the Catalog dictionary.
4. Serializes the updated document and re-validates the output against the target ISO specification.

---

## 6. Accessibility & Tagged PDF Inspection

`AccessibilityService` evaluates technical alignment with WCAG 2.1 and PDF/UA-1 (ISO 14289-1):
1. **Tagged PDF Marker:** Verifies `/MarkInfo << /Marked true >>` in the document catalog.
2. **Logical Structure Tree:** Verifies `/StructTreeRoot` presence and inspects structure elements for headings, paragraphs, and reading order.
3. **Alternative Text:** Verifies whether raster image objects have associated `/Alt` descriptive text or are marked as decorative artifacts.
4. **Document Language:** Verifies `/Lang` specification in Catalog or root metadata.
5. **Document Title:** Verifies descriptive title metadata according to WCAG 2.4.2.

---

## 7. Quality Standards & Legal Disclaimer Architecture

### Standards Verification Requirements:
> *"Do not claim PDF/A, accessibility or legal compliance based only on heuristic checks. Reports are reproducible and distinguish standards validation from best-effort inspection."*

Every report object and dialog in PDFinity embeds explicit architectural disclosures:
- **Standards-Based Validation:** Formal verification of deterministic syntactic and structural clauses defined in published ISO specifications (ISO 19005-1, ISO 19005-2, ISO 32000-1).
- **Heuristic Inspection:** Empirical publishing quality heuristics (image DPI thresholds, layout aesthetics, visual diff margins).
- **Compliance Disclaimer:** Automated software inspection validates technical format prerequisites. It does not constitute a certified legal warranty of compliance with ADA Title III, Section 508, or ISO archival certification without independent third-party accreditation.
