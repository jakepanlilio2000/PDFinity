# PDFinity: Software Dependency Register and SBOM Baseline

**Document Version:** 1.0.0  
**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Dependency Register  

---

## 1. Dependency Governance and Selection Principles

PDFinity enforces strict dependency governance to protect document integrity, application security, and licensing compliance:
1. **Minimal External Footprint:** Avoid extraneous third-party libraries. If a capability can be cleanly implemented in standard C++20 or via OS-native APIs, avoid introducing external packages.
2. **License Purity:** Strictly reject viral copyleft licenses (GPLv2, GPLv3, AGPLv3) to ensure modular enterprise distribution.
3. **Vendoring vs. System Linking:** Core PDF parsing and cryptographic engines must be pinned and reproducible across developer machines and CI pipelines.
4. **Vulnerability Tracking:** Every third-party component is tracked against national vulnerability databases (NVD) and GitHub Advisory records.
5. **Separation of Concerns:** External PDF and OCR engines are accessed exclusively through internal interfaces (`PdfEngineInterface`, `IOcrEngine`).

---

## 2. Master Dependency Register

| Component | Pinned Version / Tag | License | Linking Type | Primary Target Compatibility | Legacy Win7 Compatibility | Upstream Repository / Source | Primary Role in PDFinity |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Qt 6 Framework** | 6.8.2 LTS | LGPLv3 | Dynamic (Shared DLLs) | Win 10/11 x64, Linux x64, macOS 12+ | **No** (Requires Qt 5.15) | [https://code.qt.io/cgit/qt/qtbase.git](https://code.qt.io/cgit/qt/qtbase.git) | Desktop shell, widgets, event loop, printing abstraction, high-DPI handling, IPC. |
| **Google PDFium** | Chromium rev 8152 (win-x64) | Apache 2.0 / BSD 3-Clause | Dynamic / Import Lib | All modern desktop platforms | **Conditional** (Requires legacy CRT patch) | [https://github.com/bblanchon/pdfium-binaries](https://github.com/bblanchon/pdfium-binaries) (Vendored in `third_party/pdfium`) | High-fidelity page rasterization, text geometry extraction, font inspection, AcroForm rendering. |
| **QPDF** | 11.9.1+ | Apache 2.0 | Dynamic / Static Lib | Win 10/11 x64, Linux x64, macOS | **Yes** (Compiles under MSVC `v141_xp`) | [https://github.com/qpdf/qpdf](https://github.com/qpdf/qpdf) | Structural document assembly, page extraction, stream normalization, encryption/decryption, linearization. |
| **OpenSSL** | 3.3.2+ | Apache 2.0 | Dynamic (DLL) / OS Native | Win 10/11, Linux, macOS | **Conditional** (Requires Win7 TLS patch) | [https://github.com/openssl/openssl](https://github.com/openssl/openssl) | Cryptographic digests (SHA-256, SHA-512), digital signatures (PKCS#7/CMS), PKCS#12 container parsing. |
| **Windows CNG / BCrypt** | Windows SDK 10.0.26100 | Proprietary OS API (Microsoft) | Dynamic (`bcrypt.lib`, `ncrypt.lib`) | Windows 10/11 x64/x86 | **Partial** (Legacy WinCrypt on Win7) | Windows Operating System Component | OS-native cryptographic provider on Windows; eliminates OpenSSL runtime dependency where possible. |
| **SQLite** | 3.46.1+ | Public Domain | Static (Amalgamation) | All platforms | **Full** | [https://sqlite.org](https://sqlite.org) | Local settings storage, document session recovery, search index cache, and local audit logging. |
| **nlohmann/json** | 3.11.3 | MIT | Header-only | All platforms | **Full** (Standard C++11+) | [https://github.com/nlohmann/json](https://github.com/nlohmann/json) | JSON parsing and serialization for MCP tool schemas, settings export, and batch job specifications. |
| **Tesseract OCR** | 5.4.1+ (Optional) | Apache 2.0 | Dynamic / Standalone Tool | Win 10/11 x64, Linux x64, macOS | **Disabled** (Requires AVX2/modern CRT) | [https://github.com/tesseract-ocr/tesseract](https://github.com/tesseract-ocr/tesseract) | Local offline OCR text layer generation for scanned PDF pages. |
| **Leptonica** | 1.84.1+ (Optional) | BSD 2-Clause | Dynamic / Static Lib | Win 10/11 x64, Linux x64, macOS | **Disabled** | [https://github.com/DanBloomberg/leptonica](https://github.com/DanBloomberg/leptonica) | Image binarization, deskewing, and image preprocessing prior to OCR. |
| **zlib** | 1.3.1 | zlib License | Static / Dynamic Lib | All platforms | **Full** | [https://github.com/madler/zlib](https://github.com/madler/zlib) | Stream compression and decompression for PDF FlateDecode filters. |

---

## 3. Toolchain and Build Tools

| Tool | Version | Vendor / Maintainer | Path / Provenance | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **MSVC Compiler** | 19.51.36260 (`v144`/VS 2026) | Microsoft Corporation | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools` | C++20/C++23 compiler and linker for Windows x64 and x86 modern builds. |
| **CMake** | 4.3.1-msvc1 | Kitware | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\...\bin\cmake.exe` | Project configuration, preset evaluation, build orchestration. |
| **Ninja** | 1.13.2 | Ninja Maintainers | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\...\bin\ninja.exe` | Fast, deterministic parallel compilation execution. |
| **Python** | 3.15.0 | Python Software Foundation | `C:\Python315\python.exe` | Test fixtures, fuzz harness automation, SBOM generation, build scripts. |
| **Inno Setup** | 6.x / 7.x | Jordan Russell | `C:\Program Files\Inno Setup 7` | Windows installer compilation, uninstaller generation, file association creation. |

---

## 4. Security Advisory and Vulnerability Audit

Prior to baseline approval, all selected dependencies were audited for known critical security vulnerabilities:

1. **Qt 6.8.2:** Verified clean against known remote code execution or privilege escalation advisories. Memory leaks in SVG and image rendering patched in Qt 6.8.1+.
2. **PDFium (Current Chromium branch):** PDFium receives continuous fuzzing by Google ClusterFuzz. Using an up-to-date stable release protects against historical font parser vulnerabilities (e.g., CVE-2023-4863, CVE-2023-2033).
3. **QPDF (11.9+):** Hardened against malformed xref tables and stream length overflow vulnerabilities (CVE-2021-36978 addressed in 10.4+).
4. **OpenSSL (3.3.2):** Addresses CVE-2024-0727 and historical PKCS#7 decode memory corruption vulnerabilities.

---

## 5. Software Bill of Materials (SBOM) Generation Strategy

PDFinity adopts standard machine-readable SBOM generation as part of every release pipeline:
- **Format:** CycloneDX v1.5 JSON and SPDX 2.3 JSON.
- **Automation:** CMake post-build target executes `scripts/generate_sbom.py`, extracting package names, exact commit hashes/versions, license identifiers (SPDX), and cryptographic hashes (SHA-256) of all compiled binaries and bundled third-party libraries.
- **Verification:** CI builds compare generated SBOM records against this approved `DEPENDENCIES.md` register to prevent unauthorized dependency drift.
