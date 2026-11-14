# PDFinity Release Notes

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  

---

## Version 1.0.0 (1.0 Stable) — Production General Availability
**Release Date:** 2026-11-14  
**Git Tag:** `v1.0.0`  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  

### Highlights & Key Features
1. **Production-Hardened PDF Suite:** Complete, offline-first desktop productivity application providing rendering, editing, signing, redaction, OCR, batch automation, and inspection.
2. **Cross-Platform Distribution:**
   - **Windows:** Self-contained Inno Setup installer (`PDFinity-1.0.0-Setup-x64.exe`), enterprise WiX MSI (`PDFinity-1.0.0.msi`), and true zero-footprint portable distribution (`PDFinity-1.0.0-windows-x64-portable.zip`).
   - **Linux:** Native packages for Debian/Ubuntu (`.deb`), Arch Linux (`PKGBUILD`), Fedora/RHEL (`.rpm`), universal AppImage (`.AppImage`), and sandboxed Flatpak (`com.stacksyncsolutions.pdfinity`).
   - **macOS:** Universal Apple Silicon (`arm64`) & Intel (`x64`) DMG image with native Cocoa integration.
   - **Android:** Native mobile application package (`.apk`) supporting Android 8.0+ (API 26+) with Android Storage Access Framework (SAF) integration and touch gestures.
3. **High-Assurance Security & Privacy:**
   - Multi-pass irreversible redaction excising underlying glyphs and content stream operators.
   - PAdES / PKCS#7 detached cryptographic digital signatures with X.509 certificate trust management.
   - Boundary-hardened parser resilient against corrupted xref structures, malformed headers, and hostile path traversals.
4. **Developer & Automation Tooling:**
   - Headless scriptable CLI: `PDFinity batch`, `batch-profiles`, `batch-history`.
   - Local Model Context Protocol (MCP) stdio server (2024-11-05 spec) with approved roots sandbox.
   - Secure companion Remote Controller with PIN pairing, rate limiting, and replay defense.

---

## Version 0.1-Beta (0.1.0-beta) — Initial Clean Release Baseline
**Release Date:** 2026-11-14  
**Git Tag:** `v0.1-beta`  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  

### Initial Baseline Features
- Dual-engine core foundation integrating Google PDFium (rasterization) and QPDF (structural manipulation).
- Multi-tab document workspace with arbitrary zoom, continuous scrolling, outline bookmarks, and high-speed text search.
- Document assembly studio supporting lossless merge, split, page rotation, and extraction.
- Offline-first execution guarantee with zero telemetry and zero background network polling.
- MPL-2.0 open-source licensing compliance and dynamic LGPLv3 Qt 6 library linkage.
