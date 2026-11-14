# PDFinity: User & Operator Guide

**Application Version:** 1.0.0-rc1 (Release Candidate 1 - Production Ready)  
**Target Audience:** Desktop Users, Document Reviewers, Compliance Officers, System Administrators, DevOps Engineers  
**Last Updated:** 2026-11-14  

---

## 1. Overview and Core Philosophy

PDFinity is a high-assurance, offline-first PDF productivity suite built with modern C++20 and Qt 6. PDFinity requires zero network access, emits zero telemetry, and performs all document operations locally.

---

## 2. Desktop Interface Overview

When you launch PDFinity, you are greeted by the clean, theme-aware application shell:

```text
+-----------------------------------------------------------------------------------+
|  PDFinity - [sample_document.pdf]                                       -  o  x   |
+-----------------------------------------------------------------------------------+
|  File   Edit   View   Tools   Help                                                |
+-----------------------------------------------------------------------------------+
|  [Side Panel] | [◀] [ 1 ] / 20 [▶] | [－] 100% [＋] [Fit Width] [Fit Page] |      |
|  [Layout: Continuous ▼] | [🔍 Find] | [🖨 Print...]                               |
+-----------------------------------------------------------------------------------+
| [Thumbnails] [Outline]  |                                                         |
| +---------------------+ |                     PAGE 1                              |
| | [ 1 ] Preview       | |    +-----------------------------------------------+    |
| |                     | |    |                                               |    |
| | [ 2 ] Preview       | |    |       PDFinity Architecture Overview          |    |
| |                     | |    |                                               |    |
| | [ 3 ] Preview       | |    |   High-speed offline reading experience...    |    |
| |                     | |    |                                               |    |
| +---------------------+ |    +-----------------------------------------------+    |
+-----------------------------------------------------------------------------------+
| Viewing: sample_document.pdf  | 100% | Windows 11 (Per-Monitor DPI v2) | OFFLINE  |
+-----------------------------------------------------------------------------------+
```

---

## 3. Viewing and Layout Modes

PDFinity provides three distinct viewing layout modes selectable from the viewer toolbar:

1. **Continuous Scrolling (Default):**
   - Renders all document pages in a continuous vertical column separated by subtle drop shadows.
   - Ideal for reading reports, legal documents, and long books.
2. **Single Page Mode:**
   - Displays a single page centered in the viewport.
   - Use `Page Up` / `Page Down` or `◀` / `▶` to turn pages.
3. **Facing Pages Mode (Two-Page Spread):**
   - Displays pages side-by-side in book-style spreads.
   - Ideal for brochures, magazines, and double-page document spreads.

---

## 4. Zoom and Viewport Controls

- **Arbitrary Zoom:** Zoom seamlessly from **10% to 1000%** using the `＋` and `－` buttons.
- **Fit to Width:** Automatically calculates the scale factor so the active page spans the horizontal viewport with optimal reading margins.
- **Fit to Page:** Scales the document so the entire page fits vertically and horizontally on your screen.
- **Mouse Wheel Zoom:** Hold `Ctrl` while scrolling the mouse wheel up or down for smooth, cursor-centered zooming.
- **Actual Size (100%):** Press `Ctrl + 0` to reset the zoom scale to 100% physical size.

---

## 5. Navigation Sidebar

Click the **Side Panel** button in the toolbar to toggle the left navigation drawer:

### A. Thumbnails Tab
- Renders low-resolution preview cards of each page asynchronously in background threads.
- Clicking any thumbnail immediately jumps the main viewport to that page.
- As you scroll through the document, the active page thumbnail is highlighted automatically.

### B. Outlines / Bookmarks Tab
- Automatically parses and displays hierarchical document bookmarks (TOC).
- Clicking any bookmark node jumps directly to the bookmarked destination and page index.

---

## 6. Text Selection and Search

### A. High-Speed Text Search (`Ctrl + F`)
1. Press `Ctrl + F` or click **🔍 Find** to open the search bar.
2. Type your search query. Search runs asynchronously in a dedicated worker thread, keeping the UI completely responsive.
3. Matching words are highlighted across pages in **translucent yellow**, with the active match highlighted in **orange**.
4. Use `Enter` / `F3` for **Next Match** and `Shift + F3` for **Previous Match**.
5. Toggle **Match Case** for case-sensitive searching, or **Whole Word** to exclude partial word substrings.
6. Press `Esc` to dismiss the search bar and clear highlight overlays.

### B. Text Selection & Copy (`Ctrl + C`)
1. Click and drag your mouse across any text block to draw a rectangular selection.
2. Right-click the selected region and choose **Copy Selected Text** (`Ctrl + C`).
3. If document permissions prohibit copying text, the copy action is gracefully disabled in compliance with PDF security specifications.

---

## 7. Printing and Print Preview

PDFinity integrates natively with the host operating system's printing subsystem:

1. **Print Document (`Ctrl + P`):**
   - Click **🖨 Print...** or press `Ctrl + P`.
   - The native system print dialog opens, allowing you to select physical or virtual printers (e.g. Microsoft Print to PDF), select page ranges, and set print copies.
   - A non-blocking progress dialog displays real-time printing progress with a **Cancel** option.
2. **Print Preview:**
   - Access **File → Print Preview...** to inspect multi-page printer layouts before committing to physical media.
3. **Permissions Enforcement:**
   - If a document has printing restricted by the author, PDFinity alerts you and prevents unauthorized print dispatch.

---

## 8. Session Management & Full-Screen Mode

### A. Automatic Session Restoration
- PDFinity records your active open document tabs, active tab index, page position, and zoom settings upon application exit.
- When you reopen PDFinity, all previously opened documents are restored automatically.
- Session persistence can be toggled in **File → Preferences...**.

### B. Full-Screen Mode (`F11`)
- Press `F11` to enter distraction-free full-screen mode, hiding window borders and chrome.
- Press `F11` or `Esc` to return to normal desktop window mode.

---

## 9. Page Organization and Document Assembly

PDFinity provides lossless page-level restructuring tools under the **Tools** menu:

### A. Visual Page Organizer (`Tools → Organize Pages...` or `Ctrl + Shift + O`)
- **Thumbnail Grid:** Visualizes all document pages with thumbnail previews, page numbers, and rotation badges.
- **Page Manipulation Actions:**
  - **Rotate 90° CW / CCW:** Rotates selected pages in 90-degree increments.
  - **Delete Pages (`Del`):** Removes selected pages from the document model.
  - **Duplicate Pages:** Clones selected pages immediately after their current positions.
  - **Insert Blank Page:** Inserts a standard blank page (Letter or custom dimension) before or after selection.
  - **Move Left / Right:** Reorders pages intuitively within the document structure.
  - **Reverse Order:** Reverses the sequence of selected pages or the entire document.
  - **Extract Selected Pages:** Exports selected pages directly into a new standalone PDF file.
- **Smart Selection:** Select All (`Ctrl+A`), Select None, Select Odd Pages, Select Even Pages, or Select by Page Range.
- **Non-Destructive Safety:** By default, changes are written to a newly chosen destination file (`Save As...`), guaranteeing original document immutability.

### B. Merge Documents (`Tools → Merge Documents...` or `Ctrl + Shift + M`)
- Merge multiple PDF files into a single consolidated document.
- **File List Controls:** Add files, remove files, clear list, or reorder files with **Move Up** and **Move Down**.
- **Per-File Page Ranges:** Specify custom ranges per input file (e.g. File 1: `"1-5"`, File 2: `"all"`, File 3: `"2, 4, 6"`).
- **Integrity Verification:** PDFinity validates the output file and verifies the combined page count before offering to open.

### C. Split Document (`Tools → Split Document...` or `Ctrl + Shift + S`)
- Split the active document using four flexible modes:
  1. **Burst Mode:** Separates every single page into an individual PDF file (`_page_001.pdf`, `_page_002.pdf`, etc.).
  2. **Fixed Page Count:** Chunks the document into sub-documents of *N* pages each.
  3. **Custom Page Ranges:** Splits by comma-separated ranges (e.g. `"1-3, 4-10, 11-"`).
  4. **Bookmark Chapters:** Automatically segments the document at each top-level chapter bookmark.
- Output files are written to a user-selected output directory with customizable filename prefixes.

### D. Human Page Range Syntax
The page range parser supports standard human syntax across all tools:
- Simple list: `1, 3, 5`
- Ranges: `1-5, 8-12`
- Open-ended: `-3` (first 3 pages), `10-` (page 10 to end)
- Descending ranges: `5-1` (extracts in reverse order)
- Keywords: `all`, `*`, `odd`, `even`, `reverse`

---

## 10. Annotation and Markup Studio

PDFinity includes a full-featured, persistent PDF markup studio accessible via the **Annotate** button on the main toolbar or **Tools → Annotation Studio (`Ctrl + Shift + A`)**.

### A. Architectural Philosophy: Non-Destructive Layering
> [!IMPORTANT]
> **Annotations vs. Page Content Edits:**
> Under the ISO 32000-1 (PDF) specification, annotations exist as an interactive metadata overlay layer (`/Annots` array on each page dictionary) separate from the underlying page content stream (`/Contents`).
> - Adding, editing, moving, or deleting annotations **never alters, re-encodes, or degrades** base document text, vector shapes, or embedded raster images.
> - Annotations are saved as standard PDF annotation objects with complete appearance streams (`/AP`), ensuring full visual fidelity and interactive editability across all compliant PDF viewers (Google Chrome, Microsoft Edge, Apple Preview, and Evince).

### B. Annotation Studio Toolbar
Activating Annotation Studio reveals the dedicated markup toolbar:
- **Select / Hand Tool:** Select, reposition, or resize existing annotations on the canvas.
- **Text Markup Tools:**
  - **Highlight:** Semi-transparent color overlay for emphasizing critical text.
  - **Underline:** Precise underline styling beneath text baselines.
  - **Strikeout:** Strikethrough line indicating deleted or obsolete text.
- **Notes & Comments:**
  - **Sticky Note:** Interactive note pin with pop-up comment text, author name, and subject.
  - **Text Box (FreeText):** Directly rendered text box displayed inline on the document canvas.
- **Geometric Shapes:**
  - **Rectangle:** Outline or filled rectangular callouts.
  - **Circle / Ellipse:** Oval highlights and boundary circles.
  - **Line:** Direct point-to-point connector lines.
  - **Arrow:** Directional indicator line with standard arrowhead.
- **Freehand Drawing (Ink):** Smooth, continuous freehand pen strokes for sketching, diagramming, or margin notes.
- **Standard Stamps:** Dropdown selector featuring standard business stamps:
  - `Approved`, `Draft`, `Confidential`, `Final`, `For Public Release`, `Not Approved`, `Departmental`.
- **Styling Controls:**
  - **Color Picker:** Choose stroke and fill colors from palette or custom color picker.
  - **Line Width:** Fine-tune border and stroke thickness (1 pt to 24 pt).
- **History Controls:** Instant **Undo (`Ctrl + Z`)** and **Redo (`Ctrl + Y`)**.

### C. Canvas Interaction & Properties Editing
- **Selection:** Click any annotation on the page to select it. A bounding box with 8 resize handles appears.
- **Moving & Resizing:** Drag the bounding box to reposition; drag corner or edge handles to scale.
- **Properties Dialog:** Double-click any annotation (or select and right-click) to open the **Annotation Properties Dialog**:
  - Adjust stroke color, fill color, and opacity.
  - Modify stroke width and border style.
  - Edit author metadata (`Author`), topic (`Subject`), and comment body (`Contents`).
  - Delete the annotation directly from the dialog.
- **Direct Deletion:** Press `Delete` or `Backspace` while an annotation is selected to remove it.

### D. Multi-Level Undo / Redo Command History
- Every annotation operation (Add, Delete, Move, Resize, Color/Property modification) is encapsulated in an `IAnnotationCommand` and dispatched to the `AnnotationCommandManager`.
- Maintains a bounded 100-step undo/redo stack.
- Press `Ctrl + Z` to undo the previous action; press `Ctrl + Y` (or `Ctrl + Shift + Z`) to redo.

### E. Sidebar Annotations Navigation Panel
Click the **Side Panel** toggle and switch to Tab 3 (**Annotations**):
- Displays a hierarchical tree view of all annotations in the document grouped by page number.
- Each tree item displays:
  - Annotation type icon (Highlight, Note, Shape, Ink, Stamp, FreeText).
  - Author and modification timestamp.
  - Text snippet or comment preview.
- **Click-to-Navigate:** Clicking any annotation item in the tree immediately scrolls the canvas viewport to the page and centers on the selected annotation.

---

## 11. Interactive Forms & Form Inspector

PDFinity provides full support for industry-standard AcroForms (ISO 32000-1) with inspection, editing, input validation, JSON data exchange, reset to defaults, and permanent flattening.

### A. Accessing the Form Inspector
- Click the **📋 Forms** button in the document toolbar, or
- Select **Tools → Form Inspector & Fill...** (`Ctrl + Shift + F`).

### B. Form Inspector Interface
The Form Inspector modal provides a comprehensive split-pane workspace:
- **Left Panel (Field Inventory Table):**
  - Lists every interactive field across all pages with Page number, Field Name, Field Type (Text, Checkbox, Radio, ComboBox, ListBox, PushButton, Signature), and Current Value preview.
  - Selecting any row instantly focuses that field and displays its properties in the right editor panel.
- **Right Panel (Field Editor & Properties):**
  - Displays Field Type, Field Name, Alternate Tooltip (`TU`), and Security Flags (`Read-Only`, `Required`).
  - Context-sensitive controls for interactive modification:
    - **Text Field:** Multi-line text editor with automatic 64KB input length enforcement and control-character sanitization.
    - **CheckBox:** Interactive checkbox toggle updating `/V` and appearance state `/AS`.
    - **RadioButton:** Choice toggle for mutual exclusion button groups.
    - **ComboBox / Dropdown:** Single-selection dropdown showing all document-defined `/Opt` choices.
    - **ListBox:** Multi-line list selector for discrete option picking.
  - Click **Apply Changes** to commit updates directly to the underlying document model and PDF canvas.
- **Header Summary Card:**
  - Real-time statistics reporting Total Fields, Filled Fields, Required Fields, Read-Only Fields, and Completion Percentage (`% Complete`).

### C. Reset to Defaults
- Click **Reset Form** in the inspector footer to restore all fields to their original default values (`/DV`) defined by the document creator.
- Triggers a confirmation dialog to prevent accidental data loss.

### D. JSON Data Export & Import
- **Export Data:** Click **Export JSON...** to serialize all interactive field names, types, values, and states into a clean, human-readable JSON schema. Ideal for offline backups, programmatic data extraction, and form archiving.
- **Import Data:** Click **Import JSON...** to load previously exported or programmatically generated field data. Fields are mapped by name, sanitized against hostile input thresholds, and committed with atomic validation.

### E. Form Flattening (Permanent Conversion)
- When archiving or finalizing signed/completed documents, click **Flatten Form** to permanently merge all form field appearance streams (`FPDFPage_Flatten` / `FLAT_NORMALDISPLAY`) into the static page content.
- Flattening irreversibly eliminates interactive field widgets, safeguarding against tampering while ensuring identical visual rendering across every legacy and modern viewer.

### F. Unsupported Format Disclosure: Legacy XFA Forms
- **XFA Detection:** When opening a PDF containing dynamic or foreground XML Forms Architecture (XFA) streams, PDFinity displays a non-modal amber notification banner above the document canvas.
- **Technical Disclosure:** Clicking **Technical Details** opens a disclosure dialog explaining that legacy XFA is a proprietary format deprecated by ISO 32000-2, and recommends saving as standard AcroForms or flattened static PDF.

---

## 12. Native Page Content Editing & Object Inspection

PDFinity provides capability-aware editing directly within the PDF page content stream (`/Contents`), strictly distinguishing native content manipulation from superficial annotation overlays.

### A. Core Architecture and Layer Distinction
- **Truth in UI:** The user interface clearly distinguishes:
  - **Native Content Stream (Base Object):** Edits modify the physical PDF graphics stream operators directly (`FPDFPageObj_NewTextObj`, `FPDFPageObj_CreateNewRect`, `FPDFPageObj_NewImageObj`).
  - **Overlay Annotations:** Interactive markups residing in the page's `/Annots` array (e.g. FreeText, Highlight).
  - **Reconstruction:** Synthesized representations created when importing or vectorizing external content.
- An immutable status badge (`LAYER: Native Stream`) is displayed prominently on the Content Editing Toolbar and Object Inspector dialog, guaranteeing that no overlay is ever misrepresented as native text.

### B. Object Inspector & Stream Inspector Dialog
Access the Object Inspector via `Ctrl + Shift + E` or `Tools → Object Inspector & Streams...`:
- **Object Inventory Table:** Lists all discrete graphic objects on the page with Index, Object Type (Text, Path/Shape, Image, FormXObject, Shading), Page Space Coordinates `(X, Y)`, Dimensions `(Width x Height)`, and Preview/Font details.
- **Embedded Subset Font Warning Banner:** Detects fonts subsetted during PDF generation (identified by the standard 6-character tag prefix `ABCDEF+...`). Displays an amber warning informing the user that characters outside the embedded glyph dictionary cannot be rendered without font substitution.
- **In-Place Text Modification:** Edit text strings, font size, and fill color for native text objects.
- **Transformation & Alignment:** Translate objects with delta X and delta Y coordinates. Multi-object alignment tools support Left, Center, Right, Top, Middle, and Bottom alignment.
- **Z-Order Manipulation:** Reorder object rendering sequence using **Bring Forward**, **Send Backward**, **Bring to Front**, and **Send to Back**.
- **Layout Safety & Collision Warning:** Prevents accidental page corruption. If an edited text string expands significantly or collides with neighboring objects, a layout collision confirmation dialog warns the user before committing the modification.

### C. Content Editing Toolbar
Toggle the toolbar from the top toolbar or press `Ctrl + Shift + E`:
- **Add Text (`T+`):** Insert native text objects into the content stream at arbitrary page coordinates.
- **Add Rect (`⬜+`):** Insert native vector rectangles with customizable stroke and fill colors.
- **Add Image (`🖼+`):** Insert native raster images (PNG, JPEG, BMP) with automatic scaling and placement.
- **Command Undo/Redo:** Fully integrated 100-step bounded undo/redo history for all content stream operations.
- **Snap to Grid:** Toggle 10-point grid alignment for precise CAD and technical drawing placement.

---

## 13. Digital Signatures & Document Security

PDFinity provides an offline, cryptographic security suite accessible via the **Security** menu:

### A. Visual Signatures vs. Cryptographic Signatures
- **Visual Signatures (`Security → Place Visual Signature...`):**
  - **Drawing / Cursive / Stamp:** Allows drawing a signature with mouse/stylus, typing a name in cursive typography, or importing a scanned signature image.
  - **No Cryptographic Assurance:** Standalone visual signatures are purely graphical stamps. They do **not** bind cryptographic keys or detect tampering. An amber banner explicitly clarifies this limitation in the interface.
- **Digital Signatures (`Security → Digitally Sign Document...` / `Ctrl + Shift + D`):**
  - **ISO 32000-1 Incremental Signing:** Generates an official detached PKCS#7 / CMS signature (`adbe.pkcs7.detached`) over an exact document `/ByteRange`.
  - **Non-Destructive Update:** Appends an incremental update block without modifying or invalidating previous document revisions or earlier signatures.
  - **Certificate Selection:** Choose imported X.509 certificates from your private hardware-backed vault.

### B. Secure Certificate Vault (`Security → Manage Certificates & Vault...`)
- **PKCS#12 Import:** Import personal signing certificates (`.pfx` / `.p12`) protected with a password.
- **DPAPI Encryption:** Private keys and certificate bundles are encrypted via the Windows Data Protection API (`CryptProtectData`) before being saved to `%LOCALAPPDATA%\PDFinity\certs\`. Zero plaintext keys or passwords are ever stored on disk.
- **Certificate Inspection:** View subject, issuer, serial number, validity date range, and SHA-256 thumbprint.

### C. Signature Validation & Audit Reports (`Security → Validate Digital Signatures...` / `Ctrl + Shift + V`)
- **Real-Time Tamper Detection:** Verifies the cryptographic digest over the signed `/ByteRange`. Detects any byte modifications or unauthorized revisions.
- **Post-Signing Revision Detection:** Flags if incremental changes were added after the signature was applied.
- **Human-Readable Audit Report:** Generates a comprehensive validation report displaying signature status, digest integrity, certificate details, and offline trust limitations.

### D. Document Encryption & Permissions (`Security → Encrypt & Set Permissions...`)
- **AES-256 / AES-128 Standard Encryption:** Password-protect documents using industry-standard PDF security handlers.
- **Permissions Bitmask:** Restrict printing, content copying, page assembly, and form filling with an owner password while allowing document viewing with a user password.

---

## 14. Secure Redaction and Document Sanitization

PDFinity provides an irreversible, high-assurance redaction and sanitization studio accessible via `Tools → Redaction & Sanitization Studio...` (`Ctrl + Shift + R`) or the top toolbar:

### A. True Redaction vs. Visual Masking
- **Never Visual Masking:** Naive PDF tools draw black rectangles over sensitive text while leaving the underlying text and image streams completely intact in the file. PDFinity performs **true physical content destruction**:
  - Text objects, character glyphs, and character codes are removed from the page's `/Contents` stream.
  - Image objects are either destroyed or their pixel buffer is permanently overwritten with solid black (`0xFF000000`).
  - An opaque solid rectangle is burned into the page content stream with optional overlay text (`[REDACTED]`, `(b)(4)`, etc.).
  - Rebuilt cleanly without incremental revision updates (`FPDF_NO_INCREMENTAL`), purging all previous versions.
- **Immutable Source Preservation:** Redaction operations require output to a separate destination file. The original source document is never modified in-place and remains bit-for-bit unchanged.

### B. Pre-Apply Inspection & Confirmation Gate
- **Target Analysis:** When you click **Apply Redactions...**, the Pre-Apply Preview dialog displays:
  - Table of targeted pages and coordinate boundaries.
  - Detected text strings and object counts (Text, Image, Path).
  - Explicit warning banner if unsupported content (e.g. nested Form XObjects or smooth shading meshes) is detected.
- **Irreversible Confirmation:** The "Apply Redactions & Save Copy..." button remains disabled until you explicitly check:
  *"I understand that redaction is permanent and irreversible. Underlying content will be destroyed in the output document."*

### C. Document Sanitization Profiles
Access via `Tools → Sanitize Document...`:
1. **Standard Sanitization (Recommended):** Scrubs document information dictionary (`/Title`, `/Author`, `/Subject`, `/Keywords`, `/CreationDate`, `/ModDate`), strips XMP metadata stream, deletes embedded file attachments, and removes executable JavaScript actions.
2. **High-Assurance Clean:** All Standard targets plus strips hidden layers / Optional Content Groups (`/OCProperties`) and removes all annotations/comments.
3. **Redaction Only:** Destroys marked content regions while preserving standard document metadata.

### D. Multi-Method Verification & Residual Risks
- Redacted documents can be independently verified against 5 criteria:
  1. **Text Extraction:** `doc.ExtractPageText(page)` does not return sensitive words.
  2. **Text Search:** `doc.SearchPageText(page, query)` returns 0 results.
  3. **Object Stream Inspection:** Text objects are eliminated from the page object list.
  4. **Raw Binary Stream:** Plaintext search in the output binary finds zero occurrences.
  5. **Rendered Bitmap Inspection:** Pixels at the redacted location render 100% solid black.
- **Residual Risks Disclosed:**
  - *Scanned OCR Bleed:* When redacting scanned raster documents, character edges may bleed outside overly tight boxes. Use generous margins.
  - *Font Metrics Inference:* Surrounding line layout and word spacing can hint at redacted word lengths. Redact whole lines or sentences when confidentiality is critical.

---

## 15. Document Transformation, OCR, Conversion & Optimization

PDFinity provides an offline suite of document transformation tools designed for maximum fidelity, transparent dependency reporting, and non-destructive output preservation:

### A. OCR Studio (Generate Searchable PDF)
Access via `Tools → OCR Studio (Searchable PDF)...` (`Ctrl + Shift + O`):
- **Invisible Searchable Text Layer:** Scanned raster PDF pages are converted into searchable, selectable documents by injecting an invisible text layer (`alpha = 0`) over each recognized word box, matching coordinates and font size while keeping the original page image visually pristine.
- **Tesseract Integration & Language Packs:** PDFinity auto-detects local Tesseract installations and language packs (`eng`, `deu`, `fra`, `spa`, `chi_sim`, etc.).
- **Dependency Honesty:** If Tesseract is not installed on your system, the studio displays clear installation instructions.

### B. Document Conversion Center
Access via `Tools → Document Conversion Center...` (`Ctrl + Shift + C`):
- **Export PDF Pages to Images:** Render PDF pages to high-resolution PNG, JPEG, TIFF, or BMP at 72, 150, or 300 DPI.
- **Export to Plain Text:** Extracts document text into clean, structured UTF-8 text with page delimiters (`--- Page N ---`).
- **Export to Semantic HTML:** Converts document content to semantic HTML5 paragraphs.
  - *Layout Fidelity Notice:* Because PDF is a print-oriented fixed-geometry format, HTML flow cannot guarantee pixel-perfect reproduction of complex multi-column layouts or vector art. All generated HTML files embed an explicit layout fidelity disclaimer banner.
- **Create PDF from Images:** Combine multiple PNG, JPEG, TIFF, or BMP images into a new PDF with custom page sizing (Match Image Size, Fit A4, Fit US Letter) and margins.
- **Convert Office Documents to PDF:**
  - Converts Word (`.docx`), Excel (`.xlsx`), and PowerPoint (`.pptx`) documents using a detected LibreOffice headless engine (`soffice`).
  - **No Fake Success:** If LibreOffice is not installed, the interface displays an informative alert banner with download links and disables conversion until the engine is available.

### C. PDF Optimizer & Stream Compression
Access via `Tools → Optimize & Compress PDF...` (`Ctrl + Shift + Z`):
- **Image Downsampling:** High-resolution embedded images exceeding the target DPI are downsampled using smooth bilinear interpolation (`Qt::SmoothTransformation`) and recompressed.
- **Compression Presets:**
  - *Prepress Archival (300 DPI, Lossless):* Preserves high resolution and metadata for commercial printing.
  - *Recommended Standard (150 DPI):* Optimal balance of visual clarity and file size for everyday sharing.
  - *Screen & Web (96 DPI):* High compression for web publishing and mobile viewing; strips comments and metadata.
  - *Custom Configuration:* Fine-tune target DPI (50–600) and JPEG quality (30–100%).
- **Integrity Validation:** Every optimized PDF is verified by reopening and parsing cross-references to ensure the document remains 100% readable. Before-and-after sizes and compression ratios are reported.

---

## 16. Document Intelligence, Comparison & Standards Compliance

PDFinity provides an offline suite of deep inspection, visual/semantic comparison, preflight validation, and ISO 19005 PDF/A compliance features:

### A. Document Intelligence & Structural Inspector
Access via `Tools → Document Intelligence & Inspector...` (`Ctrl + Shift + I`):
- **Metadata & XMP:** Inspect Dublin Core metadata and raw ISO standard XMP XML packets. Edit Title, Author, Subject, Keywords, Creator, and Producer, writing updates atomically to a preserved destination copy.
- **Fonts & Encodings:** Audit all embedded and unembedded font programs, font types (Type1, TrueType, Type3, Composite CIDFont), subset prefixes (`ABCDEF+`), and encoding matrices.
- **Images & Color Spaces:** Inspect pixel dimensions, color spaces (DeviceRGB, DeviceCMYK, DeviceGray, ICCBased), bits per component, compression filters (DCTDecode, FlateDecode), and effective rendering DPI.
- **Page Dimensions & Boundaries:** View ISO 32000 page bounding boxes (MediaBox, CropBox, BleedBox, TrimBox, ArtBox) per page in points, millimeters, and inches.
- **Attachments & Optional Content Groups (Layers):** Enumerate and extract embedded file specifications (`/EmbeddedFiles`) to disk, embed external attachments, and inspect interactive layer states (`/OCGs`).

### B. Document Comparison Workspace
Access via `Tools → Compare Documents...` (`Ctrl + Shift + K`):
- **Synchronized Dual Viewports:** Compare two revisions side-by-side with synchronized zoom, viewport panning, and page navigation.
- **Pixel-Level Visual Diff:** Render overlay difference maps highlighting visual changes:
  - Additions rendered in green (`#4CAF50`).
  - Deletions/Modifications rendered in red (`#E53935`).
  - Identical pixels rendered in subtle grayscale.
  - Overall visual change percentage computed per page.
- **Semantic Text Difference:** Line-by-line semantic diff of extracted text blocks categorizing lines as Unchanged, Added, or Removed, accompanied by text similarity percentages.

### C. Preflight & ISO 19005 PDF/A Compliance Studio
Access via `Tools → Preflight & PDF/A Compliance Studio...` (`Ctrl + Shift + X`):
- **Deterministic vs. Heuristic Distinction:** PDFinity explicitly distinguishes formal standards checking from best-effort publishing inspection. Automated reports provide prominent disclaimers and cite specific ISO clauses.
- **Preflight Rules:** Audit documents across 5 inspection categories:
  - *Fonts:* Detect unembedded system fonts that risk cross-platform substitution.
  - *Images:* Flag low-resolution images (< 150 DPI) unsuitable for prepress reproduction.
  - *Page Boxes:* Warn on missing or inconsistent BleedBox/TrimBox definitions.
  - *Encryption:* Identify document passwords and restrictive permission flags.
  - *Structure:* Flag untagged content and missing document titles.
- **ISO 19005 PDF/A Validator:** Validate compliance against PDF/A-1b and PDF/A-2b profiles:
  - Clause 6.1.4: Strict prohibition of encryption/passwords.
  - Clause 6.3 / 6.2.11: Mandatory font embedding and subset program integrity.
  - Clause 6.7 / 6.6: Catalog XMP identification schema (`pdfaid:part` & `pdfaid:conformance`).
  - Clause 6.2.3: Device color spaces must be characterized by an `OutputIntent` with standard ICC profile (`sRGB IEC61966-2.1`).
  - Clause 6.6.1: Strict prohibition of JavaScript, launch actions, and multimedia streams.
- **PDF/A Conversion & Normalization:** Injects valid XMP identification schemas, sRGB `OutputIntent` dictionaries, and sanitizes prohibited executable elements.
- **Accessibility & Tagged PDF (WCAG / PDF/UA):** Audits logical structure trees (`/StructTreeRoot`), document language tags (`/Lang`), and identifies images lacking alternative text (`/Alt`), providing actionable remediation advice.

---

## 17. Multi-File Batch Automation & Scriptable CLI

PDFinity provides an enterprise-grade offline batch processing center and a scriptable command-line interface (CLI) for headless document workflows:

### A. Batch Processing Center (GUI)
Access via `Tools → Batch Processing Center...` (`Ctrl + Shift + B`):
- **Queue Management:** Add individual PDF files or entire directories (with optional recursive scanning).
- **Reusable Workflow Profiles:** Select from built-in profiles (PDF/A Archival, Web Optimization, Prepress Optimization, Searchable OCR, Full Archival Pipeline, Image Export, Text Export) or create custom ordered operation chains.
- **Output Rules & Filename Templates:** Customize destination directories and filename patterns using tokens:
  - `{basename}`: Original filename stem.
  - `{date}`: Calendar date in `YYYYMMDD`.
  - `{time}`: Timestamp in `HHMMSS`.
  - `{ext}`: Source extension.
- **Collision Handling Policies:**
  - *Auto-Rename (Default):* Generates non-colliding suffixes (`file_1.pdf`, `file_2.pdf`).
  - *Overwrite Existing:* Atomically replaces target files.
  - *Skip Existing:* Skips existing files and marks them as skipped in the report.
  - *Fail on Existing:* Flags an error and aborts that file.
- **Safe Resume:** When enabled, the engine verifies whether destination outputs already exist and pass `IPdfEngine` structural validation, skipping already completed files on re-run.
- **Bounded Concurrency & Cancellation:** Choose worker thread counts (1 to 8). Pause, Resume, or Cancel batch execution at any point.
- **Execution Reports:** Export comprehensive HTML or JSON summary reports detailing per-file sizes, processing times, and full diagnostic log traces.

### B. Batch Profile Editor
Access via `Tools → Batch Profiles Editor...`:
- Manage, clone, edit, import, and export profile JSON definitions.
- Compose ordered chains of atomic operations: Stream Optimization, OCR, PDF/A conversion, Sanitization, Image Export, and Text Extraction.

### C. Scriptable Command-Line Interface (CLI)
PDFinity executes automated batch tasks headlessly without launching the graphical desktop interface:

```bash
# Execute automated batch workflow
PDFinity.exe batch --profile "builtin_pdfa_archival" --input "C:\Docs" --output "C:\Archive" --recursive --report "C:\Archive\report.html"

# Automated CI/CD Web Optimization with JSON output
PDFinity.exe batch --profile "builtin_web_optimize" --input "docs/*.pdf" --output "dist/web" --collision overwrite --concurrency 4 --json

# Resuming an interrupted batch run
PDFinity.exe batch --profile "builtin_full_archival" --input "D:\Scans" --output "D:\Searchable" --resume

# List available built-in and custom profiles
PDFinity.exe batch-profiles --list

# Inspect past batch execution history
PDFinity.exe batch-history --list
```

---

## 18. Model Context Protocol (MCP) Integration & AI Assistant Automation

PDFinity provides an optional, sandboxed Model Context Protocol (MCP) server adhering to the official MCP specification (protocol version `2024-11-05`, JSON-RPC 2.0). External AI coding and productivity assistants—such as **Claude Code**, **Cursor**, and compliant MCP clients—can inspect document structures, extract text, run preflight checks, verify signatures, optimize files, and generate searchable text.

### A. Core Security & Privacy Guarantees
- **Off-by-Default:** The MCP server is disabled by default. It cannot be triggered remotely or silently without explicit user configuration.
- **Approved Roots Sandbox:** Tools may only read from or write to directories explicitly added to the operator's approved roots list. All relative paths, `..` traversals, and symlinks outside these roots are blocked.
- **Source Immutability:** Write and transformation operations must write to a distinct output path; source documents are never overwritten in-place.
- **Destructive Operations Gating:** Irreversible operations (such as redaction burn-in and form flattening) are gated behind explicit consent policies or the `--allow-destructive` flag.
- **Secret Redaction:** Passwords, private cryptographic keys, and tokens are scrubbed by `LogRedactor` before transmission or logging.

### B. Desktop MCP Manager
Access via `Tools → Model Context Protocol (MCP) Manager...` (`Ctrl + Shift + M`):
- **Enable / Disable Toggle:** Master switch for MCP functionality.
- **Consent Policy:** Configure policy (`Ask for Destructive Operations`, `Trust Approved Roots Only`, `Always Ask`, or `Deny All`).
- **Approved Roots Management:** Add or remove folders permitted for AI document operations.
- **Active Session Status & Revocation:** Monitor connected AI clients, protocol version, and request counters. Click **Revoke Session** to instantly disconnect the client and invalidate session tokens.
- **Real-Time Audit Log:** Review timestamped records of every request, tool name, target path, and outcome status (`SUCCESS`, `BLOCKED`, `DENIED`).

### C. Headless CLI Stdio Server
For AI assistants running on the local machine, launch PDFinity as a stdio MCP server:

```bash
# Launch stdio MCP server for local assistants
PDFinity.exe mcp-server --approved-roots "C:\Users\User\Documents\Reports" "D:\LegalPdfs"

# Permit destructive operations in an automated workflow
PDFinity.exe mcp-server --approved-roots "C:\Workspace" --allow-destructive --consent-policy TrustApprovedRootsOnly

# Display help and options
PDFinity.exe mcp-server --help
```

For complete protocol details, tool schemas, and configuration examples, consult the comprehensive [MCP Operator Guide](MCP_OPERATOR_GUIDE.md).

---

## 19. PDF Remote Controller & Secure Device Companion Service

PDFinity includes a secure host-side Remote Controller service allowing trusted companion devices (tablets, presentation clickers, and workflow runners) to navigate documents, trigger searches, inspect rendering thumbnails, and control automated batch jobs over a local network.

### A. Core Threat Model & Security Guarantees
- **Disabled by Default:** The Remote Controller server is strictly off by default and never listens on ports without explicit user enablement.
- **Local Network Bounded:** Binds exclusively to the loopback interface (`127.0.0.1`) or a user-selected private LAN adapter. Automatic WAN port forwarding (UPnP/NAT-PMP) and public internet bindings are completely blocked.
- **Cryptographic One-Time PIN Pairing:** Device pairing requires mutual operator consent using a 6-digit cryptographically random PIN (`QRandomGenerator::securelySeeded()`) with a 60-second expiration window.
- **Bearer Token Authentication:** Successful pairing issues a unique 256-bit SHA-256 bearer token. Unauthenticated connections are strictly limited to the `pair` handshake.
- **Replay Defense & Rate Limiting:** Each request requires an incrementing nonce and a ±30s timestamp window. Strict rate limits (max 25 requests/sec per client; 5 failed PIN attempts trigger a 5-minute lockout) prevent brute-force attacks.
- **Host Confirmation for Sensitive Actions:** Destructive operations (closing documents, launching batch pipelines, cancelling active jobs) require an interactive confirmation modal on the host desktop before execution.
- **Immediate Revocation & Emergency Disable:** The operator can disconnect active sessions, revoke specific devices, or press the **Emergency Disable** button to instantly unbind the listener and terminate all remote sockets.
- **Zero Shell Strings:** Remote clients interact only with a strict C++ allowlist (`status`, `goto_page`, `next_page`, `prev_page`, `first_page`, `last_page`, `set_zoom`, `search`, `thumbnail`, `job_status`, `job_pause`, `job_resume`, `job_cancel`, `toggle_fullscreen`, `close_document`, `batch_start`, `disconnect`). No arbitrary shell commands or processes can be invoked remotely.

### B. Remote Controller Manager (GUI)
Access via `Tools → Remote Controller Manager...` (`Ctrl + Shift + T`):
- **Service Configuration:** Toggle service enablement, choose network interface, and customize the listening port (default `8443`).
- **Device Pairing Card:** Click **Generate Pairing PIN (60s)...** to display a large monospace 6-digit PIN with a live countdown bar.
- **Authorized Devices Table:** View paired devices, friendly names, pairing dates, and last-seen IPs. Select any device and click **Revoke Selected** to permanently revoke access.
- **Active Session Bar:** Monitor connected client name and IP. Click **Disconnect Active Session** to sever the link, or **Emergency Disable Service** to immediately shut down the server.
- **Operations Audit Log:** Live ring buffer of the last 1,000 remote actions with client identity, command name, execution status (`ALLOWED`, `CONFIRMED`, `DENIED`, `BLOCKED`), and timestamp.

### C. Companion Client CLI
PDFinity can run as a standalone companion client from another terminal or device:

```bash
# Pair a new companion device using host PIN
PDFinity.exe remote-client --host 192.168.1.100 --port 8443 --pin 482910 --client-name "iPad Pro" --command status

# Navigate to page 5 using saved credentials
PDFinity.exe remote-client --host 192.168.1.100 --port 8443 --device-id dev_482910 --token 7a3f... --command goto_page --param page=5

# Advance to next slide / page
PDFinity.exe remote-client --host 192.168.1.100 --port 8443 --device-id dev_482910 --token 7a3f... --command next_page

# Search for a keyword with JSON output
PDFinity.exe remote-client --host 192.168.1.100 --port 8443 --device-id dev_482910 --token 7a3f... --command search --param query="contract" --json
```

For complete protocol details, line-delimited JSON-RPC schemas, and the full threat model, consult the [Remote Controller Protocol & Security Specification](REMOTE_CONTROLLER_SPEC.md).

---

## 20. Master Keyboard Shortcuts Quick Reference

| Action | Shortcut | Category | Description |
| :--- | :---: | :---: | :--- |
| **Open Document** | `Ctrl + O` | File | Open file chooser dialog to mount a PDF. |
| **Print Document** | `Ctrl + P` | File | Open native OS print dialog. |
| **Undo Annotation / Edit** | `Ctrl + Z` | Edit | Undo the last annotation or content edit action. |
| **Redo Annotation / Edit** | `Ctrl + Y` / `Ctrl + Shift + Z` | Edit | Redo the last undone action. |
| **Find in Document** | `Ctrl + F` | View | Open interactive search bar. |
| **Next Match** | `F3` / `Enter` | View | Jump to next search match. |
| **Previous Match** | `Shift + F3` | View | Jump to previous search match. |
| **Close Search** | `Esc` | View | Dismiss search bar. |
| **Copy Selected Text** | `Ctrl + C` | Edit | Copy selected text to system clipboard. |
| **Zoom In** | `Ctrl + Plus` / `Ctrl + Wheel Up` | View | Increase viewport magnification. |
| **Zoom Out** | `Ctrl + Minus` / `Ctrl + Wheel Down` | View | Decrease viewport magnification. |
| **Reset Zoom (100%)**| `Ctrl + 0` | View | Reset viewport zoom to 100%. |
| **Next Page** | `Page Down` / `Down Arrow` | Navigation | Scroll or jump to next page. |
| **Previous Page** | `Page Up` / `Up Arrow` | Navigation | Scroll or jump to previous page. |
| **First Page** | `Home` | Navigation | Jump to page 1. |
| **Last Page** | `End` | Navigation | Jump to final page. |
| **Close Tab** | `Ctrl + W` | Workspace | Close active document tab. |
| **Toggle Full Screen** | `F11` | Workspace | Toggle distraction-free full screen. |
| **Annotation Studio** | `Ctrl + Shift + A` | Tools | Toggle markup & annotation toolbar. |
| **Content Editor** | `Ctrl + Shift + E` | Tools | Toggle native content editing studio toolbar. |
| **Document Intelligence** | `Ctrl + Shift + I` | Tools | Open document intelligence & structural inspector dialog. |
| **Compare Documents** | `Ctrl + Shift + K` | Tools | Open dual-document visual and semantic comparison workspace. |
| **Preflight & PDF/A Studio** | `Ctrl + Shift + X` | Tools | Open preflight inspection, PDF/A compliance, and accessibility studio. |
| **Batch Processing Center** | `Ctrl + Shift + B` | Tools | Open multi-file batch automation and workflow center. |
| **MCP Manager** | `Ctrl + Shift + M` | Tools | Open Model Context Protocol (MCP) server & sandbox manager. |
| **Remote Controller Manager** | `Ctrl + Shift + T` | Tools | Open secure PDF Remote Controller pairing and connection dialog. |
| **Form Inspector** | `Ctrl + Shift + F` | Tools | Open interactive form inspector & filler. |
| **Organize Pages** | `Ctrl + Shift + P` | Tools | Open visual page organizer dialog. |
| **Merge Documents** | `Ctrl + Shift + M` | Tools | Open multi-file document merge dialog. |
| **Split Document** | `Ctrl + Shift + S` | Tools | Open document split dialog. |
| **Digitally Sign** | `Ctrl + Shift + D` | Security | Open digital cryptographic signing dialog. |
| **Validate Signatures** | `Ctrl + Shift + V` | Security | Open digital signature validation dialog. |
| **Redaction Studio** | `Ctrl + Shift + R` | Tools | Toggle secure redaction & sanitization toolbar. |
| **OCR Studio** | `Ctrl + Shift + O` | Tools | Open OCR studio for searchable PDF generation. |
| **Conversion Center** | `Ctrl + Shift + C` | Tools | Open document conversion center. |
| **Optimizer & Compress**| `Ctrl + Shift + Z` | Tools | Open PDF stream optimizer and compression dialog. |
| **Delete Selected** | `Del` / `Backspace` | Edit | Remove selected annotation or content object. |
| **Preferences** | `Ctrl + ,` | File | Open user settings dialog. |
| **Exit Application** | `Ctrl + Q` | File | Safely close all tabs and exit. |

---

## 21. Performance and Resource Management

- **Bounded LRU Memory Cache:** Rendered page bitmaps are held in a bounded memory cache (default 128 MB). When memory limits are reached, the least recently viewed pages are automatically evicted.
- **Asynchronous Tile Rendering:** Viewport rendering tasks are dispatched across background worker threads (`QThreadPool`), ensuring that scrolling and UI interactions remain fluid and 60 FPS even on large documents.
- **High-DPI Scaling:** On 4K / Retina displays, PDFinity renders bitmaps matching the physical device pixel ratio (`devicePixelRatio`), ensuring pin-sharp text and graphics.
- **Streamlined Persistence:** Annotation updates leverage PDFium's selective content regeneration (`FPDFPage_GenerateContent`), serializing updates rapidly without full document re-encoding.

---

## 22. Offline Updates and Integrity Verification

PDFinity enforces an absolute **Offline-First** policy:
- No background network connections are established upon application startup or shutdown.
- No telemetry or user telemetry trackers are embedded.
- To check for software releases, navigate to **Help → Check for Updates...**:
  - The dialog informs you of your current build version (`1.0.0-rc1`) and deployment mode (Installed vs Portable).
  - Clicking **Check Online Releases** opens the official GitHub release page in your default browser.
  - In air-gapped environments, system administrators verify official package releases using cryptographic SHA-256 hashes published with release notes.

---

## 23. Enterprise Deployment and True Portable Mode

### A. Windows Silent Installer Switches
For automated corporate deployment via Microsoft Intune, SCCM, or group policy:

```cmd
# Completely silent enterprise installation with PDF association
PDFinity-1.0.0-rc1-Setup-x64.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /TASKS="fileassoc"

# Silent uninstallation
"%PROGRAMFILES%\PDFinity\unins000.exe" /VERYSILENT /NORESTART
```

### B. True Zero-Footprint Portable Mode
Extract `PDFinity-1.0.0-rc1-windows-x64-portable.zip` to a USB drive or local directory:
- The presence of `portable.dat` automatically isolates all settings, logs, and caches to `<app_directory>\data`.
- Leaves zero traces in `%LOCALAPPDATA%` and writes zero keys to the Windows Registry.
- Ideal for forensic analysis, air-gapped workstations, and non-admin enterprise environments.


