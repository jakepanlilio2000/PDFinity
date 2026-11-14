# PDFinity Enterprise Deployment and Distribution Guide

**Publisher / Organization:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Audience:** System Administrators, Enterprise IT Specialists, DevOps Engineers, Packagers  
**Architecture:** Offline-First, Zero Cloud Dependency, Air-Gapped Compatible  

---

## 1. Executive Summary

PDFinity is an offline-first desktop and mobile PDF productivity suite engineered for security-conscious, enterprise, air-gapped, and privacy-focused workstations. It requires no continuous internet connectivity, executes zero background telemetry, and contains no automatic background updaters.

This document details distribution and deployment strategies across all supported operating systems and package ecosystems:
- **Windows:** Enterprise Setup (`.exe` via Inno Setup), Enterprise MSI (`.msi` via WiX Toolset), and Zero-Footprint Portable (`.zip`).
- **Linux:** Debian/Ubuntu (`.deb`), Arch Linux (`PKGBUILD`), Fedora/RHEL/openSUSE (`.rpm`), Standalone AppImage (`.AppImage`), and Flatpak (`com.stacksyncsolutions.pdfinity`).
- **macOS:** Apple Disk Image (`.dmg`) and signed application bundle (`.app`).
- **Android:** Android Application Package (`.apk`) supporting modern Storage Access Framework (SAF).

---

## 2. Windows Deployment

### 2.1 Enterprise Setup Installer (`PDFinity-Setup-x64.exe`)
Built with Inno Setup, supporting silent automation switches for mass deployment via Microsoft Endpoint Configuration Manager (SCCM), Intune, or Group Policy.

#### Silent Command-Line Installation Switches
| Switch | Purpose | Default |
| :--- | :--- | :--- |
| `/SILENT` | Displays the installation wizard progress window without interactive dialogs. | Interactive GUI |
| `/VERYSILENT` | Completely suppresses wizard UI; executes silently in background. | Interactive GUI |
| `/SUPPRESSMSGBOXES` | Suppresses confirmation dialogs (answers 'Yes' or default). | Off |
| `/NORESTART` | Prevents automatic reboot even if files were locked. | Off |
| `/DIR="C:\Path"` | Overrides the target directory (`%PROGRAMFILES%\PDFinity`). | `%PROGRAMFILES%\PDFinity` |
| `/TASKS="desktopicon,fileassoc"` | Selects optional tasks: desktop shortcut (`desktopicon`) and PDF file association (`fileassoc`). | `fileassoc` |
| `/LOG="install.log"` | Generates a verbose installer execution log. | None |

#### Silent Enterprise Installation Example
```cmd
PDFinity-Setup-x64.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /TASKS="fileassoc" /LOG="C:\Temp\pdfinity_install.log"
```

#### Silent Uninstallation
```cmd
"%PROGRAMFILES%\PDFinity\unins000.exe" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART
```

---

### 2.2 Enterprise MSI Package (`PDFinity-x64.msi`)
Built with WiX Toolset v4/v5 for native Active Directory GPO distribution and Windows Installer service management:
```cmd
msiexec /i PDFinity-x64.msi /qn /norestart /l*v "C:\Temp\pdfinity_msi.log"
```

---

### 2.3 True Portable Mode (`PDFinity-windows-x64-portable.zip`)
For restricted environments, USB drives, or air-gapped workstations where writing to `%PROGRAMFILES%` or the Windows Registry is restricted:
- **Isolation Marker:** The presence of `portable.dat` in the binary root activates full portable isolation.
- **Data Storage:** All application data, caches, and configuration are maintained in `<app_dir>\data`. Zero keys are written to the Windows Registry.

---

## 3. Linux Distribution Ecosystem

PDFinity adheres to the Freedesktop.org XDG Base Directory Specification:
- Config: `$XDG_CONFIG_HOME/PDFinity` (defaults to `~/.config/PDFinity`)
- Data: `$XDG_DATA_HOME/PDFinity` (defaults to `~/.local/share/PDFinity`)
- Cache: `$XDG_CACHE_HOME/PDFinity` (defaults to `~/.cache/PDFinity`)

### 3.1 Debian / Ubuntu (`.deb`)
```bash
# Install via APT or dpkg
sudo apt install ./pdfinity_amd64.deb
# or
sudo dpkg -i pdfinity_amd64.deb
sudo apt-get install -f
```

### 3.2 Arch Linux (`PKGBUILD`)
```bash
cd packaging/linux/arch
makepkg -si
```

### 3.3 Red Hat Enterprise Linux / Fedora / openSUSE (`.rpm`)
```bash
# Fedora / RHEL 8+
sudo dnf install ./pdfinity.x86_64.rpm

# openSUSE
sudo zypper install ./pdfinity.x86_64.rpm
```

### 3.4 Standalone AppImage (`.AppImage`)
```bash
chmod +x PDFinity-x86_64.AppImage
./PDFinity-x86_64.AppImage
```

### 3.5 Flatpak (`com.stacksyncsolutions.pdfinity`)
```bash
# Build & install locally
flatpak-builder --user --install --force-clean build-dir packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.yml

# Run
flatpak run com.stacksyncsolutions.pdfinity
```

---

## 4. macOS Deployment

### 4.1 Disk Image (`PDFinity.dmg`)
The macOS DMG contains the standardized `PDFinity.app` bundle configured for drag-and-drop installation into `/Applications`.

### 4.2 Standard Directory Structure
- **Settings & Data:** `~/Library/Application Support/PDFinity`
- **Caches:** `~/Library/Caches/PDFinity`
- **Documents:** `~/Documents`

---

## 5. Android Mobile & Tablet Deployment

### 5.1 Android Package (`.apk`)
The Android APK provides touch-first PDF viewing, form-filling, annotation, and redaction:
```bash
adb install -r dist/PDFinity-release.apk
```

### 5.2 Storage Access Framework (SAF)
Android integrates with the system document picker (`ACTION_OPEN_DOCUMENT` and `ACTION_CREATE_DOCUMENT`), isolating documents and ensuring complete compatibility with Android 11+ scoped storage requirements.

---

## 6. Offline Update & Integrity Verification

PDFinity contains **zero automatic background updaters** or network-initiated polling threads. All updates must be explicitly deployed by administrators.

### Cryptographic Checksums (SHA-256)
Always verify package integrity prior to enterprise rollout:

```powershell
# Windows PowerShell
Get-FileHash -Algorithm SHA256 .\PDFinity-Setup-x64.exe
Get-FileHash -Algorithm SHA256 .\PDFinity-windows-x64-portable.zip
```

```bash
# Linux / macOS
sha256sum PDFinity-x86_64.AppImage
sha256sum pdfinity_amd64.deb
sha256sum pdfinity.x86_64.rpm
shasum -a 256 PDFinity.dmg
```
