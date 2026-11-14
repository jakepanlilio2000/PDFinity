# Changelog

All notable changes to the **PDFinity** project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)  

---

## [0.1.2-beta] - 2026-11-14

### Fixed
- **Android Packaging Hierarchy Sync:** Mirrored Android packaging files (`AndroidManifest.xml`, `build.gradle`, `gradle.properties`, `settings.gradle`, and `res/xml/file_paths.xml`) into `packaging/android/` aligning with standard distribution structure.

---

## [0.1.1-beta] - 2026-11-14

### Fixed
- **AppStream License Alignment:** Corrected `<metadata_license>` in AppStream metainfo XML to `MPL-2.0` conforming to repository licensing requirements.
- **RPM Spec Extraction Macro:** Corrected `%prep` in `packaging/linux/rpm/pdfinity.spec` using `%global tag_version` to match GitHub archive extraction directory.
- **Arch PKGBUILD Build Isolation:** Explicitly anchored build and install targets to `"${srcdir}/build"` in `packaging/linux/arch/PKGBUILD`.
- **Flatpak App-ID Consistency:** Unified Flatpak manifest `app-id` casing to `com.stacksyncsolutions.pdfinity` ensuring exact alignment with AppStream metadata identifier.

---

## [0.1-Beta] - Initial Public Beta

### Added
- **Initial Clean Release Baseline:** Public open-source baseline tagged as `v0.1-beta`.
- **Core Architecture:** Dual-engine foundation integrating Google PDFium (high-speed rasterization, geometry extraction, and AcroForms) and QPDF (structural manipulation, stream serialization, and encryption) behind a unified `IPdfEngine` interface.
- **Application Shell & Viewport:** Modern C++20 and Qt 6 desktop application with multi-tab workspace, continuous scrolling, high-DPI scaling, outline navigation, and text search.
- **Visual Page Organization:** Interactive page thumbnail grid for drag-and-drop reordering, 90°/180° rotation, page extraction, insertion, and page deletion.
- **Annotations & Markup:** Highlighting, underlining, squiggly, strikeout, sticky notes, geometric shapes, freehand ink with smoothing, and custom stamp annotations.
- **Interactive Forms:** Native AcroForm field detection and editing (text fields, checkboxes, radio groups, comboboxes, push buttons) with dynamic flattening.
- **Content Editing & OCR:** Direct text editing and typographical manipulation, paired with local offline Tesseract OCR text layer generator.
- **Security & Cryptography:** PKCS#7 / PAdES-BES digital signatures, X.509 certificate validation, AES-256 encryption, and multi-pass irreversible redaction.
- **Automated Interfaces:** Headless batch automation CLI, stdio Model Context Protocol (MCP) server, and localhost companion Remote Controller with bearer authentication.
- **Cross-Platform Packaging:** Windows (Inno Setup & Portable ZIP), Linux (.deb, PKGBUILD, .rpm, AppImage, Flatpak), macOS (.dmg), and Android (APK & AAB with Storage Access Framework).
- **Atomic Persistence:** Robust atomic file staging ensuring open documents are never corrupted in-place.
