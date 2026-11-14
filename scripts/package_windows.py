#!/usr/bin/env python3
"""
PDFinity - Windows Packaging and Deployment Runner
Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
License: Mozilla Public License Version 2.0 (MPL-2.0)
"""

import os
import sys
import shutil
import subprocess
import hashlib
import zipfile
import re
import argparse

def get_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def detect_version(root_dir):
    cmake_path = os.path.join(root_dir, "CMakeLists.txt")
    if os.path.exists(cmake_path):
        with open(cmake_path, "r", encoding="utf-8") as f:
            content = f.read()
            m = re.search(r"project\s*\(\s*PDFinity\s+VERSION\s+([0-9\.]+)", content, re.IGNORECASE)
            if m:
                return m.group(1)
    return "1.1.0"

def main():
    parser = argparse.ArgumentParser(description="PDFinity Windows Packaging Utility")
    parser.add_argument("--version", default=None, help="Target release version string")
    parser.add_argument("--skip-installer", action="store_true", help="Skip Inno Setup compilation")
    parser.add_argument("--build-msi", action="store_true", help="Compile WiX MSI package")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    os.chdir(root)

    version = args.version or detect_version(root)

    print("==================================================")
    print(" PDFinity Windows Packaging and Deployment Runner")
    print(" Publisher: StackSync Solutions")
    print(" Lead Programmer: Jake Ashley Panlilio")
    print(f" Target Version: {version}")
    print("==================================================")

    bin_dir = os.path.join(root, "build", "windows-x64-release", "bin")
    exe_src = os.path.join(bin_dir, "PDFinity.exe")
    pdfium_src = os.path.join(bin_dir, "pdfium.dll")

    dist_dir = os.path.join(root, "dist")
    staged_dir = os.path.join(dist_dir, "staged-windows-x64")
    portable_dir = os.path.join(dist_dir, f"PDFinity-{version}-windows-x64-portable")

    if not os.path.exists(exe_src):
        # Check if pre-staged binaries exist
        if os.path.exists(os.path.join(staged_dir, "PDFinity.exe")):
            print(f"[!] Warning: Build binary not found at {exe_src}, using existing staged files in {staged_dir}")
        else:
            print(f"Error: Executable not found at {exe_src} and staged binaries missing. Please build first.")
            sys.exit(1)
    else:
        if os.path.exists(staged_dir):
            shutil.rmtree(staged_dir)
        os.makedirs(staged_dir, exist_ok=True)

        print(f"\n[1/5] Staging primary binaries to {staged_dir}...")
        shutil.copy2(exe_src, staged_dir)
        if os.path.exists(pdfium_src):
            shutil.copy2(pdfium_src, staged_dir)

        windeployqt_candidates = [
            r"C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe",
            shutil.which("windeployqt") or ""
        ]
        windeployqt = next((p for p in windeployqt_candidates if p and os.path.exists(p)), None)

        if windeployqt:
            print(f"\n[2/5] Running windeployqt from {windeployqt}...")
            staged_exe = os.path.join(staged_dir, "PDFinity.exe")
            cmd = f'"{windeployqt}" "{staged_exe}" --compiler-runtime --no-translations'
            rc = subprocess.call(cmd, shell=True)
            if rc != 0:
                print(f"Warning: windeployqt returned non-zero code {rc}")
        else:
            print("[!] Notice: windeployqt not found on host. Copying runtime libraries and plugins from bin_dir...")
            for item in os.listdir(bin_dir):
                src_item = os.path.join(bin_dir, item)
                dst_item = os.path.join(staged_dir, item)
                if item.endswith(".dll") and not item.startswith("test_"):
                    shutil.copy2(src_item, dst_item)
                elif os.path.isdir(src_item) and item in ["platforms", "styles", "imageformats"]:
                    if os.path.exists(dst_item):
                        shutil.rmtree(dst_item)
                    shutil.copytree(src_item, dst_item)

    # Copy legal and documentation files
    for doc in ["LICENSE", "LICENSES.md", "README.md"]:
        src_path = os.path.join(root, doc)
        if os.path.exists(src_path):
            shutil.copy2(src_path, staged_dir)

    print(f"\n[3/5] Building Portable Zero-Footprint Archive...")
    if os.path.exists(portable_dir):
        shutil.rmtree(portable_dir)
    shutil.copytree(staged_dir, portable_dir)

    # Touch portable marker
    portable_marker = os.path.join(portable_dir, "portable.dat")
    with open(portable_marker, "w") as f:
        f.write("# PDFinity portable mode indicator\n")

    portable_zip = os.path.join(dist_dir, f"PDFinity-{version}-windows-x64-portable.zip")
    if os.path.exists(portable_zip):
        os.remove(portable_zip)

    with zipfile.ZipFile(portable_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for foldername, _, filenames in os.walk(portable_dir):
            for filename in filenames:
                filepath = os.path.join(foldername, filename)
                arcname = os.path.relpath(filepath, dist_dir)
                zipf.write(filepath, arcname)

    print(f"  --> Portable ZIP generated: {portable_zip} ({os.path.getsize(portable_zip):,} bytes)")

    # Build WiX MSI if requested or available
    wix_exe = shutil.which("wix") or os.path.expanduser(r"~\.dotnet\tools\wix.exe")
    wxs_file = os.path.join(root, "packaging", "windows", "pdfinity.wxs")
    msi_out = os.path.join(dist_dir, f"PDFinity-{version}-x64.msi")
    if args.build_msi and os.path.exists(wix_exe) and os.path.exists(wxs_file):
        print(f"\n[4/5] Building WiX MSI Package...")
        cmd = f'"{wix_exe}" build "{wxs_file}" -out "{msi_out}"'
        rc = subprocess.call(cmd, shell=True)
        if rc == 0 and os.path.exists(msi_out):
            print(f"  --> WiX MSI generated: {msi_out} ({os.path.getsize(msi_out):,} bytes)")
        else:
            print(f"  --> WiX build returned code {rc}")

    # Build Inno Setup
    installer_exe = os.path.join(dist_dir, f"PDFinity-{version}-Setup-x64.exe")
    if not args.skip_installer:
        print(f"\n[4b/5] Compiling Inno Setup Windows Installer...")
        iscc_candidates = [
            r"C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
            r"C:\Program Files\Inno Setup 6\ISCC.exe",
            shutil.which("iscc") or ""
        ]
        iscc = next((p for p in iscc_candidates if p and os.path.exists(p)), None)
        iss_file = os.path.join(root, "packaging", "windows", "installer.iss")

        if iscc and os.path.exists(iss_file):
            cmd = f'"{iscc}" "{iss_file}"'
            rc = subprocess.call(cmd, shell=True)
            if rc != 0:
                print(f"Warning: Inno Setup compiler returned code {rc}")
        else:
            print("[!] Notice: Inno Setup compiler ISCC.exe not found on this host. Skipping EXE installer.")

    print("\n[5/5] Packaging Verification and Integrity Hashes:")
    print("--------------------------------------------------------------------------------")
    if os.path.exists(installer_exe):
        size = os.path.getsize(installer_exe)
        sha = get_sha256(installer_exe)
        print(f"Installer:  {os.path.basename(installer_exe)}")
        print(f"  Size:     {size:,} bytes")
        print(f"  SHA-256:  {sha}")

    if os.path.exists(msi_out):
        size = os.path.getsize(msi_out)
        sha = get_sha256(msi_out)
        print(f"MSI:        {os.path.basename(msi_out)}")
        print(f"  Size:     {size:,} bytes")
        print(f"  SHA-256:  {sha}")

    if os.path.exists(portable_zip):
        size = os.path.getsize(portable_zip)
        sha = get_sha256(portable_zip)
        print(f"Portable:   {os.path.basename(portable_zip)}")
        print(f"  Size:     {size:,} bytes")
        print(f"  SHA-256:  {sha}")

    print("--------------------------------------------------------------------------------")
    print("Windows packaging step completed!")

if __name__ == "__main__":
    main()
