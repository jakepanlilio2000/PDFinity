# PDFinity: Architecture Decision Records (ADRs)

**Document Version:** 1.0.0  
**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Architecture Decision Records  

---

## Index of Architecture Decision Records

- [ADR-001: Separation of Modern Platforms from Legacy Windows 7 SP1 Compatibility Profile](#adr-001-separation-of-modern-platforms-from-legacy-windows-7-sp1-compatibility-profile)
- [ADR-002: Dual-Engine Architecture: PDFium for Rendering and QPDF for Structural Manipulation](#adr-002-dual-engine-architecture-pdfium-for-rendering-and-qpdf-for-structural-manipulation)
- [ADR-003: Rejection of AGPLv3 and GPL PDF Engines for Enterprise License Compliance](#adr-003-rejection-of-agplv3-and-gpl-pdf-engines-for-enterprise-license-compliance)
- [ADR-004: Selection of Qt 6 Widgets (QtWidgets) for Desktop Presentation Layer](#adr-004-selection-of-qt-6-widgets-qtwidgets-for-desktop-presentation-layer)
- [ADR-005: Strict Offline-First Operation and Total Ban on Core Telemetry and Cloud Dependencies](#adr-005-strict-offline-first-operation-and-total-ban-on-core-telemetry-and-cloud-dependencies)
- [ADR-006: Native Windows CNG and OpenSSL Cryptography Abstraction](#adr-006-native-windows-cng-and-openssl-cryptography-abstraction)
- [ADR-007: Multi-Pass Irreversible Redaction Architecture](#adr-007-multi-pass-irreversible-redaction-architecture)
- [ADR-008: Opt-In Local-First MCP and Remote Controller with Explicit User Consent](#adr-008-opt-in-local-first-mcp-and-remote-controller-with-explicit-user-consent)

---

### ADR-001: Separation of Modern Platforms from Legacy Windows 7 SP1 Compatibility Profile

#### Context
The master specification requires supporting primary modern desktop environments (Windows 10/11 x64, Linux x64, macOS) while maintaining Windows 7 SP1 x86 as a compatibility target. However:
1. Modern desktop frameworks (Qt 6.0+) and C++ standard libraries (C++20/C++23) require modern OS kernels and user-mode APIs (`api-ms-win-*`, Per-Monitor DPI v2, modern threadpools).
2. Qt 6 explicitly dropped Windows 7 support.
3. Windows 7 lacks modern TLS 1.3 in Schannel and requires SHA-2 code signing patches (KB4474419).
4. Attempting to force a single binary or toolchain across both environments severely degrades modern capabilities or makes compilation impossible under modern MSVC toolsets.

#### Decision
We establish a **Two-Profile Architecture**:
- **Profile A (Primary Modern Target):** Modern C++20, Qt 6.8+ LTS (`QtWidgets`), Windows SDK 10.0.26100+, targeting Windows 10/11 x64/x86, Linux x64, and macOS.
- **Profile B (Legacy Windows 7 Compatibility):** Isolated build configuration/branch using C++17, Qt 5.15 LTS, MSVC `v141_xp` (or MinGW-w64), and static OpenSSL.
- **Evidence Rule:** Windows 7 support will be classified as `CONDITIONAL / UNVERIFIED` until automated end-to-end test execution is validated and logged on an authentic Windows 7 environment. Modern installers will enforce a Windows 10 minimum version gate.

#### Consequences
- **Positive:** Modern targets leverage modern C++20 language features, hardware-accelerated rendering, Per-Monitor DPI v2, and secure CNG cryptography without legacy constraints.
- **Negative:** Maintaining Profile B requires maintaining separate toolchains and CI configurations.

---

### ADR-002: Dual-Engine Architecture: PDFium for Rendering and QPDF for Structural Manipulation

#### Context
No single open-source PDF engine provides top-tier performance across both high-speed interactive rendering and deep lossless structural object manipulation:
- **Google PDFium:** Highly optimized, secure, Chromium-fuzzed engine for rasterization, page display, text coordinate extraction, and AcroForms rendering, but lacks clean APIs for structural stream reconstruction, linearizing, and lossless page surgery.
- **QPDF:** Industry-standard engine for PDF structural manipulation, page assembly, linearization, object stream deduplication, and encryption, but has no rasterization/bitmap rendering capabilities.

#### Decision
Implement a **Dual-Engine Architecture** unified behind an internal `PdfEngineInterface`:
- **PDFium Adapter:** Drives interactive viewport rendering, zoom scaling, text selection/search hit-testing, annotation rendering, and form interaction.
- **QPDF Adapter:** Drives lossless document assembly (merge, split, page reordering, rotation), stream decompression/normalization, xref reconstruction, and metadata sanitization.

#### Consequences
- **Positive:** Leverages the best-in-class capabilities of both engines while completely isolating them from the UI and application domain layers.
- **Neutral:** Increases build binary size moderately (~15-20 MB total for both engines), which is negligible for modern desktop applications.

---

### ADR-003: Rejection of AGPLv3 and GPL PDF Engines for Enterprise License Compliance

#### Context
Candidate engines include Poppler (GPLv2/GPLv3) and MuPDF (AGPLv3 / Artifex Commercial).
- Poppler's GPL license triggers strong copyleft requirements across the entire application.
- MuPDF's AGPLv3 license imposes network disclosure requirements and conflicts with proprietary or commercial plugin integration.

#### Decision
Strictly reject **Poppler** and **MuPDF**. Adopt **PDFium (Apache 2.0 / BSD 3-Clause)** and **QPDF (Apache 2.0)** as the standard engine stack.

#### Consequences
- **Positive:** Clean, unencumbered enterprise licensing. Zero risk of viral copyleft litigation or forced disclosure of proprietary enterprise modules.
- **Negative:** PDFium's build tree requires vendored pre-built binaries or specialized build scripts rather than simple system package manager links on some Linux distributions.

---

### ADR-004: Selection of Qt 6 Widgets (QtWidgets) for Desktop Presentation Layer

#### Context
Desktop UI frameworks evaluated included Qt Quick (QML), Qt Widgets (`QtWidgets`), wxWidgets, Dear ImGui, and web-based wrappers (Electron/CEF).
A professional PDF suite requires standard desktop menus, ribbon/toolbars, dockable sidebars, native system dialogs, accessibility (MSAA/IAccessible2), high-DPI scaling, and sub-100MB memory footprint.

#### Decision
Select **Qt 6 Widgets (`QtWidgets`)** dynamically linked under **LGPLv3**.
- High-fidelity native desktop widgets with comprehensive stylesheet customization.
- Excellent keyboard shortcut routing, dockable panels (`QDockWidget`), and multi-document tab controls.
- Integrated printing architecture (`Qt6PrintSupport` / `QPrinter`).
- Full dynamic linking compliance with LGPLv3 (dynamic DLLs, third-party notices, swappable binaries).

#### Consequences
- **Positive:** Mature desktop UX, robust native file dialogs, rapid development of complex toolbars, low memory footprint (~80 MB idle).
- **Negative:** Requires bundling Qt shared libraries with installers (~45 MB compressed).

---

### ADR-005: Strict Offline-First Operation and Total Ban on Core Telemetry and Cloud Dependencies

#### Context
Many commercial PDF applications leak user privacy by phoning home, sending document metadata to analytics platforms, forcing user account logins, or delegating operations to cloud services. Enterprise and security-conscious users require absolute data privacy.

#### Decision
PDFinity is strictly **offline-first**:
1. Zero telemetry, zero analytics tracking, zero crash report transmission without explicit user confirmation, zero remote account dependencies.
2. Core document processing (reading, editing, conversion, OCR, signing, redaction) operates with zero internet access.
3. Network features (Remote Controller, optional online certificate revocation checking, MCP) are disabled by default and require deliberate user opt-in.

#### Consequences
- **Positive:** Total privacy compliance (GDPR, HIPAA, air-gapped security certification).
- **Negative:** Feature usage analytics are unavailable to developers; bug reports rely on manual user submission.

---

### ADR-006: Native Windows CNG and OpenSSL Cryptography Abstraction

#### Context
Cryptographic signing, hash generation, and password encryption require cryptographically sound implementations. Relying exclusively on external OpenSSL binaries on Windows can introduce DLL hell and version mismatches.

#### Decision
Implement a decoupled `ICryptoAdapter`:
- **Windows Implementation:** Leverages Windows Cryptography API: Next Generation (CNG: `bcrypt.dll` / `ncrypt.dll`), utilizing hardware-backed TPM keys and Windows Certificate Store where available.
- **POSIX Implementation:** Leverages OpenSSL 3.x on Linux and CommonCrypto/OpenSSL on macOS.

#### Consequences
- **Positive:** Zero external crypto runtime dependency on Windows; native integration with Windows smart cards and hardware security modules.
- **Negative:** Maintaining platform-specific crypto adapters.

---

### ADR-007: Multi-Pass Irreversible Redaction Architecture

#### Context
Visual redaction (painting a black rectangle over text) is a well-known vulnerability that has repeatedly caused catastrophic data leaks in legal and government agencies because the underlying text stream remains intact in the PDF file structure.

#### Decision
PDFinity implements a mandatory **Multi-Pass Irreversible Redaction Architecture**:
1. **Pass 1 (Visual Obliteration):** Rasterize the target redaction bounding box directly into the underlying page bitmap or substitute with a solid vector block.
2. **Pass 2 (Content Stream Surgery):** Parse the page content stream (`/Contents`), identify all text operators (`Tj`, `TJ`, `'`, `"`) and vector paths intersecting the redaction box, and completely excise them from the operator stream.
3. **Pass 3 (Metadata & Auxiliary Sanitization):** Purge all matching text strings from Document Information dictionaries, XMP metadata streams, bookmarks, annotations, thumbnails, and AcroForm field entries.
4. **Pass 4 (Serialization & XRef Rebuilding):** Pass the document through QPDF to purge unreferenced orphaned object streams and rebuild a clean cross-reference table.

#### Consequences
- **Positive:** 100% mathematically and structurally irreversible redaction verifiable via automated byte-level extraction tests.
- **Negative:** More computationally intensive than naive drawing; requires a background asynchronous job.

---

### ADR-008: Opt-In Local-First MCP and Remote Controller with Explicit User Consent

#### Context
The master specification provides for optional Model Context Protocol (MCP) tool integration and a companion Remote Controller interface. If unconstrained, these integrations can introduce severe security vulnerabilities (remote command injection, unauthorized file access, path traversal).

#### Decision
1. **Disabled by Default:** Both subsystems are inactive upon fresh installation.
2. **Local Stdio Transport:** MCP communicates via standard I/O (stdio) for local AI clients. Any network socket binding is restricted strictly to loopback (`127.0.0.1`).
3. **Pairing Handshake:** Remote Controller requires a cryptographically random one-time pairing code and mutual TLS authentication.
4. **Mandatory Confirmation Gate:** Any destructive action (file overwrite, document deletion, redaction, digital signing) triggered via MCP or Remote Controller mandates an explicit modal confirmation dialog in the host desktop UI.

#### Consequences
- **Positive:** Eliminates arbitrary code execution and silent document tampering vectors while supporting powerful developer automation.
- **Negative:** Fully automated headless unattended workflows for destructive actions are intentionally constrained by design.
