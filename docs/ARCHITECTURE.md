# PDFinity: System Architecture and Modular Design Specification

**Document Version:** 1.0.0  
**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official System Architecture Specification  

---

## 1. Architectural Vision and Core Principles

PDFinity is architected as an offline-first, modular, enterprise-grade desktop PDF productivity suite. The system design strictly adheres to the following principles:

1. **Clean Separation of Concerns:** Strict layer boundaries where UI, domain logic, PDF engine adapters, services, platform adapters, and persistence are strictly decoupled.
2. **Third-Party Engine Isolation:** Third-party PDF engines (PDFium, QPDF) are entirely encapsulated behind an internal `IPdfEngine` abstraction. Application code and UI never include third-party headers directly.
3. **Responsive Asynchronous Execution:** The UI thread handles only user interaction and presentation. All document loading, parsing, rendering, page assembly, search indexing, OCR, and export operations execute on background threads via asynchronous job workers with progress reporting and cancellation tokens.
4. **Document Integrity & Atomic Operations:** Documents are never modified or overwritten in-place. All saves are written to secure temporary storage, verified, and atomically replaced.
5. **Local-First & Zero Telemetry:** Core PDF functionality operates entirely without network access. Optional network-facing components (Remote Controller, MCP) are strictly opt-in, disabled by default, and isolated behind dedicated security policies.

---

## 2. Layered Component Architecture

```
+---------------------------------------------------------------------------------------------------+
|                                      PRESENTATION LAYER (ui/)                                     |
|  Shell Window | Multi-tab Workspace | Toolbars & Ribbons | Document Canvas | Dialogs | Themes     |
+---------------------------------------------------------------------------------------------------+
                                                  | (Qt Signals / Command Pattern)
                                                  v
+---------------------------------------------------------------------------------------------------+
|                                     APPLICATION SERVICES LAYER                                    |
|   +--------------------------+   +---------------------------+   +----------------------------+   |
|   |   JobManager (Async)     |   | DocumentLifecycleManager  |   |    SessionRecoveryService  |   |
|   +--------------------------+   +---------------------------+   +----------------------------+   |
|   |   BatchProcessingService |   | Export & ConversionService|   |    PrintingService         |   |
|   +--------------------------+   +---------------------------+   +----------------------------+   |
+---------------------------------------------------------------------------------------------------+
                                                  |
                                                  v
+---------------------------------------------------------------------------------------------------+
|                                      DOMAIN LAYER (domain/)                                       |
|   DocumentModel | PageCollection | AnnotationTree | FormManager | Undo/Redo Engine | Capabilities |
+---------------------------------------------------------------------------------------------------+
          |                                                                           |
          v                                                                           v
+------------------------------------+                             +--------------------------------+
|    PDF ENGINE ABSTRACTION (pdf_api) |                             |   SECURITY & PERSISTENCE       |
|  +-------------------------------+ |                             |  +---------------------------+ |
|  | IPdfEngine Interface          | |                             |  | AppConfig / Settings      | |
|  +-------------------------------+ |                             |  +---------------------------+ |
|                 |                  |                             |  | SecureString              | |
|                 v                  |                             |  +---------------------------+ |
|  +-------------------------------+ |                             |  | PathSanitizer             | |
|  | PdfiumAdapter (Rendering/OCR) | |                             |  +---------------------------+ |
|  +-------------------------------+ |                             |  | LogRedactor               | |
|  | QpdfAdapter (Structure/PDF/A) | |                             |  +---------------------------+ |
|  +-------------------------------+ |                             +--------------------------------+
+------------------------------------+                                             |
                  |                                                                v
                  v                                                +--------------------------------+
+------------------------------------+                             |     PLATFORM ADAPTER LAYER     |
|    THIRD-PARTY ENGINES (Isolated)  |                             |  +---------------------------+ |
|  +-------------------------------+ |                             |  | IPlatformAdapter          | |
|  | Google PDFium (Apache-2.0)    | |                             |  +---------------------------+ |
|  | QPDF Library (Apache-2.0)     | |                             |  | WindowsPlatformAdapter    | |
|  | Tesseract OCR (Apache-2.0)    | |                             |  | LinuxPlatformAdapter      | |
|  +-------------------------------+ |                             |  | AndroidPlatformAdapter    | |
+------------------------------------+                             +--------------------------------+
```

---

## 3. Directory Layout

The PDFinity codebase is structured into modular layers with clean separation of concerns:

```
PDFinity/
├── src/
│   ├── app/                 # CLI entry points and command handlers (Batch, MCP, Remote)
│   ├── application/         # AppContext (singleton lifecycle), AppConfig, Logger
│   ├── domain/              # Pure domain models (DocumentModel, BatchTypes, Annotations)
│   ├── mcp/                 # Model Context Protocol stdio server, tool catalog, sandbox
│   ├── pdf_adapters/        # Google PDFium and QPDF engine implementations
│   ├── pdf_api/             # Abstract PDF engine interfaces (IPdfEngine, PdfTypes)
│   ├── platform/            # OS platform adapters (IPlatformAdapter, Windows, Generic)
│   ├── remote/              # Remote Controller server, client, pairing, dispatcher
│   ├── security/            # SecureString, PathSanitizer, LogRedactor
│   ├── services/            # Domain services (Batch, Redaction, Signatures, OCR, etc.)
│   └── ui/                  # Qt Widgets UI dialogs, toolbars, viewports, canvas
├── include/                 # Public SDK headers (if exported)
├── tests/                   # Automated test suites (unit, integration, ui, security)
├── packaging/               # Inno Setup (Windows), AppImage/tarball (Linux), DMG (macOS)
├── scripts/                 # Automation scripts (build_runner.py, package_windows.py)
└── docs/                    # Specifications, architecture decisions, guides, and SBOM
```

---

## 4. Key Subsystems and Patterns

### A. PDF Engine Abstraction (`pdf_api/IPdfEngine.hpp`)
All document operations (opening, rendering, page counts, metadata, text extraction, content editing, annotations) pass through `IPdfEngine`. The default implementation is `pdf_adapters/PdfiumAdapter.cpp`, backed by Google's PDFium library.
- **Rule:** Never invoke `FPDF_*` functions directly from UI classes or services. Always route through `IPdfEngine`.

### B. Document Life Cycle & Atomic Writes
- Saving a document writes to a temporary file (`.tmp`) first. Once writing and checksum calculation succeed, the file is atomically renamed over the destination.
- Source documents opened for reading are never modified in place unless explicitly approved by the user through a distinct Save command.

### C. Security and Sandboxing (`security/`)
- **`SecureString`:** Wraps sensitive memory (passwords, PINs, tokens) and zeroes memory (`SecureZeroMemory` / `memset_s`) upon destruction.
- **`PathSanitizer`:** Canonicalizes paths, strips traversal attempts (`..`), detects symbolic link escapes, and blocks reserved Windows DOS devices (`CON`, `PRN`, `AUX`, `NUL`, `COM1-9`, `LPT1-9`).
- **`LogRedactor`:** Automatically filters passwords, keys, and tokens from log statements before writing to disk.
