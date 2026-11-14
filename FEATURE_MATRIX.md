# PDFinity: Master Feature and Capability Matrix

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Feature & Capability Matrix  

---

## 1. Feature Tier Classifications

Features are classified into three architectural tiers:
- **Essential (Tier 1):** Foundational capabilities required for a reliable, production-ready offline PDF reader and lightweight organizer.
- **Advanced (Tier 2):** Comprehensive document manipulation, markup studio, interactive AcroForms, OCR, and local document optimization.
- **Professional (Tier 3):** High-assurance security operations, cryptographic digital signatures, irreversible redaction, standards compliance, batch automation, and optional integrations.

---

## 2. Feature Capability Inventory

| Feature ID | Category / Module | Capability Description | Tier | Modern Platforms (Win10/11, Linux, macOS, Android) | Windows 7 SP1 (Legacy Target)* | Status | Verification Test / Evidence |
| :--- | :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **FEAT-ENG-01** | **PDF Engine** | Stable engine abstraction (`IPdfEngine`) and Google PDFium adapter. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-ENG-02** | **PDF Engine** | Document inspection: page count, page dimensions, and metadata extraction. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-ENG-03** | **PDF Engine** | Standard permissions detection (print, copy, edit) and encryption flags. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-ENG-04** | **PDF Engine** | Password-authenticated document opening without password cracking. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-ENG-05** | **PDF Engine** | Corrupt/malformed document resilience and file size/page limits enforcement. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-ENG-06** | **PDF Engine** | Non-destructive atomic copy saving and clean RAII lifecycle cleanup. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_engine.cpp` |
| **FEAT-VW-01** | **Viewer** | Multi-tab document workspace with session persistence. | Essential | **Supported** | **Supported** | **Implemented** | `tests/ui/test_shell_smoke.cpp` |
| **FEAT-VW-02** | **Viewer** | Continuous scroll, single-page, two-page, and book layouts. | Essential | **Supported** | **Supported** | **Implemented** | `tests/ui/test_viewer_stress.cpp` |
| **FEAT-VW-03** | **Viewer** | Arbitrary zoom (10% to 1000%), fit-width, fit-page. | Essential | **Supported** | **Supported** | **Implemented** | `tests/ui/test_viewer_stress.cpp` |
| **FEAT-VW-04** | **Viewer** | Document outline / bookmarks navigation and thumbnail strip. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_bookmarks.cpp` |
| **FEAT-VW-05** | **Viewer** | High-speed text selection, clipboard copy, and text search. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_pdf_search.cpp` |
| **FEAT-VW-06** | **Viewer** | System print dialog integration and print preview. | Essential | **Supported** | **Supported** | **Implemented** | `tests/ui/test_viewer_stress.cpp` |
| **FEAT-PG-01** | **Organizer** | Lossless document merge of multiple PDF files. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_document_assembly.cpp` |
| **FEAT-PG-02** | **Organizer** | Document split by page ranges, fixed count, or bookmarks. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_document_assembly.cpp` |
| **FEAT-PG-03** | **Organizer** | Page extraction, insertion, deletion, reordering, and rotation. | Essential | **Supported** | **Supported** | **Implemented** | `tests/integration/test_document_assembly.cpp` |
| **FEAT-PG-04** | **Organizer** | Visual thumbnail drag-and-drop page organizer. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/ui/test_page_organizer.cpp` |
| **FEAT-AN-01** | **Annotations** | Text markup: highlight, underline, squiggly, strikeout. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_annotation_persistence.cpp` |
| **FEAT-AN-02** | **Annotations** | Sticky notes, callout boxes, text comments. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_annotation_persistence.cpp` |
| **FEAT-AN-03** | **Annotations** | Geometric shapes: rectangles, circles, lines, arrows, polygons. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_annotation_persistence.cpp` |
| **FEAT-AN-04** | **Annotations** | Freehand ink drawing with pressure/smoothing and eraser. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_annotation_persistence.cpp` |
| **FEAT-AN-05** | **Annotations** | Standard and custom stamp creation, placement, and flattening. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_annotation_persistence.cpp` |
| **FEAT-FM-01** | **Forms** | AcroForms field detection: text, checkbox, radio, dropdown. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_form_persistence.cpp` |
| **FEAT-FM-02** | **Forms** | Form filling, auto-calculation, field reset, and save. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_form_persistence.cpp`, `tests/unit/test_form_service.cpp` |
| **FEAT-FM-03** | **Forms** | Permanent AcroForm field flattening to visual content. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_form_persistence.cpp` |
| **FEAT-ED-01** | **Editor** | In-place text block editing where structurally supported. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_content_editing_persistence.cpp`, `tests/unit/test_content_editing_service.cpp` |
| **FEAT-ED-02** | **Editor** | Image insertion, replacement, resizing, and extraction. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/integration/test_content_editing_persistence.cpp`, `tests/ui/test_content_editing_ui.cpp` |
| **FEAT-SG-01** | **Signing** | Visual signature placement and appearance management. | Essential | **Supported** | **Supported** | **Implemented** | `tests/ui/test_signature_ui.cpp` |
| **FEAT-SG-02** | **Signing** | Cryptographic digital signing (PKCS#12, detached PKCS#7). | Professional | **Supported** | **Supported** | **Implemented** | `tests/integration/test_signature_persistence.cpp` |
| **FEAT-SG-03** | **Signing** | Digital signature integrity verification and chain validation. | Professional | **Supported** | **Supported** | **Implemented** | `tests/unit/test_signature_service.cpp` |
| **FEAT-SC-01** | **Security** | Standard password encryption (AES-128 / AES-256) and permissions. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/unit/test_encryption_service.cpp` |
| **FEAT-RD-01** | **Redaction** | Visual redaction marking with preview and annotation tags. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/ui/test_redaction_ui.cpp`, `tests/unit/test_redaction_service.cpp` |
| **FEAT-RD-02** | **Redaction** | Irreversible multi-pass redaction (content stream & object removal). | Professional | **Supported** | **Supported** | **Implemented** | `tests/integration/test_redaction_verification.cpp`, `tests/unit/test_redaction_service.cpp` |
| **FEAT-RD-03** | **Redaction** | Full document sanitization (metadata, attachments, hidden layers). | Professional | **Supported** | **Supported** | **Implemented** | `tests/unit/test_sanitization_service.cpp` |
| **FEAT-OC-01** | **OCR** | Local offline Tesseract OCR text layer generation. | Advanced | **Supported** | **Disabled (No AVX)** | **Implemented** | `tests/unit/test_ocr_service.cpp`, `tests/integration/test_transformation_pipeline.cpp` |
| **FEAT-OP-01** | **Optimization** | Lossless stream compression, downsampling and stream cleanup. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/unit/test_optimization_service.cpp`, `tests/integration/test_transformation_pipeline.cpp` |
| **FEAT-CV-01** | **Conversion** | Image formats (PNG/JPEG/TIFF/BMP) to PDF conversion. | Essential | **Supported** | **Supported** | **Implemented** | `tests/unit/test_conversion_service.cpp`, `tests/integration/test_transformation_pipeline.cpp` |
| **FEAT-CV-02** | **Conversion** | PDF to image extraction, text, and semantic HTML export. | Essential | **Supported** | **Supported** | **Implemented** | `tests/unit/test_conversion_service.cpp`, `tests/integration/test_transformation_pipeline.cpp` |
| **FEAT-DI-01** | **Intelligence** | PDF/A conformance inspection (1b/2b) and standards conversion. | Professional | **Supported** | **Supported** | **Implemented** | `tests/unit/test_pdfa_compliance.cpp`, `tests/ui/test_compliance_ui.cpp` |
| **FEAT-DI-02** | **Intelligence** | Document comparison (pixel visual diff & semantic text alignment). | Advanced | **Supported** | **Supported** | **Implemented** | `tests/unit/test_comparison_service.cpp`, `tests/ui/test_compliance_ui.cpp` |
| **FEAT-DI-03** | **Intelligence** | Preflight inspection, font/box/image audit, and WCAG accessibility report. | Professional | **Supported** | **Supported** | **Implemented** | `tests/unit/test_document_intelligence.cpp`, `tests/unit/test_preflight_service.cpp`, `tests/unit/test_accessibility_service.cpp` |
| **FEAT-BA-01** | **Batch** | Multi-file persistent job queue, operation chains, profiles, and reports. | Advanced | **Supported** | **Supported** | **Implemented** | `tests/unit/test_batch_service.cpp`, `tests/unit/test_batch_profile.cpp`, `tests/ui/test_batch_ui.cpp` |
| **FEAT-MP-01** | **MCP** | Model Context Protocol local stdio server (2024-11-05, approved roots sandbox, consent policies). | Professional | **Supported** | **Disabled** | **Implemented** | `tests/security/test_mcp_security.cpp`, `tests/unit/test_mcp_server.cpp`, `tests/ui/test_mcp_ui.cpp` |
| **FEAT-RC-01** | **Remote** | Secure Remote Controller companion server with PIN pairing, replay defense, rate limits, allowlist, and companion CLI. | Professional | **Supported** | **Disabled** | **Verified** | `tests/security/test_remote_pair.cpp`, `tests/unit/test_remote_server.cpp`, `tests/ui/test_remote_ui.cpp` |
| **FEAT-PK-01** | **Packaging** | Windows Inno Setup & WiX MSI, Linux (.deb, Arch PKGBUILD, .rpm, AppImage, Flatpak), macOS DMG, Android APK. | Essential | **Supported** | **Unsupported** | **Verified** | Packaging scripts & CI workflow |
| **FEAT-PK-02** | **Updates** | Optional manual offline update dialog with air-gapped verification; zero automatic background telemetry. | Essential | **Supported** | **Supported** | **Verified** | `src/ui/UpdateDialog.cpp`, `tests/ui/test_shell_smoke.cpp` |
| **FEAT-CL-01** | **CLI** | Scriptable command-line interface for headless automation (batch, profiles, history). | Essential | **Supported** | **Supported** | **Verified** | `tests/integration/test_batch_cli.cpp`, `PDFinity.exe batch --help` |
| **FEAT-SEC-01**| **Hardening** | Comprehensive fuzzing, malformed parser resilience, path traversal defense, and protocol boundaries. | Professional | **Supported** | **Supported** | **Verified** | `tests/security/test_fuzz_boundaries.cpp` |

*\*Note on Windows 7 SP1 (Legacy Target): Windows 7 SP1 support is an architectural and source-level target maintained via compatible API usage and Qt 6 legacy fallback pathways; physical binary runtime testing is performed on Windows 11 x64 in this environment. Legacy OS verification remains unverified pending dedicated legacy VM/CI validation.*

---

## 3. Maintenance Policy

- Every feature entry is audited and maintained in conjunction with continuous integration pipelines.
- No feature status transitions to `Implemented` without accompanying source code and tests.
- High-assurance security and privacy guarantees (offline-first, zero telemetry, irreversible redaction) are non-negotiable architectural invariants.
