# PDFinity: PDF Engine Capabilities and Technical Limitations

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Governing Engine:** Google PDFium (Primary Interactive & Rendering Engine)  
**Date:** 2026-11-14  

---

## 1. Primary Engine Overview

PDFinity integrates **Google PDFium** (the battle-tested PDF rasterization and parsing engine maintained by the Chromium project) as its primary engine behind the neutral `IPdfEngine` interface.

PDFium is selected for its exceptional rendering performance, strict memory safety standards enforced through Google ClusterFuzz, multi-platform consistency, and permissive Apache 2.0 / BSD 3-Clause licensing.

---

## 2. Supported Standards and Core Capabilities

| Capability Domain | Supported Standards & Specifications | Implementation Details |
| :--- | :--- | :--- |
| **PDF Standard Versions** | ISO 32000-1 (PDF 1.0 through 1.7), partial ISO 32000-2 (PDF 2.0) | Full support for cross-reference tables, cross-reference streams, object streams, and linearized documents. |
| **Page Geometry & Layout** | `MediaBox`, `CropBox`, `BleedBox`, `TrimBox`, `ArtBox` | Correct handling of coordinate systems (72 points/inch user units) and standard rotations ($0^\circ, 90^\circ, 180^\circ, 270^\circ$). |
| **Font & Text Technologies** | TrueType, OpenType (CFF/TrueType), PostScript Type 1, Type 3, CIDFont Type 0/2 | Embedded font decoding, glyph coordinate hit-testing, Unicode mapping via `/ToUnicode` CMaps. |
| **Color Spaces** | DeviceRGB, DeviceCMYK, DeviceGray, CalRGB, Lab, ICCBased, Indexed, Separation | High-fidelity color conversion to standard 32-bit BGRA/RGBA display bitmaps. |
| **Interactive AcroForms** | Traditional AcroForms (ISO 32000-1 Section 12.7) | Text fields, multi-line text, checkboxes, radio groups, combo boxes, list boxes, push buttons, field appearance generation. |
| **Standard Encryption** | Standard Security Handler (Revisions 2, 3, 4, 5, 6) | 40-bit RC4, 128-bit RC4, AES-128 (PDF 1.6), AES-256 (PDF 1.7 Extension 3 / ISO 32000-1). Password-based authentication. |
| **Standard Annotations** | Text notes, Highlight, Underline, Strikeout, Squiggly, FreeText, Line, Square, Circle, Stamp, Ink | Visual appearance rendering, property queries, coordinate mapping. |
| **Output Integrity** | Atomic Save Protocol | Multi-step serialization to temporary files with atomic swap, preventing corruption of source files. |

---

## 3. Disclosed Engine Limitations and Unsupported Features

In strict accordance with the engineering principle of transparency ("Never claim unsupported functionality"), the following limitations are formally documented:

### 3.1 Dynamic XML Forms Architecture (XFA)
- **Status:** **Unsupported by Design.**
- **Rationale:** XFA is a proprietary Adobe XML-based form specification that was deprecated in ISO 32000-2 (PDF 2.0). XFA introduces massive security risks (remote XML external entity attacks, arbitrary JavaScript execution) and extreme memory overhead.
- **Handling:** When an XFA document is encountered, PDFinity renders the static AcroForm compatibility layer if present. If the document contains only dynamic XFA, an informative warning dialog is presented to the user explaining that dynamic XFA forms require Adobe Acrobat.

### 3.2 Embedded PDF JavaScript & Launch Actions
- **Status:** **Disabled by Default (Hardened Security Invariant).**
- **Rationale:** Embedded PDF JavaScript and `/Launch` actions are the single most prolific attack vector in PDF history (CVE-2009-0927, CVE-2010-0188, CVE-2013-0640, etc.).
- **Handling:** PDFinity suppresses all automatic script execution. Action triggers (`/A`, `/AA`, `/JavaScript`, `/Launch`) are ignored during rendering and navigation.

### 3.3 Proprietary Digital Rights Management (DRM)
- **Status:** **Unsupported.**
- **Rationale:** Documents protected by proprietary vendor DRM schemes (such as Adobe LiveCycle Rights Management, FileOpen, LockLizard, or Azure Information Protection) require proprietary, closed-source client plugins.
- **Handling:** Returns `EngineError::UnsupportedEncryption`. The user is informed that the document is locked by a proprietary vendor DRM system.

### 3.4 3D Models and Legacy Multimedia
- **Status:** **Unsupported.**
- **Rationale:** U3D, PRC, Flash (`/RichMedia`), and legacy QuickTime/AVI annotations are excluded to eliminate memory corruption vectors and unmaintained third-party codecs.
- **Handling:** Viewport displays the 2D poster image or fallback bounding rectangle.

### 3.5 Incomplete or Non-Embedded Fonts
- **Status:** **Graceful Fallback.**
- **Rationale:** PDFs that reference standard 14 fonts or system fonts without embedding font streams rely on host font substitution.
- **Handling:** If a requested font glyph cannot be resolved from the document, PDFium falls back to standard system typography (e.g. Arial / Segoe UI on Windows, DejaVu Sans on Linux). If no suitable glyph exists, a placeholder glyph is drawn.

---

## 4. Resource Ceilings and Protection Invariants

To defend the host system against denial-of-service, algorithmic complexity, and decompression bomb attacks, PDFinity enforces the following hard limits:

| Boundary Parameter | Default Ceiling | Architectural Justification |
| :--- | :--- | :--- |
| **Maximum File Size** | 500 MB (Configurable) | Prevents 32-bit address space exhaustion and system RAM starvation. |
| **Maximum Page Count** | 10,000 pages (Configurable) | Prevents infinite loop attacks in corrupted `/Kids` page tree arrays. |
| **Max Render Resolution** | 10,000 × 10,000 pixels | Guards against integer overflow during bitmap allocation at extreme zoom levels. |
| **Stream Depth Limit** | 32 nested dictionaries | Prevents stack overflow attacks via recursive indirect object chains. |
