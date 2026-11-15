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
import hashlib
import json
import pathlib
import tempfile

PDFIUM_RELEASE_TAG = "chromium/8152"
PDFIUM_VERSION = "159.0.8152.0"
BASE_URL = f"https://github.com/bblanchon/pdfium-binaries/releases/download/{PDFIUM_RELEASE_TAG}"

LOCK = json.loads((pathlib.Path(__file__).resolve().parents[1] / 'third_party/pdfium/dependency-lock.json').read_text())
PLATFORM_ARCHIVES = LOCK['platforms']


def verified_cache(target, config):
    for name, expected in config['files'].items():
        path = pathlib.Path(target) / name
        if path.is_symlink() or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            return False
    return True


def verified_members(data, config):
    if hashlib.sha256(data).hexdigest() != config['sha256']:
        raise RuntimeError('PDFium archive SHA-256 mismatch; refusing extraction')
    contents = {}
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as tar:
        names = set()
        for member in tar:
            name = member.name.removeprefix('./')
            parts = pathlib.PurePosixPath(name).parts
            if not parts or name.startswith('/') or any(p in ('.', '..') for p in parts) or '\\' in name or ':' in name:
                raise RuntimeError('Unsafe PDFium archive path: ' + name)
            if name in names or not (member.isfile() or member.isdir()):
                raise RuntimeError('Unsupported/duplicate PDFium archive entry: ' + name)
            names.add(name)
            if name in config['files']:
                if not member.isfile():
                    raise RuntimeError('Pinned PDFium file is not regular: ' + name)
                content = tar.extractfile(member).read()
                if hashlib.sha256(content).hexdigest() != config['files'][name]:
                    raise RuntimeError('PDFium member SHA-256 mismatch: ' + name)
                contents[name] = content
    if set(contents) != set(config['files']):
        raise RuntimeError('Pinned PDFium archive members are missing')
    return contents

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

def install_pdfium(target_dir, plat, force=False, verify_only=False, archive_path=None):
    cfg = PLATFORM_ARCHIVES[plat]
    if not force and verified_cache(target_dir, cfg):
        print(f"[+] Verified pinned PDFium cache for '{plat}' at {target_dir}")
        return True
    if verify_only:
        raise RuntimeError('Missing or corrupt pinned PDFium files. Run scripts/download_pdfium.py --platform ' + plat)
    if not archive_path and cfg.get('acquisition') == 'source-build':
        raise RuntimeError('This variant requires the locked source build archive; run scripts/build_pdfium_rpm.py first')
    data = pathlib.Path(archive_path).read_bytes() if archive_path else download_with_retry(cfg.get('url', BASE_URL + '/' + cfg['archive']))
    # Validate the complete archive and all pinned members before any target write.
    contents = verified_members(data, cfg)
    target = pathlib.Path(target_dir).resolve()
    for name in contents:
        destination = target / name
        if destination.is_symlink() or not destination.resolve().is_relative_to(target):
            raise RuntimeError('Unsafe PDFium destination: ' + name)
    target.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='pdfium-verified-') as work:
        stage = pathlib.Path(work)
        for name, content in contents.items():
            path = stage / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        for name in contents:
            path = target / name
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(stage / name, path)
            if name.startswith(('bin/', 'lib/')):
                path.chmod(0o755)
    if not verified_cache(target, cfg):
        raise RuntimeError('PDFium installed-file verification failed')
    print(f"[+] Verified PDFium {PDFIUM_VERSION}: {plat}, archive SHA-256 {cfg['sha256']}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Download & extract PDFium precompiled binaries")
    parser.add_argument("--platform", choices=[*PLATFORM_ARCHIVES, "all"], default=None,
                        help="Target platform (default: auto-detect)")
    parser.add_argument("--target-dir", default=None,
                        help="Target destination directory (default: third_party/pdfium)")
    parser.add_argument("--force", action="store_true", help="Force re-download even if files exist")
    parser.add_argument('--verify-only', action='store_true', help='Verify locked installed files without network or writes')
    parser.add_argument('--archive', help='Use a local archive matching the pinned digest')
    args = parser.parse_args()
    if args.archive and args.platform == 'all':
        parser.error('--archive requires one platform')

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    target_dir = args.target_dir or os.path.join(root, "third_party", "pdfium")

    plat = args.platform or detect_platform()
    print("==================================================")
    print(" PDFinity PDFium Binaries Provisioner")
    print(f" Target Directory: {target_dir}")
    print(f" Selected Platform: {plat}")
    print(f" PDFium Release: {PDFIUM_RELEASE_TAG}")
    print("==================================================")

    if plat == "all":
        for p in ["win", "linux", "mac"]:
            install_pdfium(target_dir, p, force=args.force, verify_only=args.verify_only)
    else:
        install_pdfium(target_dir, plat, force=args.force, verify_only=args.verify_only, archive_path=args.archive)

    print("\n[+] PDFium binary provisioning completed successfully.")

if __name__ == "__main__":
    main()
