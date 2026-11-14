# Changelog

All notable changes to the **PDFinity** project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)  

---

## [1.0.8] - 2026-11-14

### Fixed
- **Right Inspector Selection Properties Geometry & Metadata Synchronization [BUG-U3-04]:** Added `GetSelectedAnnotationData()` to `PdfCanvasWidget` and connected live annotation geometry (X, Y, W, H), stroke color, fill color, stroke width, opacity, author, and note contents directly into `RightInspectorPanel`. Added regression coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.7] - 2026-11-14

### Fixed
- **Shell Splitter Sizes and Left/Right Panel State Persistence [BUG-U3-03]:** Added serialization and persistence for `leftRailDrawerExpanded`, `leftRailActivePanel`, `rightInspectorVisible`, and `viewerSplitterSizes` in `AppConfig` and `SettingsManager`. Automatically restores panel states on document mount and saves on application close. Added regression coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.6] - 2026-11-14

### Fixed
- **LeftNavRail Page Selection Sync on Canvas Page Change [BUG-U3-02]:** In `PdfViewerWidget::OnCanvasPageChanged`, replaced legacy dead sidebar call with `m_leftRail->SetActivePage(pageIndex)` and `m_inspector->UpdatePageContext(pageIndex, zoom)`, and scrubbed legacy sidebar calls in `Undo()` and `Redo()`. Added regression test in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.5] - 2026-11-14

### Fixed
- **Contextual Workflow Ribbon Action Slots Wiring [BUG-U3-01]:** Connected all View and Comment mode action signals (`ViewPanClicked`, `ViewSelectClicked`, `CommentHighlightClicked`, `CommentUnderlineClicked`, `CommentStrikeoutClicked`, `CommentNoteClicked`, `CommentDrawClicked`, `CommentShapesClicked`, `CommentCalloutClicked`, `CommentStampClicked`, `SaveClicked`) to live canvas annotation tool modes and atomic document persistence. Added regression coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.4] - 2026-11-14

### Added
- **Top Chrome & Multi-Tab Document Bar (`StudioTabBar`):**
  - Integrated top bar combining pinned Home tab, closable document tabs with dirty-state indicator (`●`), page count and file size tooltips, middle-click close, drag-to-reorder, tab overflow menu, search trigger (`Ctrl+F`), and instant theme toggle.
- **2-Tier Grouped Contextual Workflow Ribbon (`WorkflowRibbon`):**
  - 8 primary workflow mode tabs: `All Tools`, `View`, `Comment`, `Edit`, `Organize`, `Fill & Sign`, `Protect & Redact`, `Convert & OCR`.
  - Context-sensitive action strips with grouped icon+label controls and vertical dividers.
  - Persistent right-aligned Quick-Access strip providing instant access to *Save*, *Print*, *Undo*, *Redo*, and *Share/Export*.
- **Left Navigation Rail & Resizable Drawer (`LeftNavRail`):**
  - 44px vertical icon rail on far left featuring crimson active indicator bar (`3px solid #DC2626`).
  - 7 dedicated drawer panels: *Page Thumbnails* (multi-select grid with lazy rendering), *Bookmarks & Outline* (hierarchical tree), *Attachments* (embedded file list with export), *Layers (OCG)* (visibility checkboxes), *Comments List* (filterable annotation list), *Signatures & Certificates* (PKCS#7 verification tree), and *Document Info & Preflight* (live PDF standard, geometry, color space, and encryption specs).
  - Resizable drawer container with header title, close button, and expand/collapse toggle.
- **Right Contextual Tool & Property Inspector (`RightInspectorPanel`):**
  - 260px–340px collapsible workstation panel adapting dynamically between *All Tools Palette* (10 fast-access suite operations) and *Selection Properties* (boundary coordinates X/Y/W/H, stroke/fill color pickers, stroke width, opacity slider, author metadata, and deletion triggers).
  - Smooth collapse/expand toggle via header close button and `Ctrl+Shift+I` global shortcut.

---

## [1.0.3] - 2026-11-14

### Fixed
- **Brand Terminology Scrub in Security Documentation [BUG-U2-01]:** In `docs/SIGNATURES_AND_SECURITY.md`, scrubbed legacy third-party signature handler names and aligned with open ISO 32000-1 / ISO 32000-2 detached PKCS#7 signature terminology.

---

## [1.0.2] - 2026-11-14

### Added
- **Crimson-Slate Enterprise Design System (`ThemeTokens`):**
  - Centralized palette token system supporting seamless runtime switching between **Studio Dark (Warm Charcoal)** and **Studio Light (Crisp Daylight)**.
  - Studio Dark: App Chrome `#18181B`, Ribbon `#222428`, Side Panel `#1E2024`, Canvas Backdrop `#111214`, Surface `#27292E`, Border `#34373E`, Primary Text `#F4F4F5`, Secondary Text `#A1A1AA`.
  - Studio Light: App Chrome `#F3F4F6`, Ribbon `#FFFFFF`, Side Panel `#F9FAFB`, Canvas Backdrop `#E5E7EB`, Surface `#FFFFFF`, Border `#D1D5DB`, Primary Text `#111827`, Secondary Text `#4B5563`.
  - Brand Accent Tokens: Primary Crimson `#DC2626`, Hover Crimson `#B91C1C`, Soft Crimson Tint `rgba(220, 38, 38, 0.12)`, Focus Ring `#EF4444`.
  - Native typography stack (`Segoe UI`, `SF Pro Text`, `Inter`, `Roboto`) with structured type scale (11px, 12px, 13px, 16px, 18px) and 4px–8px border radii.
- **Crisp Vector SVG Icon Suite (`assets/icons/studio/`):**
  - 43 pixel-aligned 24x24 vector SVG icons with 1.6px stroke width, rounded caps/joins, and Crimson accents covering all core actions and toolbar tools.
- **Enhanced Pinned Home Dashboard (`HomeDashboardWidget`):**
  - Permanent left navigation shortcuts: *Recent*, *Starred / Pinned*, *Local Files*, and *Batch & Automate*.
  - 7 Hero Quick-Action Cards: *Open PDF*, *Create from Images*, *Combine Files*, *Edit PDF*, *Fill & Sign*, *Protect & Redact*, and *OCR Text Recognition*.
  - Sortable Recent Documents Table with pin toggle (`⭐`/`☆`), file path, last opened timestamp, formatted size, and quick removal actions.
  - Real-time search filter and interactive drag-and-drop landing target.

---

## [1.0.1] - 2026-11-14

### Added
- **Enterprise Document Studio Workstation:** Integrated modern 3-pane layout featuring:
  - **Pinned Home Dashboard:** Always-available Tab 0 featuring quick workflow cards, searchable recent documents table, 100% offline security guarantee badge, and drag-and-drop landing area.
  - **Grouped Contextual Workflow Ribbon:** Structured toolbars for Home & File, Assemble, Review & Annotate, Forms & Edit, Sign & Protect, and Tools & Automate.
  - **Collapsible Navigation Rail:** 48px slim left rail with expand/collapse drawer for thumbnails, document outline, and annotations.
  - **Contextual Document & All-Tools Inspector:** Right-hand dockable panel presenting document metadata, page dimensions, permissions, viewport zoom/rotation state, and 18-tool suite action palette.
  - **Floating Bottom HUD:** Semi-transparent glassmorphism floating overlay with page counter, navigation, quick zoom controls, fit width/page, and clockwise rotation.
  - **Vector SVG Suite Icons:** Custom crisp vector icon generator supporting 35 suite tools, ribbons, and actions.
- **Dedicated Console Companion Binary (`pdfinity-cli`):** Independent console binary for batch processing, MCP stdio server, remote controller client, and headless automation.

### Changed
- **Silent Native GUI Launch:** Configured GUI launcher to start completely silently without terminal console windows across Windows (`WIN32_EXECUTABLE TRUE` with `/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup`), macOS (`MACOSX_BUNDLE TRUE`), Linux (`Terminal=false`), and Windows subprocess creation (`CREATE_NO_WINDOW` / `0x08000000`).
- **Strict Brand Nomenclature Scrubbing:** Scrubbed all references to proprietary third-party PDF suites across UI, dialogs, error messages, and documentation, ensuring 100% compliance with ISO 32000 open standard terminology.

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
