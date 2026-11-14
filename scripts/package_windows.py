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
from pathlib import Path

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
    raise ValueError("Cannot determine package version from CMakeLists.txt")

def main():
    parser = argparse.ArgumentParser(description="PDFinity Windows Packaging Utility")
    parser.add_argument("--version", default=None, help="Target release version string")
    parser.add_argument("--skip-installer", action="store_true", help="Skip Inno Setup compilation")
    parser.add_argument("--build-msi", action="store_true", help="Compile WiX MSI package")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    os.chdir(root)

    version = args.version or detect_version(root)
    if version != detect_version(root):
        raise ValueError("Requested package version differs from checked-out source")

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

    for required in (exe_src, pdfium_src, os.path.join(bin_dir, "pdfinity-cli.exe")):
        if not os.path.isfile(required) or os.path.getsize(required) == 0:
            raise RuntimeError(f"Required fresh build output missing: {required}; stale staging is forbidden")
    if os.path.exists(exe_src):
        if os.path.exists(staged_dir):
            shutil.rmtree(staged_dir)
        os.makedirs(staged_dir, exist_ok=True)

        print(f"\n[1/5] Staging primary binaries to {staged_dir}...")
        shutil.copy2(exe_src, staged_dir)
        shutil.copy2(pdfium_src, staged_dir)
        shutil.copy2(os.path.join(bin_dir, "pdfinity-cli.exe"), staged_dir)

        windeployqt_candidates = [
            shutil.which("windeployqt") or "",
            r"C:\Qt\6.8.2\msvc2022_64\bin\windeployqt.exe"
        ]
        windeployqt = next((p for p in windeployqt_candidates if p and os.path.exists(p)), None)

        if windeployqt:
            print(f"\n[2/5] Running windeployqt from {windeployqt}...")
            staged_exe = os.path.join(staged_dir, "PDFinity.exe")
            subprocess.run([windeployqt, staged_exe, "--compiler-runtime", "--no-translations"], check=True)
            subprocess.run([windeployqt, os.path.join(staged_dir, "pdfinity-cli.exe"),
                            "--compiler-runtime", "--no-translations"], check=True)
        else:
            raise RuntimeError("windeployqt is required; incomplete runtime fallback is forbidden")
        crt = ("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll")
        if not all(os.path.isfile(os.path.join(staged_dir, name)) for name in crt):
            redist = os.environ.get("VCToolsRedistDir")
            if not redist:
                raise RuntimeError("App-local MSVC runtime missing; initialize the MSVC developer environment")
            directories = list((Path(redist) / "x64").glob("Microsoft.VC*.CRT"))
            if len(directories) != 1:
                raise RuntimeError("Cannot identify a unique x64 MSVC redistributable directory")
            for runtime in directories[0].glob("*.dll"):
                shutil.copy2(runtime, staged_dir)
        for runtime in ("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Network.dll",
                        "Qt6PrintSupport.dll", *crt, os.path.join("platforms", "qwindows.dll")):
            if not os.path.isfile(os.path.join(staged_dir, runtime)):
                raise RuntimeError(f"Deployment omitted required runtime: {runtime}")

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
    if args.build_msi:
        if not os.path.isfile(wix_exe) or not os.path.isfile(wxs_file):
            raise RuntimeError("Requested WiX MSI build requires wix and the source recipe")
        print(f"\n[4/5] Building WiX MSI Package...")
        subprocess.run([wix_exe, "build", wxs_file, "-out", msi_out], check=True)
        if not os.path.isfile(msi_out) or os.path.getsize(msi_out) == 0:
            raise RuntimeError("WiX returned without producing the required MSI")

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
            subprocess.run([iscc, iss_file], check=True)
            if not os.path.isfile(installer_exe) or os.path.getsize(installer_exe) == 0:
                raise RuntimeError("Inno Setup returned without the required installer")
        else:
            raise RuntimeError("Inno Setup is required unless --skip-installer was explicitly requested")

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
