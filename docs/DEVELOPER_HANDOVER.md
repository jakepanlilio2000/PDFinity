# PDFinity Developer Handover and Architectural Guide

**Target Audience:** Core C++ Maintainers, Contributing Engineers, Systems Architects  
**Codebase Standard:** ISO C++20 / Qt 6.8+ LTS  
**Version:** 1.0.0-rc1  

---

## 1. Architectural Layout

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
├── tests/                   # 51 automated test suites (unit, integration, ui, security)
├── packaging/               # Inno Setup (Windows), AppImage/tarball (Linux), DMG (macOS)
├── scripts/                 # Automation scripts (build_runner.py, package_windows.py)
└── docs/                    # Specifications, architecture decisions, guides, and SBOM
```

---

## 2. Key Subsystems and Patterns

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

### D. Model Context Protocol (`mcp/`)
- Implements JSON-RPC 2.0 version `2024-11-05` over standard I/O.
- Operates under strict user-approved directory roots (`McpSecurityPolicy`).
- Requires interactive desktop confirmation for destructive or administrative operations.

### E. Remote Controller Server (`remote/`)
- Lightweight line-delimited JSON-RPC 2.0 server binding exclusively to loopback or LAN.
- Mutual authentication using short-lived 6-digit numeric PINs with a 60-second window.
- Lockout protection: 5 failed attempts trigger a 5-minute client lockout.
- Monotonically increasing nonces provide replay protection.

---

## 3. How to Extend PDFinity

### Adding a New MCP Tool
1. Define the tool metadata and JSON Schema in `src/mcp/McpToolCatalog.cpp` within `GetAvailableTools()`.
2. Assign the appropriate permission classification (`ReadOnly`, `Write`, `Destructive`, `SecuritySensitive`).
3. Handle the execution in `src/mcp/McpServer.cpp` inside `HandleToolsCall()`.
4. Add automated test coverage in `tests/security/test_mcp_security.cpp`.

### Adding a New Remote Controller Command
1. Add the enum value to `RemoteCommand` in `src/remote/RemoteTypes.hpp` and map its string identifier.
2. In `src/remote/RemoteCommandDispatcher.cpp`, add the execution dispatch branch in `Execute()`.
3. If the command modifies files or session state, register it in `IsSensitiveRemoteCommand()` to trigger host desktop confirmation.
4. Add test cases in `tests/unit/test_remote_server.cpp` and `tests/security/test_fuzz_boundaries.cpp`.

### Adding a New Batch Processing Operation
1. Add the operation type to `BatchOperationType` in `src/domain/BatchTypes.hpp`.
2. Implement the file-level transformation in `src/services/BatchProcessingService.cpp` within `ExecuteSingleFile()`.
3. Add the operation UI configuration in `src/ui/BatchProfileEditorDialog.cpp`.

---

## 4. Build, Test, and Packaging Recipes

```powershell
# 1. Clean build and run all 51 test suites
python scripts/build_runner.py

# 2. Package Windows installer and zero-footprint portable ZIP
python scripts/package_windows.py

# 3. Launch portable build smoke check
dist/PDFinity-1.0.0-rc1-windows-x64-portable/PDFinity.exe --headless-smoke
```
