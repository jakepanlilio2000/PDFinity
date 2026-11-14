# PDFinity Build and Compilation Guide

**Publisher:** StackSync Solutions ([stacksyncsolutions.com](https://www.stacksyncsolutions.com/))  
**Lead Programmer:** Jake Ashley Panlilio ([github.com/jakepanlilio2000](https://github.com/jakepanlilio2000))  
**Primary License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Supported Platforms:** Windows 10/11 x64, Linux x64, macOS Universal, Android 8.0+ (API 26+)  

---

## 1. System Prerequisites

### Windows 10/11 x64 (Primary Host Target)
- **C++ Compiler:** Visual Studio 2022 or 2026 (MSVC v19.3x / v19.5x with C++20 standard support).
- **Windows SDK:** Windows 10 SDK (Version 10.0.17763.0 or later, e.g. 10.0.26100.0).
- **Build Generator:** CMake 3.25 or later.
- **Build Tool:** Ninja 1.11 or later.
- **UI Framework:** Qt 6.8.2 LTS (MSVC 2022/2026 64-bit) installed at `C:/Qt/6.8.2/msvc2022_64` (or configured via `CMAKE_PREFIX_PATH`).

### Linux x64 (Ubuntu 22.04 / 24.04, Debian 12, Fedora 39/40)
- **Compiler:** GCC 11+ or Clang 14+ (`g++ -std=c++20`).
- **Tools:** CMake 3.25+, Ninja (`ninja-build`).
- **Packages:** `qt6-base-dev`, `libgl1-mesa-dev`, `libxkbcommon-dev`.

### macOS (Apple Silicon arm64 & Intel x64)
- **Compiler:** Apple Clang (Xcode 15+).
- **Tools:** CMake 3.25+, Ninja.
- **UI Framework:** Qt 6.8+ LTS.

---

## 2. Quick Build Using CMake Presets

PDFinity provides standardized `CMakePresets.json` configurations for reproducible multi-platform builds.

### 2.1 Windows x64 Build Instructions

Launch the **Visual Studio Developer Command Shell** (or run `VsDevCmd.bat -arch=x64`):

```cmd
:: 1. Configure the project using the Release preset
cmake --preset windows-x64-release

:: 2. Build all targets (libraries, executable, and tests)
cmake --build --preset windows-x64-release

:: 3. Run automated unit and UI smoke tests
ctest --preset windows-x64-release

:: 4. Launch the application
.\build\windows-x64-release\bin\PDFinity.exe
```

For a Debug build with symbols:
```cmd
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

### 2.2 Linux x64 Build Instructions

```bash
# Configure, build, and test
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release
xvfb-run ctest --preset linux-x64-release
./build/linux-x64-release/bin/PDFinity
```

### 2.3 macOS Build Instructions

```bash
cmake --preset macos-release
cmake --build --preset macos-release
ctest --preset macos-release
./build/macos-release/bin/PDFinity.app/Contents/MacOS/PDFinity
```

---

## 3. Command-Line Options and Verification

The compiled binary supports diagnostic and automation flags:

```cmd
:: Show application version and license info
.\build\windows-x64-release\bin\PDFinity.exe --version

:: Display help and options
.\build\windows-x64-release\bin\PDFinity.exe --help

:: Run headless diagnostic and exit with status 0
.\build\windows-x64-release\bin\PDFinity.exe --headless-smoke

:: Open a specific PDF document directly
.\build\windows-x64-release\bin\PDFinity.exe "C:\Path\To\Document.pdf"
```

---

## 4. Legacy Windows 7 SP1 Profile Notes

Windows 7 SP1 is supported via the isolated **Profile B** compatibility configuration. 
- Qt 6 is incompatible with Windows 7; Profile B requires Qt 5.15 LTS and the MSVC `v141_xp` toolchain.
- Do not build Profile B using the modern presets. Dedicated build scripts will be provided upon physical/VM test environment provisioning.
