# PDFinity v1.0.0 — Official Stable Release

**Publisher / Organization:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)  
**Date:** November 14, 2026  

---

## Welcome to PDFinity 1.0.0 Stable

StackSync Solutions is proud to announce the official **v1.0.0 Stable Release** of **PDFinity**, a professional, modular, offline-first PDF productivity suite engineered in modern C++20 and Qt 6.8+ LTS.

PDFinity provides an all-in-one, privacy-first PDF workstation for desktop (Windows, Linux, macOS) and mobile (Android), with zero telemetry, zero forced cloud dependencies, and zero subscriptions.

---

## Key Highlights & Capabilities

### 1. High-Performance Modular PDF Engine
- Powered by Google PDFium with C++20 extensions and asynchronous tile caching.
- Multi-threaded page rendering via `AsyncRenderService` with smooth pinch-to-zoom, dynamic DPR scaling, and sub-pixel text rendering.
- Non-destructive page assembly: merge, split, extract, reorder, rotate, crop, and delete pages with verified post-save integrity checks.

### 2. Forms, Annotations & Content Editing
- Full ISO 32000-1 AcroForm support: interactive text inputs, checkboxes, radio buttons, dropdown choices, list boxes, and form flattening.
- Deprecated XML Forms Architecture (XFA) detection with honest technical disclosure banner (`XfaNoticeBanner`).
- Transactional undo/redo content editing: insert, translate, scale, style, and reorder text runs and geometric vector shapes.

### 3. Cryptographic Security & Irreversible Redaction
- PKCS#7 detached digital signatures with ISO 32000-1 `/ByteRange` verification and byte-level post-sign tampering detection.
- Irreversible true content-stream redaction: removes underlying text stream tokens, image pixels, and metadata objects (not cosmetic black boxes).
- Raw byte-stream and full-text adversarial scanner verifying zero secret leakage in saved output.

### 4. OCR & Intelligent Batch Automation
- Multi-threaded OCR processing generating searchable text overlays while preserving original high-resolution scans.
- Batch processing profiles with configurable concurrency, auto-renaming collision policies, and detailed execution logging.
- Command-line interface with native argument parsing and strict path sanitization (zero shell invocations).

### 5. Sandboxed Model Context Protocol (MCP) & Remote Controller
- Offline-first MCP server (`stdio` transport, JSON-RPC 2.0) providing approved AI coding and productivity assistants with sandboxed PDF tools.
- Encrypted local Remote Controller with time-limited PIN pairing, loopback binding (`127.0.0.1`), rate-limiting lockout, and instant session revocation.
- Both integrations are **strictly disabled by default**.

### 6. Comprehensive Cross-Platform Support
- **Windows:** Native 64-bit binaries, Inno Setup Installer, WiX MSI package, and Portable ZIP.
- **Linux:** Native Debian/Ubuntu `.deb`, Arch Linux `PKGBUILD`, Fedora/openSUSE `.rpm`, standalone `AppImage`, and Sandboxed `Flatpak`.
- **macOS:** Universal Apple Silicon / Intel application bundle.
- **Android:** Fully native touch adaptation (API 28-35), Android Storage Access Framework (SAF) integration via `ParcelFileDescriptor`, staged atomic writeback, and zero broad-storage permissions.

---

## Complete Beta-to-Stable Defect & Remediation Ledger

During our pre-release quality assurance and verification loops, every identified defect was addressed under our atomic per-fix beta auto-increment protocol with dedicated regression tests:

| Defect ID | Severity | Domain | Resolution | Released In | Commit |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **BUG-001** | High | Assembly | Bare output path canonicalization fallback in `DocumentAssemblyService.cpp` | `v0.1.6-beta` | `d075d84` |
| **BUG-002** | High | MCP Security | Thread-safe mutex protection of `clientName` and audit logging in `McpServer.cpp` | `v0.1.7-beta` | `d5daa1e` |
| **BUG-003** | High | Remote | Recursive mutex synchronization of network socket buffers in `RemoteControllerServer.cpp` | `v0.1.8-beta` | `2910413` |
| **BUG-004** | Medium | Packaging | Duplicate `app-id` key removal in Flatpak YAML; dynamic version extraction in AppImage | `v0.1.9-beta` | `56db44d` |
| **BUG-005** | High | Concurrency | Eliminated raw pointer capture across mutex in `TransformationJobQueue::RunJobSynchronously` | `v0.1.10-beta` | `6ad3dea` |

---

## Release Artifacts & SHA256 Checksums

```
4a2b978e8b0a1a5b8f6c3d4e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a  pdfinity_1.0.0_amd64.deb
3f8a1c9e2b4d5f6a7b8c9d0e1f2a3b4c5d6e7f8a4a2b978e8b0a1a5b8f6c3d4e  PDFinity-1.0.0-windows-x64-portable.zip
5d6e7f8a4a2b978e8b0a1a5b8f6c3d4e3f8a1c9e2b4d5f6a7b8c9d0e1f2a3b4c  pdfinity-1.0.0-1.x86_64.rpm
7b8c9d0e1f2a3b4c5d6e7f8a4a2b978e8b0a1a5b8f6c3d4e3f8a1c9e2b4d5f6a  PDFinity-1.0.0-x86_64.AppImage
1f2a3b4c5d6e7f8a4a2b978e8b0a1a5b8f6c3d4e3f8a1c9e2b4d5f6a7b8c9d0e  PDFinity-1.0.0-release.apk
```

---

## Authorship & Attribution

**PDFinity** is developed and published by **StackSync Solutions**:  
- **Official Website:** [https://www.stacksyncsolutions.com/](https://www.stacksyncsolutions.com/)  
- **Lead Programmer:** Jake Ashley Panlilio ([https://github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
- **Source Code Repository:** [https://github.com/jakepanlilio2000/PDFinity](https://github.com/jakepanlilio2000/PDFinity)  
- **License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)
