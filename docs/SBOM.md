# Software Bill of Materials (SBOM)

**Product:** PDFinity  
**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Format Alignment:** SPDX 2.3 / CycloneDX 1.5 Specification  

---

## 1. Primary Application Metadata

| Property | Value |
| :--- | :--- |
| **Package Name** | `PDFinity` |
| **SPDX ID** | `SPDXRef-Application-PDFinity` |
| **Supplier** | StackSync Solutions (https://www.stacksyncsolutions.com/) |
| **Lead Programmer** | Jake Ashley Panlilio (https://github.com/jakepanlilio2000) |
| **License Declared** | `MPL-2.0` (with dynamic `LGPL-3.0-only` Qt 6 bindings) |
| **C++ Standard** | ISO C++20 |
| **Supported Platforms** | Windows 10/11 x64, Linux (Ubuntu/Debian, Arch, Fedora/RHEL), macOS (Apple Silicon / Intel), Android 8.0+ (API 26+) |

---

## 2. Component Inventory

| Component Name | Version | SPDX License Identifier | Linking Method | Scope / Function | Upstream Repository / Source |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Qt 6 Framework** | 6.8.2 | `LGPL-3.0-only` | Dynamic (`.dll` / `.so` / `.dylib`) | UI, Event Loop, High-DPI, Printing, Network Sockets | `https://code.qt.io` |
| **Google PDFium** | 6825 | `Apache-2.0` AND `BSD-3-Clause` | Dynamic (`pdfium.dll`) | PDF Rendering, Text Extraction, Annotation Geometry | `https://pdfium.googlesource.com/pdfium/` |
| **QPDF** | 11.9.0 | `Apache-2.0` | Header / API integration | PDF Document Linearization, Structure Manipulation, Optimization | `https://github.com/qpdf/qpdf` |
| **OpenSSL** | 3.3.1 | `Apache-2.0` | Shared / System Driver | Cryptographic primitives, SHA-256 digests, X.509 cert validation | `https://www.openssl.org` |
| **SQLite** | 3.45.1 | `blessing` (Public Domain) | Static Source Amalgamation | Structured metadata storage, Batch queue state, Audit trails | `https://www.sqlite.org` |
| **nlohmann/json** | 3.11.3 | `MIT` | Header-only | Configuration, MCP JSON-RPC 2.0 schemas, Remote protocol | `https://github.com/nlohmann/json` |
| **zlib** | 1.3.1 | `Zlib` | Shared / Static | Flate compression/decompression | `https://zlib.net` |
| **Tesseract OCR / Leptonica** | 5.3.4 | `Apache-2.0` AND `BSD-2-Clause` | Dynamic / External worker | Optical Character Recognition service integration | `https://github.com/tesseract-ocr/tesseract` |

---

## 3. Compliance and Copyleft Verification

1. **Viral Copyleft Exclusion:**
   - No components licensed under `GPLv2`, `GPLv3`, or `AGPLv3` exist within the dependency graph.
   - Poppler (`GPLv2+`), MuPDF (`AGPLv3`), and Ghostscript (`AGPLv3`) are strictly prohibited and absent.
2. **LGPLv3 Dynamic Linking Compliance:**
   - Qt dynamic libraries (`Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6Network`, `Qt6PrintSupport`) reside alongside the main executable and are never linked statically.
   - End-users can swap out Qt shared libraries with their own compiled versions.
3. **Attribution Preservation:**
   - Binary packages bundle `LICENSE` and `THIRDPARTY_NOTICES.txt`.
   - The application provides an interactive **Help -> About -> Licenses** dialog.
