# Changelog

All notable changes to the **PDFinity** project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)  

---

## [1.0.0] - 2026-11-14 (Official Stable Release)

### Added
- **First Official Stable Release:** Production-ready release of PDFinity across Windows x64, Linux distributions (.deb, Arch, RPM, AppImage, Flatpak), macOS, and Android (API 28-35).
- **Multi-Platform Desktop & Mobile Packaging:** Native installer and portable packaging for Windows (Inno Setup / WiX MSI / Portable ZIP), Linux (Debian, Arch Linux, RPM, AppImage, Flatpak), macOS App Bundle, and Android APK/AAB with Storage Access Framework (SAF) integration.
- **Audited Security & Reliability:** 100% pass on all 51 automated unit, integration, UI, and security test suites. Verified zero broad-storage permissions on Android, thread-safe MCP stdio communication, encrypted remote controller pairing, and cryptographically verified digital signatures (PKCS#7).

---

## [0.1.10-Beta] - 2026-11-14

### Fixed
- **Transformation Job Queue Concurrency & Token Safety [BUG-005]:** In `src/services/TransformationJobQueue.cpp`, eliminated raw pointer capture across mutex lock scope in `RunJobSynchronously`, encapsulated `executeFunc` copy inside `m_mutex`, and guaranteed non-null cancellation token resolution to prevent use-after-free and data races under concurrent job enqueuing and pruning. Added regression test `testJobQueueConcurrencyAndTokenSafety` in `test_transformation_pipeline.cpp`.

---

## [0.1.9-beta] - 2026-11-14

### Fixed
- **Flatpak Manifest Duplicate Key & Dynamic AppImage Versioning [BUG-004]:** In `packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.yml`, eliminated duplicate `app-id` key adhering to strict YAML 1.2 rules. In `packaging/linux/appimage/build-appimage.sh`, replaced hardcoded fallback version with dynamic project version extraction from `CMakeLists.txt`.

---

## [0.1.8-beta] - 2026-11-14

### Fixed
- **Remote Server Socket Buffer Synchronization [BUG-003]:** In `RemoteControllerServer.cpp`, guarded all `m_socketBuffers` operations in `onNewConnection` and `onSocketReadyRead` with `m_mutex`, extracted complete lines under the lock, and processed network dispatches outside the critical section to prevent data races and deadlocks.

---

## [0.1.7-beta] - 2026-11-14

### Fixed
- **MCP Client Name Synchronization & Thread Safety [BUG-002]:** In `McpServer.cpp`, encapsulated all `m_state.clientName` reads and audit logging under `m_mutex` synchronization, eliminating potential data race conditions during concurrent JSON-RPC requests.

---

## [0.1.6-beta] - 2026-11-14

### Fixed
- **Assembly Relative Output Path Normalization [BUG-001]:** In `DocumentAssemblyService.cpp`, added fallback to `current_path()` when `outputPath.parent_path()` is empty, enabling seamless merging and page extraction targeting bare relative filenames in the active working directory.

---

## [1.0.0] - 2026-11-14 (Stable Release)

### Added
- **First Official Stable Release:** Production-ready release of PDFinity across Windows x64, Linux distributions (.deb, Arch, RPM, AppImage, Flatpak), macOS, and Android (API 28-35).
- **Multi-Platform Desktop & Mobile Packaging:** Native installer and portable packaging for Windows (Inno Setup / WiX MSI / Portable ZIP), Linux (Debian, Arch Linux, RPM, AppImage, Flatpak), macOS App Bundle, and Android APK/AAB with Storage Access Framework (SAF) integration.
- **Audited Security & Reliability:** 100% pass on all 51 automated unit, integration, UI, and security test suites. Verified zero broad-storage permissions on Android, thread-safe MCP stdio communication, encrypted remote controller pairing, and cryptographically verified digital signatures (PKCS#7).

---

## [0.1.10-Beta] - 2026-11-14

### Fixed
- **Transformation Job Queue Concurrency & Token Safety [BUG-005]:** In `src/services/TransformationJobQueue.cpp`, eliminated raw pointer capture across mutex lock scope in `RunJobSynchronously`, encapsulated `executeFunc` copy inside `m_mutex`, and guaranteed non-null cancellation token resolution to prevent use-after-free and data races under concurrent job enqueuing and pruning. Added regression test `testJobQueueConcurrencyAndTokenSafety` in `test_transformation_pipeline.cpp`.

---

## [0.1.9-beta] - 2026-11-14

### Fixed
- **Flatpak Manifest Duplicate Key & Dynamic AppImage Versioning [BUG-004]:** In `packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.yml`, eliminated duplicate `app-id` key adhering to strict YAML 1.2 rules. In `packaging/linux/appimage/build-appimage.sh`, replaced hardcoded fallback version with dynamic project version extraction from `CMakeLists.txt`.

---

## [0.1.8-beta] - 2026-11-14

### Fixed
- **Remote Server Socket Buffer Synchronization [BUG-003]:** In `RemoteControllerServer.cpp`, guarded all `m_socketBuffers` operations in `onNewConnection` and `onSocketReadyRead` with `m_mutex`, extracted complete lines under the lock, and processed network dispatches outside the critical section to prevent data races and deadlocks.

---

## [0.1.7-beta] - 2026-11-14

### Fixed
- **MCP Client Name Synchronization & Thread Safety [BUG-002]:** In `McpServer.cpp`, encapsulated all `m_state.clientName` reads and audit logging under `m_mutex` synchronization, eliminating potential data race conditions during concurrent JSON-RPC requests.

---

## [0.1.6-beta] - 2026-11-14

### Fixed
- **Assembly Relative Output Path Normalization [BUG-001]:** In `DocumentAssemblyService.cpp`, added fallback to `current_path()` when `outputPath.parent_path()` is empty, enabling seamless merging and page extraction targeting bare relative filenames in the active working directory.

---

## [0.1.5-beta] - 2026-11-14

### Fixed
- **Mobile Responsive Ergonomics & Desktop Action Suppression:** Relocated the mobile toolbar to `Qt::BottomToolBarArea` for ergonomic thumb accessibility on mobile/touch form factors, disabled desktop-only CLI/stdio actions (`m_mcpManagerAction`, `m_batchCenterAction`, `m_batchProfileEditorAction`, `m_exitAction`) on Android to eliminate dead buttons, and provided `app/platform/android/` architecture forwarding references.

---

## [0.1.4-beta] - 2026-11-14

### Fixed
- **Android Lifecycle State Persistence:** In `AndroidPlatformAdapter.cpp`, wired `app::AppContext::Instance().SaveCurrentState()` directly into `Qt::ApplicationSuspended`, `ApplicationHidden`, and `ApplicationInactive` handlers, guaranteeing immediate session and document recovery state persistence before background process termination.

---

## [0.1.3-beta] - 2026-11-14

### Fixed
- **SAF Descriptor Read & Atomic Staged Writeback:** In `PDFinityActivity.java`, routed all content URI reads explicitly through `ParcelFileDescriptor` (`openFileDescriptor(uri, "r")`) and staged document writebacks through a verified temporary file in `context.getCacheDir()` before flushing to avoid document corruption on write interruption.

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
