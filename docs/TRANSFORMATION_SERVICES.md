# PDFinity: Document Transformation, OCR, Conversion & Optimization Specification

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Transformation Services Specification  

---

## 1. Architectural Overview

PDFinity implements high-fidelity document transformation services, adhering to the core tenets of offline capability, non-destructive safety, explicit fidelity disclosures, and honest dependency reporting:

```
+-----------------------------------------------------------------------------------------+
|                                    PDFinity Desktop Shell                               |
|   +-----------------------+   +-----------------------------+   +-------------------+   |
|   |   OcrStudioDialog     |   |   ConversionCenterDialog    |   |  OptimizerDialog  |   |
|   +-----------+-----------+   +--------------+--------------+   +---------+---------+   |
+---------------|------------------------------|----------------------------|-------------+
                |                              |                            |
+---------------v------------------------------v----------------------------v-------------+
|                               Domain Services Layer                                     |
|   +-----------------------+   +-----------------------------+   +-------------------+   |
|   |      OcrService       |   |      ConversionService      |   |OptimizationService|   |
|   +-----------+-----------+   +--------------+--------------+   +---------+---------+   |
|               |                              |                            |             |
|               |       +----------------------v----------------------+     |             |
|               +-----> |           TransformationJobQueue            | <---+             |
|                       | (Batch Queue, Progress, Cancellation)       |                   |
|                       +---------------------------------------------+                   |
+-----------------------------------------------------------------------------------------+
                |                              |                            |
+---------------v------------------------------v----------------------------v-------------+
|                        PDF Engine & External Engine Adapters                            |
|   +-----------------------+   +-----------------------------+   +-------------------+   |
|   |   PdfiumAdapter       |   |  External Engine Detectors  |   |SanitizationService|   |
|   |   (Page Editing,      |   |  • Tesseract CLI & tessdata |   | (Metadata Scrub,  |   |
|   |    Render, Sizing)    |   |  • LibreOffice CLI (soffice)|   |  Stream Clean)    |   |
|   +-----------------------+   +-----------------------------+   +-------------------+   |
+-----------------------------------------------------------------------------------------+
```

---

## 2. Optical Character Recognition (OCR) Studio

### 2.1 Searchable Layer Injection Mechanics
PDFinity injects a searchable text layer onto scanned PDF documents without altering or re-rasterizing the underlying visual image:
1. The page image is rendered at high resolution (default 300 DPI) for text recognition.
2. Tesseract OCR analyzes the image to produce word-level bounding boxes (`OcrWordBox`) and confidence metrics via TSV or hOCR streams.
3. For each recognized word, an invisible text object is inserted into the PDF page content stream:
   - Font: Standard Adobe Type 1 Helvetica font.
   - Position: `(rect.x, rect.y)` matching the word's bounding box on the page.
   - Font Size: Calibrated to word bounding box height (`fontSize = rect.height * 0.85`).
   - Color: Completely transparent (`PdfColor{0, 0, 0, 0}`).
4. The content stream is regenerated using `FPDFPage_GenerateContent`.

**Result:** The original scanned image remains 100% visually pristine with zero loss of graphical detail, while document text becomes fully searchable, copyable, and selectable in any PDF viewer.

### 2.2 Dependency Detection & Actionable Guidance
- The system discovers `tesseract.exe` (Windows) or `tesseract` (POSIX) in the system PATH and well-known installation directories.
- If Tesseract is not installed, the UI displays an actionable alert banner guiding the user to download the engine (e.g. from UB-Mannheim on Windows or distribution package managers on Linux/macOS).
- The system never pretends OCR succeeded if the engine is missing. For automated testing in headless CI, a deterministic synthetic pattern recognizer is available when explicitly enabled.

---

## 3. Document Conversion Center

### 3.1 Image to PDF (`ConvertImagesToPdf`)
- Converts any sequence of PNG, JPEG, TIFF, or BMP images into a clean PDF document.
- Sizing presets:
  - **Match Image Size:** Page media box matches image dimensions at native DPI.
  - **Fit A4 / Fit Letter:** Image is scaled proportionally with configurable margins to standard paper dimensions.
- Images are encoded as clean Flate or DCT streams via `FPDFImageObj_SetBitmap`.

### 3.2 PDF to Image Export (`ExportPdfToImages`)
- Renders PDF pages to high-resolution raster images (`QImage`) at configurable DPI (72, 150, 300).
- Supports lossless PNG, compact JPEG, uncompressed BMP, and archival TIFF.

### 3.3 PDF to Text Export (`ExportPdfToText`)
- Extracts all text from target pages into a clean, UTF-8 encoded plain text file with structured page headers (`--- Page N ---`).

### 3.4 PDF to HTML Export (`ExportPdfToHtml`) & Layout Fidelity Policy
- Converts PDF page text into semantic HTML5 paragraphs with CSS page wrappers.
- **Mandatory Fidelity Notice:** PDF is a fixed-geometry print format. HTML flow cannot guarantee pixel-perfect reproduction of complex multi-column layouts, wrapped text, or vector illustrations. All generated HTML files embed an explicit layout fidelity disclaimer banner:
  > *⚠️ Notice on Layout Fidelity: PDF is a fixed-geometry print format. This HTML export converts document text objects into responsive semantic paragraphs. Exact column boundaries, wrapping points, and complex vector art may differ from native PDF rendering.*

### 3.5 External Office Conversion Engine (`ConvertOfficeToPdf`)
- Microsoft Word (`.docx`), Excel (`.xlsx`), and PowerPoint (`.pptx`) documents are converted exclusively through a compatible external engine (LibreOffice CLI `soffice`).
- **Engine Detection:** Discovers LibreOffice in system PATH or standard installation paths (`C:\Program Files\LibreOffice\program\soffice.exe`, `/usr/bin/soffice`).
- **Actionable Rejection:** If LibreOffice is not installed, conversion requests are rejected with `EngineError::DependencyMissing` and clear installation links. The application **never fakes success**.

---

## 4. PDF Optimizer & Stream Compression

### 4.1 Compression Presets
| Preset | Target Image DPI | JPEG Quality | Metadata Cleanup | Annotation Purge | Primary Use Case |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Prepress Lossless** | 300 DPI | 95% | Disabled | Disabled | Archival, commercial printing, maximum fidelity. |
| **Recommended Standard** | 150 DPI | 80% | Enabled | Disabled | Everyday distribution, email attachments, desktop viewing. |
| **Maximum Screen/Web** | 96 DPI | 65% | Enabled | Enabled | Web downloads, low-bandwidth viewing, mobile devices. |
| **Custom** | 50–600 DPI | 30–100% | User-defined | User-defined | Fine-grained publisher tuning. |

### 4.2 Image Downsampling Engine
- Traverses all page graphics objects.
- For each `FPDF_PAGEOBJ_IMAGE`, calculates effective DPI: `effectiveDpi = (pixelWidth / targetWidthPoints) * 72.0`.
- If `effectiveDpi > targetDpi * 1.15`, the image is downsampled using high-quality bilinear filtering (`Qt::SmoothTransformation`).
- Replaces the bitmap stream in-place via `FPDFImageObj_SetBitmap`.

### 4.3 Output Validation & Integrity Gate
- Post-optimization, the output document is reopened in `IPdfEngine`.
- Verifies:
  1. Output file exists and is non-empty.
  2. Document structure and cross-reference tables parse without errors.
  3. Page count matches the original document.
  4. Text extraction succeeds on page 0.
- Calculates and presents exact before/after metrics: original bytes, optimized bytes, space saved, and percentage compression ratio.

---

## 5. Batch Operation & Transformation Job Queue

`TransformationJobQueue` provides a thread-safe, decoupled queue for running batch transformations:
- **Asynchronous Execution:** Supports background task execution without blocking the UI event loop.
- **Progress Callbacks:** Reports `(current, total, message)` throughout conversion loops.
- **Cancellation:** Atomic cancellation tokens allow immediate termination of in-flight batch jobs.
- **Status Tracking:** Tracks `Queued`, `Running`, `Completed`, `Failed`, and `Cancelled` states with error diagnostics.
