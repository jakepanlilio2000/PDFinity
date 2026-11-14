# PDFinity Secure Redaction & High-Assurance Sanitization Specification

## 1. Executive Summary: True Redaction vs. Visual Masking

In insecure PDF tools, "redaction" is frequently implemented as a naive visual mask: drawing an opaque black rectangle annotation or vector shape on top of the page. This practice is extremely hazardous:
- **Underlying Text Remains Intact:** The text operators (`Tj`, `TJ`) and glyph codes remain in the `/Contents` stream. Anyone can select, copy, search (`Ctrl+F`), or extract the sensitive text using standard tools.
- **Underlying Image Pixels Remain Intact:** Covered image XObjects remain complete within the PDF binary stream and can be easily extracted.
- **Incremental Revision History Exposes Secrets:** Saving with incremental updates preserves earlier, unredacted revisions earlier in the file.

### PDFinity True Redaction Guarantee
PDFinity implements **true irreversible redaction**:
1. **Physical Content Stream Destruction:** All intersecting text objects, character runs, and vector paths within the redaction boundary are physically excised from the page's `/Contents` stream using PDFium engine-level manipulation (`FPDFPage_RemoveObject`, `FPDFPageObj_Destroy`, `FPDFPage_GenerateContent`).
2. **Raster Pixel Overwriting:** Targeted raster image objects are either completely purged or their intersecting pixel coordinates are permanently overwritten with solid black in the bitmap memory buffer (`FPDFBitmap_FillRect`) before committing back to the image XObject (`FPDFImageObj_SetBitmap`).
3. **Burn-in Opaque Overlay:** An opaque solid rectangle is inserted into the content stream as a replacement native path object, optionally accompanied by an overlay text marker (e.g. `[REDACTED]` or exemption code `(b)(4)`).
4. **Clean Serialization Rebuild:** Documents are saved cleanly without incremental revision updates (`FPDF_NO_INCREMENTAL`), guaranteeing that deleted objects and previous revisions are completely eliminated from the output binary.
5. **Immutable Source Preservation:** The source document is never modified in-place. All redaction operations require a distinct destination file, leaving the original file bit-for-bit unchanged.

---

## 2. Technical Architecture & Redaction Pipeline

```
[ User Marks Redaction Region (Rect) ]
                 │
                 ▼
[ Pre-Apply Target Analysis ]
   ├── Intersecting Text Characters (FPDFText_GetCharBox)
   ├── Intersecting Image Objects (FPDFPageObj_GetBounds)
   └── Unsupported Object Check (Form XObjects, Shading Meshes)
                 │
                 ▼
[ Pre-Apply Preview & Confirmation Dialog ]
   ├── Displays detected text snippets & object counts
   ├── Alerts if unsupported content is present
   └── Requires explicit confirmation of irreversible destruction
                 │
                 ▼
[ Physical Content Destruction Pass ]
   ├── FPDFPage_RemoveObject + FPDFPageObj_Destroy for text/paths
   ├── FPDFBitmap_FillRect (0xFF000000) for raster images
   ├── FPDFPageObj_CreateNewRect (burned-in opaque overlay)
   ├── Optional centered native text overlay ([REDACTED])
   └── FPDFPage_RemoveAnnot (purges temporary /Redact annotations)
                 │
                 ▼
[ Content Stream Regeneration ]
   └── FPDFPage_GenerateContent (rewrites page /Contents)
                 │
                 ▼
[ Sanitization Pass ]
   ├── Info Dictionary Scrubbing (/Title, /Author, /Subject, etc.)
   ├── XMP Metadata Stream Purge (/Metadata in Catalog)
   ├── Embedded Attachments Deletion (FPDFDoc_DeleteAttachment)
   └── JavaScript Action Stripping (/JavaScript, /OpenAction)
                 │
                 ▼
[ Clean File Write & Multi-Method Verification ]
   ├── Write copy without incremental updates (FPDF_NO_INCREMENTAL)
   ├── Verify raw binary stream (forbidden secret string absence)
   ├── Verify engine text extraction (GetPageText)
   ├── Verify engine search (SearchText returns 0)
   └── Generate Verification Audit Report
```

---

## 3. Sanitization Profiles

PDFinity provides three standardized sanitization profiles to balance distribution requirements with document functionality:

| Profile | Target Audience | Metadata Scrubbing | Attachment Removal | JavaScript Removal | Hidden Layer / OCG Removal | All Annotations Removal | Clean Rebuild |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Redaction Only** | Legal & redaction reviews where document metadata must be preserved | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| **Standard Sanitization** *(Recommended)* | Public release, court filings, FOIA releases | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ |
| **High-Assurance Clean** | Air-gapped government, intelligence, classified declassification | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

### Sanitization Elements:
- **Info Dictionary:** Overwrites `/Title`, `/Author`, `/Subject`, `/Keywords`, `/CreationDate`, `/ModDate` with empty strings `()`, and resets `/Creator` and `/Producer` to `(PDFinity)`.
- **XMP Stream:** Eliminates `/Metadata` catalog reference and zeroes all `<x:xmpmeta>...</x:xmpmeta>` blocks.
- **Embedded Files:** Deletes all `/EmbeddedFiles` attachments from the document name tree.
- **JavaScript Actions:** Strips document-level `/OpenAction` scripts and `/JavaScript` entries from names trees.
- **Hidden Layers:** Strips Optional Content Group dictionaries (`/OCProperties`, `/OCGs`).

---

## 4. Unsupported Content Types & Policy

The security policy mandates:
> *"Any unsupported content prevents a success claim and is explicitly disclosed."*

The following PDF object types cannot be cleanly decomposed via standard vector operations:
1. **Form XObjects (`/Subtype /Form`):** Encapsulated, nested graphics streams. Content inside a Form XObject cannot be safely excised without flattening the entire page into base content objects.
2. **Smooth Shading / Gradient Meshes (`/ShadingType`):** Mathematical parametric shading meshes cannot be partially clipped without rasterization.
3. **3D / RichMedia Annotations:** Specialized multimedia streams.

**Policy:** When any unsupported content intersects a marked redaction rectangle:
- The pre-apply analysis flags `hasUnsupportedContent = true`.
- The user is alerted with a prominent warning banner explaining the specific limitation.
- The post-redaction verification report records `isSuccess = false` and lists the specific warnings.
- Complete destruction cannot be claimed until the user flattens the document.

---

## 5. Documented Residual Risks

Even with secure content destruction, operators must be aware of physical and statistical residual risks:
1. **Scanned Page OCR Bleed:** When redacting scanned raster documents, optical character recognition artifacts or character edges may bleed slightly outside tight bounding boxes. Operators should always apply generous margins around sensitive words.
2. **Font Metrics & Word Length Inference:** Because surrounding unredacted text maintains its original formatting and line wrapping, adversaries may occasionally infer the length of an isolated redacted word based on word spacing. To prevent inference, redact complete clauses, lines, or entire paragraphs.
3. **Page Geometry Artifacts:** If a redaction removes an entire paragraph, the resulting white space may indicate that content was removed.

---

## 6. Multi-Method Verification Corpus

Every redaction operation can be verified against the 5 standardized methods:
1. **Text Extraction:** `doc.GetPageText(pageIndex)` does not contain sensitive tokens.
2. **Text Search:** `doc.SearchText(secret)` yields 0 matches.
3. **Object Stream Inspection:** `doc.GetPageObjects(pageIndex)` shows text objects destroyed and opaque path object present.
4. **Binary Stream Inspection:** Raw PDF bytes (`in.read()`) do not contain secret text strings.
5. **Rendered Inspection:** Visual bitmap renders solid black over the redacted area with no underlying pixels visible.
