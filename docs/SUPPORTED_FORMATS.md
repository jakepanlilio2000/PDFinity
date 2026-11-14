# PDFinity: Supported Formats and Form Technologies Matrix

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Supported Formats Specification  

---

## 1. Executive Form Scope & Standards Conformance

PDF documents utilize fundamentally divergent technologies for interactive forms. PDFinity strictly adheres to open international standards (ISO 32000-1 and ISO 32000-2) while maintaining robust, deterministic security boundaries:

```text
+------------------------------------------------------------------------------------------------+
|                                    PDF Form Technologies Scope                                 |
+----------------------------------------------------+-------------------------------------------+
|               STANDARD ACROFORMS                   |       XML FORMS ARCHITECTURE (XFA)        |
|             (ISO 32000-1 / PDF 1.7)                |       (Adobe Proprietary / Deprecated)    |
+----------------------------------------------------+-------------------------------------------+
| - Text Fields (Single/Multi-line) [SUPPORTED]      | - Dynamic XFA (Template + Data) [EXCLUDED]|
| - Check Boxes & Radio Buttons     [SUPPORTED]      |   (Requires Adobe proprietary DOM/Script) |
| - Drop-Down ComboBox & ListBox    [SUPPORTED]      | - Static / Foreground XFA       [PARTIAL] |
| - Appearance Regeneration & Flatten [SUPPORTED]    |   (Renders static pre-drawn visual fallback)|
| - JSON & FDF Data Interchange      [SUPPORTED]     | - Disclosed via Prominent Warning Banner   |
+----------------------------------------------------+-------------------------------------------+
```

---

## 2. Form Format Support Matrix

| Form Technology | Underlying Standard | Security & Sandbox Risk | PDFinity Support Level | Runtime Handling & User Experience |
| :--- | :--- | :--- | :---: | :--- |
| **Standard AcroForms** | ISO 32000-1 (Sec. 12.7) | Low (Bounded native fields) | **Full Support** | Native interactive filling, field inspection, appearance stream regeneration, reset, flattening, and JSON export/import. |
| **Pre-rendered Static XFA** | Adobe XFA 3.3 / PDF 1.7 | Low to Medium | **Partial Support** | Displays pre-rendered visual vector/raster layers; interactive field modification is disabled. Warning banner displayed. |
| **Dynamic XFA** | Adobe LiveCycle XML DOM | **Severe** (Arbitrary XML injection, dynamic reflow, memory corruption) | **Explicitly Excluded** | Disclosed via dismissible warning banner. Users are guided to Adobe Acrobat Pro for proprietary workflows. Deprecated in PDF 2.0. |
| **Hybrid Forms (AcroForm + XFA)** | Dual Catalog `/AcroForm` + `/XFA` | Medium | **AcroForm Fallback** | PDFinity detects the hybrid dictionary and parses standard AcroForm fields, ignoring unsafe XFA XML DOM scripts. |
| **Form Data Interchange (JSON)** | RFC 8259 Schema | Zero (Offline local parsing) | **Full Support** | Fast structured JSON import and export for headless batch workflows and record archival. |
| **Form Flattening** | ISO 32000-1 (Sec. 12.7.4) | Zero (Static vector/raster) | **Full Support** | Permanently converts all interactive `/Widget` annotations into native page content streams (`/Contents`), removing all form fields. |

---

## 3. Supported Field Types and Capabilities

### A. Text Fields (`FPDF_FORMFIELD_TEXTFIELD`)
- **Single-Line & Multi-Line Text:** Full UTF-16 Unicode character set support.
- **Security Validation:**
  - Strict **64 KB size threshold** per field to prevent memory exhaustion and buffer overflows.
  - Automatic stripping of hostile control characters (null bytes `\0`, bell `\x07`, escape `\x1b`) while preserving standard text whitespace (`\t`, `\r`, `\n`).
- **Default Value Support:** Reads `/DV` attribute; restores field upon form reset.

### B. Checkboxes (`FPDF_FORMFIELD_CHECKBOX`)
- **Interactive Toggling:** Supports checked (`true`) and unchecked (`false`) state transitions.
- **Export Values:** Extracts custom export values (e.g. `/Yes`, `/On`, `/Accepted`) from the field dictionary.
- **Appearance Streams:** Synchronizes `/AS` (Appearance State) and `/V` (Value) for 100% third-party viewer compatibility.

### C. Radio Buttons (`FPDF_FORMFIELD_RADIOBUTTON`)
- **Group Mutual Exclusion:** Activating one button in a radio group automatically clears alternative choices in the same control group.
- **Custom Export States:** Binds selection directly to the control's unique export key.

### D. Choice Fields (`FPDF_FORMFIELD_COMBOBOX` & `FPDF_FORMFIELD_LISTBOX`)
- **Drop-down ComboBox:** Displays option list; allows selecting existing options.
- **ListBox:** Inspects and queries option labels and multi-selection indices.

---

## 4. XFA Disclosure & Limitation Architecture

> [!WARNING]
> **Why Dynamic XFA is Unsupported:**
> 1. **Standards Status:** Dynamic XFA was officially **deprecated and removed** from the international PDF standard in ISO 32000-2 (PDF 2.0).
> 2. **Security Architecture:** Dynamic XFA relies on an external XML DOM interpreter with embedded proprietary ECMAScript extensions. Historically, dynamic XFA is responsible for over 65% of critical memory vulnerabilities in Adobe Acrobat.
> 3. **Offline Engine Independence:** Open-source engines (Google PDFium, Apple Quartz, Mozilla PDF.js, Poppler) do not support dynamic XFA scripting. PDFinity maintains a deterministic, high-assurance security posture by strictly executing standard AcroForms.

When an XFA document is opened in PDFinity, the suite automatically detects the `/XFA` packet and surfaces an amber **XfaNoticeBanner**:
- Displays clear technical disclosure regarding the limitation.
- Prevents user data loss from broken interactive scripts or uncommitted calculations.
- Recommends opening in Adobe Acrobat if dynamic LiveCycle workflows are mandatory.

---

## 5. Non-Destructive Persistence Verification

All form field operations adhere to PDFinity's non-destructive data integrity rules:
1. **Atomic File Saving:** Output documents are saved via atomic temporary files (`SaveCopy`), guaranteeing that original files are never corrupted.
2. **Standard Dictionaries:** Value mutations are written to standard PDF `/V` and `/AS` entries and synchronized with PDFium's `FPDFPage_GenerateContent`.
3. **Third-Party Compatibility:** Saved documents are independently verified in Adobe Acrobat, Google Chrome, Microsoft Edge, and Evince.
