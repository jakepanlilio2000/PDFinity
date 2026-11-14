# PDFinity: Target Platform Matrix and Capability Specification

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Platform Matrix  

---

## 1. Platform Support Classification

PDFinity enforces an evidence-based platform classification model. Support is never asserted based solely on theoretical source code portability; each platform class requires verifiable build, test, and packaging evidence.

| Target Platform | Architecture | Support Class | Primary Toolchain | UI Framework | Minimum OS Baseline | Packaging Target |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Windows 10/11** | x86_64 (`x64`) | **Primary Modern** | MSVC 19.4x/19.5x (VS 2022/2026), Windows SDK 10.0+ | Qt 6.8+ LTS | Windows 10 Version 1809 (Build 17763) | Inno Setup / WiX MSI / Portable ZIP |
| **Linux Desktop** | x86_64 (`x64`) | **Primary / Targeted** | GCC 11+ / Clang 14+, glibc 2.31+ | Qt 6.8+ LTS | Ubuntu 22.04+, Debian 12+, Fedora 39+, Arch | AppImage / Flatpak / `.deb` / Arch PKGBUILD / `.rpm` |
| **macOS** | Apple Silicon (`arm64`) & Intel (`x64`) | **Targeted** | Apple Clang (Xcode 15+), macOS SDK 14+ | Qt 6.8+ LTS | macOS 12.0 (Monterey) | DMG / App Bundle |
| **Android** | `arm64-v8a`, `armeabi-v7a`, `x86_64` | **Mobile Port** | Android NDK r25+, Gradle 8+, JDK 17/21 | Qt 6.8+ for Android | Android 8.0 (Oreo, API level 26) | Release APK / Android App Bundle (AAB) |
| **Windows 10/11** | i686 (`x86`) | **Conditional Modern** | MSVC x86, Windows SDK 10.0+ | Qt 6.8+ LTS (x86 build) | Windows 10 Version 1809 (32-bit) | Inno Setup / Portable ZIP |
| **Windows 7 SP1** | i686 (`x86`) & x86_64 (`x64`) | **Legacy / Conditional** | MSVC `v141_xp` (VS 2017) or MinGW-w64 GCC | Qt 5.15 LTS (Legacy Profile) | Windows 7 SP1 + KB4474419 (SHA-2) + KB2999226 (UCRT) | Inno Setup Legacy / ZIP |

---

## 2. Required Verification Evidence per Platform

In compliance with the project's engineering standards, release sign-offs require explicit evidence according to support classification:

### 2.1 Primary Modern Targets (Windows 10/11 x64, Linux x64)
- **Compilation:** Clean build under CMake with zero fatal errors and zero unsuppressed compiler warnings (`/W4` on MSVC, `-Wall -Wextra` on GCC/Clang).
- **Automated Tests:** 100% pass rate on unit test suite, integration document lifecycle suite, and security regression suite.
- **Packaging:** Verified installer creation and uninstaller execution with complete removal of program files and registry entries.
- **Runtime Sanity:** Execution of UI shell, file open/save dialogs, multi-tab navigation, rendering, and background thread responsiveness.

### 2.2 Targeted Platforms (macOS)
- **Compilation:** Native universal binary (`x86_64` + `arm64`) or separate native architectures.
- **Platform Features:** macOS menu bar integration, App Sandbox compatibility, Retina high-DPI scaling, and native Cocoa file picker.
- **Code Signing:** Valid developer certificate signing and Apple notarization ticket stapling.

### 2.3 Conditional Targets (Windows x86 32-bit)
- **Memory Audit:** Strict address-space testing verifying application stability under the 2 GB virtual address limit.
- **Large Document Resilience:** Graceful handling and out-of-memory prevention when loading large files (> 500 MB).

### 2.4 Legacy Compatibility Target (Windows 7 SP1)
- **Isolated Execution:** Must be built from the dedicated `legacy-win7` profile/branch.
- **Runtime Verification:** Execution on an authentic Windows 7 SP1 machine (physical or clean VM).
- **Dependency Audit:** Full verification of all bundled DLLs to ensure no unresolved imports from `api-ms-win-*` or missing kernel functions.
- **Rule of Record:** No release or promotional material will state Windows 7 is supported until exact test logs and screenshots from a validated Windows 7 environment are permanently recorded.

---

## 3. Platform Feature Capability Matrix

The following matrix documents feature availability across platforms. If a feature is not supported on a specific platform, the application **must disable the feature gracefully and never present a broken or nonfunctional control**.

| Functional Capability | Windows 10/11 x64 | Linux x64 | macOS (arm64/x64) | Windows x86 (Modern) | Windows 7 SP1 (Legacy) | Graceful Degradation Strategy |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Document Viewer & Multi-tab Navigation** | **Full** | **Full** | **Full** | **Full** | **Full** | Unified across all platforms via PDFium rendering. |
| **High-DPI Dynamic Scaling** | **Per-Monitor v2** | Wayland / X11 DPI | Native Retina | **Per-Monitor v2** | System DPI Only | Windows 7 uses static system DPI scaling; disables dynamic per-monitor listener. |
| **Page Organization (Merge/Split/Reorder)** | **Full** | **Full** | **Full** | **Full** | **Full** | Headless QPDF engine operates identically on all platforms. |
| **Content Editing & Object Modification** | **Full** | **Full** | **Full** | **Full** | **Full** | Internal vector and font engine operates independently of OS. |
| **AcroForms Detection & Editing** | **Full** | **Full** | **Full** | **Full** | **Full** | PDFium AcroForm engine standardized. |
| **Cryptographic Digital Signatures (PKCS#12)** | **Full** (WinCNG) | **Full** (OpenSSL) | **Full** (CommonCrypto) | **Full** (WinCNG) | **Conditional** (WinCrypt / OpenSSL) | Windows 7 falls back to OpenSSL or WinCrypt; shows warning if TLS 1.3 timestamping unavailable. |
| **Irreversible Multi-Pass Redaction** | **Full** | **Full** | **Full** | **Full** | **Full** | Pure domain algorithm with zero OS dependencies. |
| **OCR Text Layer Generation (Tesseract)** | **Full** (AVX2) | **Full** (AVX2) | **Full** (NEON/AVX) | **Conditional** (SSE2) | **Disabled by default** | If CPU lacks AVX/AVX2 or running on Win7 x86, OCR menu displays informative dialog and hides batch OCR button. |
| **System Native Print Dialog** | **Full** | **Full** (CUPS) | **Full** (NSPrint) | **Full** | **Full** | Native printing abstracted through `Qt6PrintSupport` / `QPrinter`. |
| **Local Model Context Protocol (MCP)** | **Full** (stdio) | **Full** (stdio) | **Full** (stdio) | **Full** (stdio) | **Disabled** | Windows 7 lacks modern secure local pipe IPC; feature disabled in configuration. |
| **Secure Remote Controller (LAN Paired)** | **Full** (TLS 1.3) | **Full** (TLS 1.3) | **Full** (TLS 1.3) | **Full** (TLS 1.3) | **Disabled** | Windows 7 lacks modern TLS 1.3 stack; Remote Controller menu entry hidden. |
| **Hardware-Accelerated Viewport** | **DirectX / OpenGL** | OpenGL / Vulkan | Metal / OpenGL | DirectX / OpenGL | Software / OpenGL 1.4 | Windows 7 defaults to software rasterizer if graphics driver lacks modern OpenGL. |

---

## 4. Platform Abstraction Layer (PAL) Architecture

To decouple the core document and UI logic from platform-specific APIs, all OS interactions must transit through explicit interface adapters defined in `src/platform/`:

```
+-----------------------------------------------------------------------------------+
|                                 Application Core                                  |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                          IPlatformAdapter (Platform API)                          |
+-----------------------------------------------------------------------------------+
      |                            |                                |
      v                            v                                v
+----------------------+  +-----------------------+  +------------------------------+
| WindowsPlatformAdapter|  | LinuxPlatformAdapter  |  | MacOSPlatformAdapter         |
+----------------------+  +-----------------------+  +------------------------------+
| - Per-Monitor DPI v2 |  | - XDG Desktop Specs   |  | - AppKit / Cocoa Bridges     |
| - WinCNG Crypto      |  | - OpenSSL Crypto      |  | - CommonCrypto / Keychain    |
| - Windows Shell Links|  | - SecretService / KWallet| - App Sandbox File Access    |
| - Path Sanitization  |  | - Posix File Locking  |  | - macOS Bundle File Resolvers|
+----------------------+  +-----------------------+  +------------------------------+
```

### Key Responsibilities of `IPlatformAdapter`
1. **Secure Path Handling:** Canonicalization of paths, detection of junction points, symlinks, volume mounts, and casing normalization.
2. **Secure Credentials Storage:** Platform-native keychains (Windows Credential Manager / DPAPI, macOS Keychain, Linux Secret Service / FreeDesktop Secret API).
3. **Display Scaling & Metrics:** Querying per-monitor DPI, screen refresh rates, and primary viewing geometry.
4. **Desktop Integration:** Native file association registration, taskbar progress reporting, dock bouncing, and recent file integration.
5. **Capability Querying:** Runtime query method `bool HasCapability(PlatformCapability cap)` allowing UI widgets to interrogate OS capabilities before presenting actions.

---

## 5. Graceful Degradation Rules

1. **Rule of Visibility:** Never display a menu item, toolbar button, or configuration tab that triggers an error or nonfunctional state on the host operating system.
2. **Rule of Safe Interrogation:** Every platform-dependent subsystem must implement a self-diagnostic check at startup.
3. **Rule of Informative Explanation:** If an optional capability (e.g., OCR or Digital Timestamping) cannot function due to missing hardware (e.g., AVX2 instructions) or missing OS updates, provide an explicit, non-blocking informational dialog explaining the exact missing prerequisite.
4. **Rule of Zero Crash:** Missing OS features (such as DirectWrite font enumeration or modern TLS 1.3) must trigger fallback codepaths or clean feature deactivation; they must never terminate the process.
