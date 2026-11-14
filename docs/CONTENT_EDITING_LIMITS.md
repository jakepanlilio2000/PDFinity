# PDFinity: Page Content Editing Architecture & Capabilities Guide

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Content Editing Guide  
**Last Updated:** 2026-11-14  

---

## 1. Executive Overview

Unlike word processing formats (such as DOCX, ODF, or HTML) which store content as a continuous flow of semantic paragraphs, tables, and sections, **PDF is a static page-description language (ISO 32000-1 / ISO 32000-2)**.

In a PDF file, text, images, and vector graphics are placed at exact physical coordinate offsets `(x, y)` inside a page's `/Contents` stream using low-level drawing operators (`BT`, `ET`, `Tj`, `cm`, `re`, `Do`, etc.).

PDFinity provides **honest, capability-aware content editing**:
- Native modifications are applied directly to the underlying `FPDF_PAGEOBJECT` content streams.
- The interface explicitly distinguishes between **Native Stream Edits**, **Font Reconstructions**, and **Interactive Overlays**.
- Dangerous edits that risk layout corruption, collision, or unrenderable glyphs are detected, flagged, or rejected.

---

## 2. The Three Layers of PDF Document Mutation

To guarantee truth in user representation and compliance with PDF standards, PDFinity strictly partitions document modifications into three architectural tiers:

```
+-----------------------------------------------------------------------------------+
|  TIER 3: INTERACTIVE OVERLAYS (Annotations Layer)                                 |
|  - Highlights, sticky notes, freehand ink, stamps, callouts                       |
|  - Stored in /Annots array detached from page graphics                            |
|  - Zero alteration to underlying text or graphics streams                         |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|  TIER 2: RECONSTRUCTED CONTENT (Standard Font Substitution)                        |
|  - Replacement of subset-font text with standard PDF base-14 fonts (e.g. Helvetica)|
|  - Triggered when original glyph mappings do not support newly typed characters   |
|  - Clearly labeled in Object Inspector with user confirmation                     |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|  TIER 1: NATIVE CONTENT STREAM EDITS (Base Page Objects)                          |
|  - Direct modification of FPDF_PAGEOBJECT in the page content stream              |
|  - Text string replacement, matrix transformation (move/scale), deletion, z-order |
|  - Vector path insertion, raster image XObject insertion                          |
|  - Bakes permanently into /Contents without annotation wrappers                   |
+-----------------------------------------------------------------------------------+
```

### Truth in UI Rule
> [!IMPORTANT]
> **No overlay may ever be presented to the user as a native text edit.**  
> If an edit creates an annotation, the UI indicates `LAYER: Interactive Overlay (Annotation)`. If an edit modifies a base page object, the UI indicates `LAYER: Native Page Content Stream (Base PDF Object)`.

---

## 3. Font Subsets vs. Fully Embedded vs. Standard Fonts

One of the most frequent causes of corrupted PDF edits in traditional editors is the failure to account for **embedded font subsets**.

### A. Embedded Subset Fonts (`ABCDEF+FontName`)
- Most modern PDF generators (InDesign, Word, Chrome, LaTeX) embed only the exact characters used in the document to minimize file size.
- A subset font is identified by a 6-letter uppercase tag followed by a plus sign (e.g., `ABCDEF+Helvetica`, `XQZQZW+TimesNewRoman`).
- **The Problem:** The font file inside the PDF does not contain glyphs or character mappings for letters not present in the original document. If a user types the letter `z` into a subset font that never originally included `z`, the PDF renderer cannot draw it (rendering as a blank space, question mark, or wrong glyph).
- **PDFinity Behavior:**
  1. The Object Inspector scans font names for subset tags (`+`).
  2. If a subset is detected, a prominent amber warning is displayed:
     *`⚠️ Embedded Subset Font: Characters outside the existing subset cannot be rendered natively. Standard font reconstruction (Helvetica) is required.`*
  3. Modifying text in a subset font triggers standard font replacement to ensure character rendering reliability.

### B. Standard 14 Base Fonts
- Standard fonts (`Helvetica`, `Times-Roman`, `Courier`, `Symbol`, `ZapfDingbats`, and their bold/italic variants) are guaranteed to be supported across every PDF-compliant viewer.
- Text inserted with standard fonts (`FPDFText_LoadStandardFont`) is 100% editable without character substitution issues.

### C. Fully Embedded Fonts
- Contain the entire font outline table.
- Editing is supported provided the character code maps to an existing Unicode point.

---

## 4. Why Automatic Text Reflow Does Not Exist in Native PDF

In word processing tools, adding a sentence pushes subsequent paragraphs down automatically. In PDF:
1. Each line or word is positioned independently with absolute coordinates `(x, y)`.
2. There is no concept of a "paragraph" or "line wrap" in the raw content stream.
3. If text length increases from 10 characters to 40 characters, the text extends horizontally across the page, directly **colliding with neighboring columns, images, or right margins**.

### PDFinity Layout Collision Safeguards
To protect document integrity, `ContentEditingService::ValidateLayoutSafety()` performs automated geometric preflight checks before committing any text edit:
1. **Margin Overflow:** Detects whether the expanded bounding box exceeds page physical boundaries (e.g. `x + width > 612 pt`).
2. **Neighboring Collision Detection:** Performs Axis-Aligned Bounding Box (AABB) intersection testing against every other object on the page.
3. If an intersection is detected, a warning modal alerts the user:
   *`Layout Collision: Text expansion causes overlap with object #2 at (320, 700). PDF content stream lacks automatic text reflow. Proceed with native content modification?`*

---

## 5. Z-Order and Object Stacking

Page objects are painted sequentially in the order they appear in the content stream (painter's algorithm):
- Object `0` is painted first (background).
- Object `N-1` is painted last (foreground).

PDFinity provides full z-order manipulation:
- **Bring Forward:** Swaps object `i` with `i + 1`.
- **Send Backward:** Swaps object `i` with `i - 1`.
- **Bring to Front:** Moves object `i` to index `N - 1`.
- **Send to Back:** Moves object `i` to index `0`.

---

## 6. Supported Operations Matrix

| Operation | Native Content Stream | Reconstructed Stream | Overlay Annotation | Reliability & Limitations |
| :--- | :---: | :---: | :---: | :--- |
| **Inspect Objects** | **YES** | N/A | YES | Full inspection of bounds, fonts, matrices, and types. |
| **Insert Text** | **YES** | **YES** | YES | Uses standard PDF fonts (`Helvetica`, `Times`, `Courier`). |
| **Insert Shape** | **YES** | N/A | YES | Native closed paths (`re`, `m`, `l`, `c`) with stroke and fill. |
| **Insert Image** | **YES** | N/A | YES | Native XObject images with matrix scaling and positioning. |
| **Translate / Scale** | **YES** | N/A | YES | Modifies object affine transform matrix (`FPDFPageObj_SetMatrix`). |
| **Delete Object** | **YES** | N/A | YES | Removes object from page stream via `FPDFPage_RemoveObject`. |
| **Edit Subset Text** | NO (Refused) | **YES** | YES | Replaces font with standard font to prevent missing glyphs. |
| **Global Text Reflow** | NO | NO | NO | Not supported by PDF specifications without external document reconstruction. |
| **Z-Order Stacking** | **YES** | N/A | YES | Reorders stream objects via `FPDFPage_InsertObjectAtIndex`. |
