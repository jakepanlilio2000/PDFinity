# PDFinity v1.1.0 — Enterprise Document Studio UI & Silent Desktop Launch

**StackSync Solutions** ([https://www.stacksyncsolutions.com/](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** [Mozilla Public License Version 2.0 (MPL-2.0)](LICENSE)

---

## Executive Overview

PDFinity **v1.1.0** represents a transformative milestone in the evolution of our enterprise offline-first PDF suite. Building on top of the robust v1.0.0 dual-engine foundation (Google PDFium + QPDF), v1.1.0 delivers a cohesive **Enterprise Document Studio workstation** designed for deep focus, ergonomic efficiency, and seamless adaptation across workstation monitors, compact laptop screens, tablets, and Android mobile devices.

---

## What's New in v1.1.0

### 1. Silent Native Desktop GUI Launch & CLI Separation (Phase U1)
- **Zero Console Flash:** Main GUI binary targets (`WIN32_EXECUTABLE`, `/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup`, `MACOSX_BUNDLE`, and Linux `.desktop` with `Terminal=false`) now launch with absolute silence—zero flashing terminal or console windows on desktop workstations.
- **Dedicated CLI Automation Wrapper:** Headless batch operations, MCP server stdio pipelines, and diagnostic smoke tests (`--headless-smoke`, `--version`, `--help`) operate smoothly via `pdfinity-cli` with intelligent console attachment.

### 2. Crimson-Slate Design Token System & Pinned Home Dashboard (Phase U2)
- **Centralized Design System:** Instant, flashless runtime switching between **Studio Dark (Warm Charcoal)** and **Studio Light (Crisp Daylight)** palettes with WCAG AA compliant text contrast and signature `#DC2626` crimson accent highlights.
- **Standalone SVG Iconography:** 100% vector-rendered crisp iconography across every studio action, eliminating bitmap blurring at any display scale.
- **Pinned Home Dashboard (Tab 0):** High-efficiency workspace tab featuring quick workflow launch cards (*Organize*, *Edit*, *Annotate*, *Forms*, *Redact*, *OCR*, *Intelligence*, *Batch*), recent document history with file size badges, and offline health status.

### 3. 3-Pane Document Studio Workstation Shell (Phase U3)
- **Studio Tab Bar:** Unified document bar with pinned Home tab, dirty-state dot indicators, page count tooltips, middle-click close, and tab overflow menus.
- **2-Tier Grouped Contextual Workflow Ribbon:** Compact ribbon featuring 8 primary mode tabs (`All Tools`, `View`, `Comment`, `Edit`, `Organize`, `Fill & Sign`, `Protect & Redact`, `Convert & OCR`) with contextually reactive tool strips.
- **Collapsible Left Navigation Rail & Drawer (240px):** Ultra-slim 44px icon rail paired with sliding drawers for Thumbnails, Bookmarks, Attachments, Layers, Comments, Signatures, and Preflight standards.
- **Contextual Right Tool & Property Inspector (280px):** Dynamic panel displaying real-time properties for selected markup, cryptographic PAdES digital certificates, and ISO compliance reporting.
- **Persistent Splitter Geometry:** Workspace layouts persist seamlessly across application restarts.

### 4. Elevated Document Sheet Rendering & Floating Canvas HUD (Phase U4)
- **Crisp Page Elevation:** Centered white document sheet surfaces with multi-layered soft drop shadows over neutral pasteboards (`#111214` dark / `#E5E7EB` light) ensuring page edges stand out clearly even on pure white documents.
- **Translucent Floating Canvas HUD:** Ergonomic bottom-center pill overlay featuring idle ambient auto-fading, direct page spinbox navigation, select/hand pointer modes, zoom presets (`50%`–`300%`, `Fit Width`, `Fit Page`), spread toggles, and 90° clockwise view rotation.
- **Alignment Rulers & Snap Guides:** High-precision horizontal and vertical measurement overlays (`mm`, `in`, `pt`) with closest-guide magnetic alignment snapping.
- **Non-Intrusive Studio Toasts:** Subtle, dismissible bottom-right notifications for routine document operations.

### 5. Responsive Mobile & Android Studio UI Adaptation (Phase U5)
- **Adaptive Breakpoint Layout Engine:** Dynamic layout transitions across `DesktopExpanded` (≥ 1100px), `TabletCompact` (700–1099px), and `PhoneMobile` (< 700px / Android handsets).
- **Mobile Top App Bar & Bottom Navigation Bar:** Clean 52px top bar and 56px bottom navigation bar with strict adherence to **≥ 48dp x 48dp** touch target ergonomics.
- **Smooth Mobile Bottom Sheet:** Slide-over modal sheet with drag handle, title, 48dp close button, outside-tap dismissal, and Escape/Back button support.
- **Touch Gestures & Virtual Keyboard Panning:** Smooth pinch-to-zoom, horizontal swipe page turns, smart double-tap zoom (1.0x ↔ 1.5x), 12.0dp annotation corner handles, and automatic scroll panning to prevent on-screen software keyboard obstruction.
- **Clean Android Storage Access Framework (SAF):** Full Android 9.0–15 compatibility with zero broad storage permission requirements and automated `versionCode` synchronization.

### 6. Refreshed Showcase Visuals & Documentation (Phase U6)
- High-resolution studio screenshots capturing Home Dashboard, Studio Dark Workstation, Tool Inspector, and Mobile Touch Shell.
- Comprehensively updated `README.md` and `docs/USER_GUIDE.md`.

---

## Security & Architecture Invariants

1. **100% Offline Core:** PDFinity makes zero outbound network connections, transmits zero telemetry, and operates completely air-gapped.
2. **Atomic Safe File Serialization:** Files are never modified in-place; all writes execute through isolated staging files and atomic filesystem swaps with cross-reference validation.
3. **Dual-Engine Precision:** Google PDFium provides sub-pixel rendering and text geometry, while QPDF guarantees lossless structural object manipulation and linearization.
4. **License & Freedom:** Licensed under the Mozilla Public License Version 2.0 (MPL-2.0).
