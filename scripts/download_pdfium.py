#!/usr/bin/env python3
"""
PDFinity - PDFium Precompiled Binaries Downloader & Validator
Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
License: Mozilla Public License Version 2.0 (MPL-2.0)
"""

import os
import sys
import argparse
import urllib.request
import tarfile
import io
import time
import shutil

PDFIUM_RELEASE_TAG = "chromium/8152"
PDFIUM_VERSION = "159.0.8152.0"
BASE_URL = f"https://github.com/bblanchon/pdfium-binaries/releases/download/{PDFIUM_RELEASE_TAG}"

PLATFORM_ARCHIVES = {
    "win": {
        "archive": "pdfium-win-x64.tgz",
        "expected": [os.path.join("bin", "pdfium.dll"), os.path.join("lib", "pdfium.dll.lib")]
    },
    "linux": {
        "archive": "pdfium-linux-x64.tgz",
        "expected": [os.path.join("lib", "libpdfium.so")]
    },
    "mac": {
        "archive": "pdfium-mac-univ.tgz",
        "expected": [os.path.join("lib", "libpdfium.dylib")]
    }
}

def detect_platform():
    if sys.platform.startswith("win"):
        return "win"
    elif sys.platform.startswith("darwin"):
        return "mac"
    elif sys.platform.startswith("linux"):
        return "linux"
    return "linux"

def download_with_retry(url, retries=3, delay=3):
    headers = {"User-Agent": "PDFinity-Build-Bot/1.1.0"}
    req = urllib.request.Request(url, headers=headers)
    for attempt in range(1, retries + 1):
        try:
            print(f"[+] Downloading {url} (Attempt {attempt}/{retries})...")
            with urllib.request.urlopen(req, timeout=60) as response:
                if response.status == 200:
                    data = response.read()
                    print(f"[+] Download complete: {len(data)} bytes received.")
                    return data
                else:
                    print(f"[-] HTTP error {response.status}: {response.reason}")
        except Exception as e:
            print(f"[-] Download attempt {attempt} failed: {e}")
            if attempt < retries:
                time.sleep(delay * attempt)
    raise RuntimeError(f"Failed to download from {url} after {retries} attempts.")

def install_pdfium(target_dir, plat, force=False):
    cfg = PLATFORM_ARCHIVES[plat]
    expected_paths = [os.path.join(target_dir, p) for p in cfg["expected"]]
    
    if not force and all(os.path.exists(p) for p in expected_paths):
        print(f"[+] PDFium for '{plat}' is already installed at {target_dir}. Skipping download.")
        return True

    archive_name = cfg["archive"]
    url = f"{BASE_URL}/{archive_name}"
    
    data = download_with_retry(url)
    print(f"[+] Extracting {archive_name} to {target_dir}...")
    
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tar:
        for member in tar.getmembers():
            # Extract bin, lib, and include if not present
            if member.name.startswith("bin") or member.name.startswith("lib"):
                tar.extract(member, path=target_dir)
            elif member.name.startswith("include") and not os.path.exists(os.path.join(target_dir, "include")):
                tar.extract(member, path=target_dir)
            elif member.name.startswith("licenses") and not os.path.exists(os.path.join(target_dir, "licenses")):
                tar.extract(member, path=target_dir)

    # Ensure executable permissions for shared libraries on UNIX
    for p in expected_paths:
        if os.path.exists(p):
            os.chmod(p, 0o755)
            print(f"[+] Verified: {p}")
        else:
            raise FileNotFoundError(f"Expected file missing after extraction: {p}")

    print(f"[+] Successfully installed PDFium {PDFIUM_VERSION} for {plat} into {target_dir}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Download & extract PDFium precompiled binaries")
    parser.add_argument("--platform", choices=["win", "linux", "mac", "all"], default=None,
                        help="Target platform (default: auto-detect)")
    parser.add_argument("--target-dir", default=None,
                        help="Target destination directory (default: third_party/pdfium)")
    parser.add_argument("--force", action="store_true", help="Force re-download even if files exist")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    target_dir = args.target_dir or os.path.join(root, "third_party", "pdfium")
    os.makedirs(target_dir, exist_ok=True)

    plat = args.platform or detect_platform()
    print("==================================================")
    print(" PDFinity PDFium Binaries Provisioner")
    print(f" Target Directory: {target_dir}")
    print(f" Selected Platform: {plat}")
    print(f" PDFium Release: {PDFIUM_RELEASE_TAG}")
    print("==================================================")

    if plat == "all":
        for p in ["win", "linux", "mac"]:
            install_pdfium(target_dir, p, force=args.force)
    else:
        install_pdfium(target_dir, plat, force=args.force)

    print("\n[+] PDFium binary provisioning completed successfully.")

if __name__ == "__main__":
    main()
