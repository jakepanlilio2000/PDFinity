# PDFinity Installation and Deployment Guide

**Organization / Publisher:** [StackSync Solutions](https://www.stacksyncsolutions.com/)  
**Lead Programmer:** [Jake Ashley Panlilio](https://github.com/jakepanlilio2000)  
**License:** Mozilla Public License Version 2.0 (MPL-2.0)  
**Audience:** End Users, System Administrators, Enterprise IT Specialists, Packagers  
**Architecture:** Offline-First, Zero Cloud Dependency, Air-Gapped Compatible  

---

## 1. Overview

PDFinity is distributed across all major desktop and mobile operating systems:
- **Windows:** Standard Installer (`.exe`), Enterprise MSI (`.msi`), and Portable Zero-Footprint Archive (`.zip`).
- **Linux:** Debian/Ubuntu (`.deb`), Arch Linux (`PKGBUILD`), Fedora/RHEL/openSUSE (`.rpm`), universal AppImage (`.AppImage`), and sandboxed Flatpak (`com.stacksyncsolutions.pdfinity`).
- **macOS:** Apple Disk Image (`.dmg`) and signed Application Bundle (`.app`).
- **Android:** Android Application Package (`.apk`) supporting Storage Access Framework (SAF).

---

## 2. Windows Installation

### 2.1 Standard Setup Wizard (`PDFinity-Setup-x64.exe`)
Download and execute `PDFinity-Setup-x64.exe`. Follow the installation wizard steps to select destination directory and optional desktop icons.

#### Silent Automated Deployment Switches
| Switch | Description | Default |
| :--- | :--- | :--- |
| `/SILENT` | Displays the installation progress window without interactive dialogs. | Interactive GUI |
| `/VERYSILENT` | Completely suppresses the wizard UI; installs silently in the background. | Interactive GUI |
| `/SUPPRESSMSGBOXES` | Suppresses confirmation dialogs (answers 'Yes' or default). | Off |
| `/NORESTART` | Prevents automatic reboot even if files were locked. | Off |
| `/DIR="C:\Path"` | Overrides destination directory (`%PROGRAMFILES%\PDFinity`). | `%PROGRAMFILES%\PDFinity` |
| `/TASKS="desktopicon,fileassoc"` | Selects optional tasks (shortcuts and file associations). | `fileassoc` |
| `/LOG="install.log"` | Generates a verbose installer execution log. | None |

#### Silent Installation Command
```cmd
PDFinity-Setup-x64.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /TASKS="fileassoc" /LOG="C:\Temp\pdfinity_install.log"
```

---

### 2.2 Enterprise Active Directory MSI (`PDFinity-x64.msi`)
Deploy silently via Group Policy Objects (GPO) or Microsoft Endpoint Configuration Manager:
```cmd
msiexec /i PDFinity-x64.msi /qn /norestart /l*v "C:\Temp\pdfinity_msi.log"
```

---

### 2.3 True Portable Mode (`PDFinity-windows-x64-portable.zip`)
1. Extract `PDFinity-windows-x64-portable.zip` to a local folder or USB flash drive.
2. Ensure `portable.dat` is present in the extracted folder.
3. Launch `PDFinity.exe`.
   - All settings and caches are stored in the local `data/` subdirectory.
   - Zero registry entries are created or modified.

---

## 3. Linux Distribution Ecosystem

### 3.1 Debian / Ubuntu (`.deb`)
```bash
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

### 3.3 Fedora / RHEL / openSUSE (`.rpm`)
```bash
# Fedora / RHEL 8+
sudo dnf install ./pdfinity.x86_64.rpm

# openSUSE
sudo zypper install ./pdfinity.x86_64.rpm
```

### 3.4 Standalone AppImage
```bash
chmod +x PDFinity-x86_64.AppImage
./PDFinity-x86_64.AppImage
```

### 3.5 Sandboxed Flatpak
```bash
flatpak-builder --user --install --force-clean build-dir packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.yml
flatpak run com.stacksyncsolutions.pdfinity
```

---

## 4. macOS Installation

1. Mount the disk image:
   ```bash
   hdiutil attach PDFinity.dmg
   ```
2. Drag `PDFinity.app` into `/Applications`.
3. Launch `PDFinity` from Applications or Spotlight.

---

## 5. Android Mobile Installation

### 5.1 Standalone APK (`PDFinity-release.apk`)
Install directly via Android Debug Bridge (ADB) or mobile package manager:
```bash
adb install -r PDFinity-release.apk
```

### 5.2 Android App Bundle (`PDFinity-release.aab`)
For deploying or testing release app bundles locally via `bundletool`:
```bash
bundletool build-apks --bundle=PDFinity-release.aab --output=pdfinity.apks --mode=universal
bundletool install-apks --apks=pdfinity.apks
```

PDFinity on Android requests zero broad-storage permissions, leveraging the system Storage Access Framework (SAF) document picker to protect user data integrity.

---

## 6. Cryptographic Integrity Verification

Before deploying binary packages, verify SHA-256 checksums:

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
