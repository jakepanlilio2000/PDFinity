# Changelog

All notable changes to the **PDFinity** project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)  

---

## [1.1.0] — Enterprise Document Studio UI & Silent Desktop Launch - 2026-11-14

### Added
- **Silent Desktop Launch & CLI Automation Separation (Phase U1):**
  - Native GUI binary subsystem configuration (`WIN32_EXECUTABLE`, `/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup`, `MACOSX_BUNDLE`, and Linux `.desktop`) eliminating console/terminal flashes on desktop launch.
  - Preserved full command-line batch and automation capabilities via dedicated `pdfinity-cli` binary and dual-subsystem console attachment.
- **Crimson-Slate Enterprise Design System & Home Dashboard (Phase U2):**
  - Modern studio color tokens supporting instant runtime switching between **Studio Dark** (`#18181B` chrome, `#222428` ribbon, `#111214` backdrop, `#27292E` card surface, `#DC2626` crimson accent) and **Studio Light** (`#F3F4F6` chrome, `#FFFFFF` ribbon, `#E5E7EB` backdrop).
  - Pinned Tab 0 Home Dashboard with 8 primary workflow cards, recent document history with file size badges, and instant offline health telemetry.
  - Comprehensive standalone crisp SVG vector icon resource catalog (`VectorIcons`).
- **3-Pane Document Studio Workstation Shell (Phase U3):**
  - Integrated `StudioTabBar` combining the pinned Home tab, closable document tabs with dirty indicators, page count tooltips, and overflow menus.
  - 2-Tier grouped contextual `WorkflowRibbon` with 8 primary mode tabs (`All Tools`, `View`, `Comment`, `Edit`, `Organize`, `Fill & Sign`, `Protect & Redact`, `Convert & OCR`) and dynamic tool strips.
  - Collapsible 44px `LeftNavRail` and 240px drawer with fast switching between Thumbnails, Bookmarks, Attachments, Layers, Comments, Signatures, and Preflight.
  - Contextual `RightInspectorPanel` (280px) providing live property editing for markups, digital certificates, and ISO compliance.
  - Splitter state persistence across application sessions.
- **Elevated Document Sheet Rendering & Floating Canvas HUD (Phase U4):**
  - High-DPI centered page elevation with multi-layered soft drop shadows over neutral pasteboard.
  - Translucent rounded bottom control pill (`FloatingCanvasHud`) with idle ambient auto-fading, direct page spinbox navigation, select/hand pointer modes, zoom presets, layout spreads, and 90° clockwise rotation.
  - Optional toggleable alignment rulers (`mm`, `in`, `pt`), alignment snap guides, and non-intrusive `StudioToast` notifications.
- **Responsive Mobile & Android Studio UI Adaptation (Phase U5):**
  - 3-Tier responsive breakpoint layout engine (`DesktopExpanded` ≥ 1100px, `TabletCompact` 700–1099px, `PhoneMobile` < 700px).
  - Touch-optimized 52px `MobileTopAppBar`, 56px `MobileBottomNavBar`, and smooth slide-up `MobileBottomSheet` tools palette.
  - Minimum 48dp x 48dp touch targets, pinch-to-zoom, swipe page turns, smart double-tap zoom (1.0x ↔ 1.5x), 12.0dp annotation handles, and virtual keyboard pan avoidance.
  - Clean desktop capability gating for Android with Storage Access Framework (SAF) integration and synchronized `versionCode`.
- **Showcase Screenshots & Documentation Refresh (Phase U6):**
  - Generated high-resolution showcase screenshots (`studio-home.png`, `studio-workspace-dark.png`, `studio-tools-inspector.png`, `studio-mobile.png`).
  - Updated `README.md` and `docs/USER_GUIDE.md` matching the new Studio workstation architecture.

---

## [1.0.14] - 2026-11-14

### Fixed
- **Mobile Bottom Sheet Outside Tap & Escape Dismissal [BUG-U5-03]:**
  - Added parent widget mouse event filter to detect clicks/taps outside bottom sheet geometry and dismiss the sheet automatically, eliminating trapped modal states.
  - Implemented `keyPressEvent` handling for `Qt::Key_Escape` (and device hardware back buttons) to dismiss open bottom sheets smoothly.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.
- **Desktop Chrome Suppression on Mobile Viewports [BUG-U5-04]:**
  - Automatically hidden desktop `QMenuBar` and `QStatusBar` when transitioning into `ViewportClass::PhoneMobile` (< 700px), eliminating dual headers and recovering vertical reading area on handset displays while preserving clean restoration upon expanding to tablet or desktop widths.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.
- **Studio Tab Bar Corner Controls Adaptation [BUG-U5-05]:**
  - Added `SetCornerControlsVisible()` to `StudioTabBar` and connected viewport transition engine to hide corner action buttons on narrow mobile viewports, preventing tab title squeezing and crowding.
  - Synchronized Android `versionCode` calculation in `scripts/bump_version.py` across `android/build.gradle`, `packaging/android/build.gradle`, `android/AndroidManifest.xml`, and `packaging/android/AndroidManifest.xml`.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.13] - 2026-11-14

### Fixed
- **Mobile Top App Bar Tab Switching Synchronization [BUG-U5-01]:**
  - Connected `StudioTabBar::currentChanged` in `DocumentWorkspace` to emit `CurrentDocumentChanged(title)`, synchronizing active document titles and status bar indicators immediately upon tab changes and returning to `"PDFinity Studio"` when viewing the Home dashboard.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.
- **Mobile Bottom Sheet Signal Accumulation & Content Lifecycle [BUG-U5-02]:**
  - Bound `m_slideAnimation::finished` dismiss handler once in `MobileBottomSheet` constructor instead of creating cumulative lambda connections on every invocation of `HideSheet()`.
  - Added cleanup for previous custom widget contents via `deleteLater()` in `ShowSheet()`, preventing child widget leaks during palette navigation.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.12] - 2026-11-14

### Added
- **Responsive Breakpoint Layout Engine (`ResponsiveLayoutManager`):**
  - Centralized 3-tier viewport adaptation system:
    * `DesktopExpanded` (width >= 1100px): Full 3-pane Studio workstation layout (Left Rail + Drawer, Top Workflow Ribbon, Right Tool/Property Inspector, Floating Bottom HUD).
    * `TabletCompact` (700px <= width < 1100px): Auto-collapses left drawer and right inspector into compact margins, maximizing document canvas reading width.
    * `PhoneMobile` (width < 700px or Android handset): Replaces desktop ribbon with touch-optimized `MobileTopAppBar` and 56px `MobileBottomNavBar`.
- **Mobile Top App Bar (`MobileTopAppBar`):**
  - Touch-friendly 52px top bar featuring Home/Back navigation button, middle-elided document filename header, quick Search trigger, theme switcher, and overflow action menu (Save, Share, Print, Preflight, Settings, About).
- **Mobile Bottom Navigation Bar (`MobileBottomNavBar`):**
  - 56px bottom navigation bar with 48dp x 48dp minimum touch targets for primary studio workflows: *Tools*, *Comment*, *Pages*, *Fill & Sign*, and *More*.
- **Mobile Studio Bottom Sheet (`MobileBottomSheet`):**
  - Smooth animated slide-over bottom sheet with drag handle, title bar, 48dp close button, and card-based touch buttons for tools palette and studio actions.
- **Touch Gestures & Virtual Keyboard Panning:**
  - Added smart double-tap zoom (toggling between 1.0x and 1.5x zoom) and enlarged annotation corner resize handles to 12.0dp for finger manipulation.
  - Connected `QInputMethod::keyboardRectangleChanged` so active document forms and inputs smoothly pan above the on-screen software keyboard.
- **Platform Capability Gating:**
  - Desktop-only controls (stdio MCP server toggles, desktop CLI references) cleanly hidden on mobile viewports without empty separators or disabled items.

---

## [1.0.11] - 2026-11-14

### Fixed
- **Canvas HUD Scrollbar Clearance, Layout Sync & Page Rotation Geometry [BUG-U4-03]:**
  - Updated `FloatingCanvasHud::Reposition()` to dynamically anchor against inner `QScrollArea::viewport()` geometry, ensuring centered alignment across split panels and full clearance above horizontal scrollbars.
  - Added `ReloadPageSizes()` to `PdfCanvasWidget` and `RotateClockwise()` to `PdfViewerWidget` to refresh aspect ratio and canvas layout rects upon 90° view rotation, eliminating sheet geometry distortion.
  - Added two-way signal synchronization between `PdfViewerWidget::SetLayout` and `FloatingCanvasHud` layout mode toggles with recursion guards.
  - Added regression test coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.10] - 2026-11-14

### Fixed
- **Canvas HUD Viewport Wake-Up & Custom Zoom Input [BUG-U4-01]:** Connected scroll area viewport to HUD event filter so canvas mouse movements immediately restore HUD ambient opacity, enabled editable mode on the zoom dropdown with Enter key commit, and synchronized non-preset zoom ratios properly. Added regression coverage in `tests/ui/test_shell_smoke.cpp`.
- **Ruler Tick Scaling & Closest Guide Snapping [BUG-U4-02]:** Added adaptive tick frequency scaling to prevent tick clustering or label overlap at extreme zoom levels (< 25% or > 200%), and updated snap calculation to resolve to the closest alignment guide within threshold rather than first found. Added regression coverage in `tests/ui/test_shell_smoke.cpp`.

---

## [1.0.9] - 2026-11-14

### Added
- **Elevated Document Sheet Rendering (`PdfCanvasWidget`):**
  - Crisp white sheet surface centered on neutral pasteboard backdrop (`#111214` dark / `#E5E7EB` light) with multi-layered soft drop shadow (ambient diffusion, medium elevation, and contact shadow) and subtle 1px hairline border (`#34373E` / `#D1D5DB`).
  - High-DPI device pixel ratio rendering support for razor-sharp vector rendering and crisp typography across 1.25x, 1.5x, 2.0x, and 3.0x displays.
  - Interactive alignment rulers (`Points`, `Inches`, `Millimeters`) and crimson alignment snap guides for edit and layout operations.
- **Translucent Floating Canvas HUD (`FloatingCanvasHud`):**
  - Modern rounded pill overlay anchored at viewport bottom-center with smooth idle ambient auto-fading (3.5s timeout to 0.18 opacity) and instant wakeup on mouse/keyboard interaction.
  - Page navigation controls: Previous (`▲`), Direct Page SpinBox (`[ 1 ] / N`), Next (`▼`).
  - Pointer mode selector: Select Tool (`V`) and Hand Pan Dragging Tool (`H`) with kinetic scrollbar integration.
  - Zoom controls: Zoom Out (`−`), Preset Selector (`50%`, `75%`, `100%`, `125%`, `150%`, `200%`, `300%`, `Fit Width`, `Fit Page`), Zoom In (`+`).
  - Page layout & rotation: Single Page (`1`), Continuous Scroll (`≡`), Facing Spread (`⚏`), and 90° Clockwise Rotation (`↷`).
- **Non-Intrusive Studio Toast Feedback (`StudioToast`):**
  - Replaced jarring modal dialogs for routine operations (document saving, text copying, mark clearing, and sanitization) with dismissible bottom-right toast notifications featuring subtle type accent bars and fade animations.

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
