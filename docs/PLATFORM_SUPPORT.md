# PDFinity: Target Platform Matrix and Support Specification

**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Status:** Official Platform Support Specification  

---

## 1. Platform Support Classification

PDFinity enforces an evidence-based platform classification model. Support is verified through automated CI pipelines and native packaging recipes across all target tiers:

| Target Platform | Architecture | Support Class | Primary Toolchain | UI Framework | Minimum OS Baseline | Packaging Target |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Windows 10/11** | x86_64 (`x64`) | **Primary Modern** | MSVC 19.4x/19.5x (VS 2022/2026), Windows SDK 10.0+ | Qt 6.8+ LTS | Windows 10 Version 1809 (Build 17763) | Inno Setup / WiX MSI / Portable ZIP |
| **Linux Desktop** | x86_64 (`x64`) | **Primary / Targeted** | GCC 11+ / Clang 14+, glibc 2.31+ | Qt 6.8+ LTS | Ubuntu 22.04+, Debian 12+, Fedora 39+, Arch | AppImage / Flatpak / `.deb` / Arch PKGBUILD / `.rpm` |
| **macOS** | Apple Silicon (`arm64`) & Intel (`x64`) | **Targeted** | Apple Clang (Xcode 15+), macOS SDK 14+ | Qt 6.8+ LTS | macOS 12.0 (Monterey) | DMG / App Bundle |
| **Android** | `arm64-v8a`, `armeabi-v7a`, `x86_64` | **Mobile Port** | Android NDK r25+, Gradle 8+, JDK 17/21 | Qt 6.8+ for Android | Android 9.0 (Pie, API level 28) | Release APK / Android App Bundle (AAB) |
| **Windows 10/11** | i686 (`x86`) | **Conditional Modern** | MSVC x86, Windows SDK 10.0+ | Qt 6.8+ LTS (x86 build) | Windows 10 Version 1809 (32-bit) | Inno Setup / Portable ZIP |
| **Windows 7 SP1** | i686 (`x86`) & x86_64 (`x64`) | **Legacy / Conditional** | MSVC `v141_xp` (VS 2017) or MinGW-w64 GCC | Qt 5.15 LTS (Legacy Profile) | Windows 7 SP1 + KB4474419 (SHA-2) + KB2999226 (UCRT) | Inno Setup Legacy / ZIP |

---

## 2. Verification Standards per Platform Tier

Release validation adheres to the following criteria according to target platform classification:

### 2.1 Primary Modern Targets (Windows 10/11 x64, Linux x64)
- **Compilation:** Clean build under CMake with zero fatal errors and zero unsuppressed compiler warnings (`/W4` on MSVC, `-Wall -Wextra` on GCC/Clang).
- **Automated Tests:** 100% pass rate on unit test suite, integration document lifecycle suite, and security regression suite.
- **Packaging:** Verified installer creation and uninstaller execution with complete removal of program files and registry entries.
- **Runtime Sanity:** Execution of UI shell, file open/save dialogs, multi-tab navigation, rendering, and background thread responsiveness.

### 2.2 Targeted Platforms (macOS)
- **Compilation:** Native universal binary (`x86_64` + `arm64`) or separate native architectures.
- **Platform Features:** macOS menu bar integration, App Sandbox compatibility, Retina high-DPI scaling, and native Cocoa file picker.
- **Code Signing:** Valid developer certificate signing and Apple notarization ticket stapling.

### 2.3 Mobile Targets (Android)
- **Storage Access Framework (SAF):** Full integration via JNI bridge and `PDFinityActivity.java` for reading and saving documents without broad storage permissions.
- **Touch & Gesture UI:** Bilinear-filtered pinch-to-zoom, momentum scrolling, and touch-friendly hit targets.
- **ABI Coverage:** Native `.so` libraries packaged for `arm64-v8a`, `armeabi-v7a`, and `x86_64`.

### 2.4 Legacy Compatibility Target (Windows 7 SP1)
- **Isolated Execution:** Must be built from the dedicated `legacy-win7` profile/branch.
- **Runtime Verification:** Execution on an authentic Windows 7 SP1 machine (physical or clean VM).
- **Dependency Audit:** Full verification of all bundled DLLs to ensure no unresolved imports from `api-ms-win-*` or missing kernel functions.
- **Rule of Record:** No release or promotional material will state Windows 7 is supported until exact test logs and screenshots from a validated Windows 7 environment are permanently recorded.
