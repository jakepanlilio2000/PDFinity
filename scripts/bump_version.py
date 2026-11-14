#!/usr/bin/env python3
"""
PDFinity - Release Version Management & Synchronization Utility
Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
License: Mozilla Public License Version 2.0 (MPL-2.0)
"""

import sys
import os
import re
import argparse

def parse_version_info(ver_str):
    ver_clean = ver_str.strip().lstrip("v")
    m = re.match(r"^(\d+\.\d+\.\d+)(?:-([a-zA-Z0-9\.]+))?$", ver_clean)
    if not m:
        m2 = re.match(r"^(\d+\.\d+)(?:-([a-zA-Z0-9\.]+))?$", ver_clean)
        if m2:
            base = m2.group(1) + ".0"
            suffix = m2.group(2)
        else:
            raise ValueError(f"Invalid version string format: '{ver_str}'. Expected format like 0.1.0-beta or 1.0.0")
    else:
        base = m.group(1)
        suffix = m.group(2)

    semver_num = base
    full_ver = f"{base}-{suffix}" if suffix else base

    # Construct tag and display names
    if suffix:
        if base == "0.1.0":
            tag_name = "v0.1-beta"
            display_name = "0.1-Beta"
        else:
            tag_name = f"v{full_ver}"
            display_name = f"{base}-{suffix.capitalize()}"
    else:
        if base == "1.0.0":
            tag_name = "v1.0.0"
            display_name = "1.0 Stable"
        else:
            tag_name = f"v{base}"
            display_name = f"{base} Stable"

    major, minor, patch = base.split(".")
    return {
        "semver_num": semver_num,
        "full_version": full_ver,
        "raw_version": ver_clean,
        "tag_name": tag_name,
        "display_name": display_name,
        "major": major,
        "minor": minor,
        "patch": patch
    }

def update_file(path, pattern, replacement, count=1):
    if not os.path.exists(path):
        print(f"[-] File not found: {path}")
        return False
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    new_content, n = re.subn(pattern, replacement, content, count=count)
    if n == 0:
        print(f"[!] Warning: Pattern not found in {path}")
        return False

    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(new_content)
    print(f"[+] Updated {path}")
    return True

def bump_all(root_dir, ver_info):
    semver = ver_info["semver_num"]
    full_ver = ver_info["full_version"]
    display = ver_info["display_name"]
    major = ver_info["major"]
    minor = ver_info["minor"]
    patch = ver_info["patch"]
    tag = ver_info["tag_name"]

    print(f"\n[+] Synchronizing repository version to:")
    print(f"    SemVer Number:   {semver}")
    print(f"    Full Version:    {full_ver}")
    print(f"    Git Tag:         {tag}")
    print(f"    Display Name:    {display}\n")

    # 1. CMakeLists.txt
    update_file(
        os.path.join(root_dir, "CMakeLists.txt"),
        r"(project\s*\(\s*PDFinity\s+VERSION\s+)[0-9\.]+",
        rf"\g<1>{semver}"
    )

    # 2. src/app/main.cpp
    update_file(
        os.path.join(root_dir, "src", "app", "main.cpp"),
        r'(void PrintVersion\(\)\s*\{\s*std::cout << "PDFinity )[^"\n]+(?:\\n)?(")',
        rf'\g<1>{display}\\n\g<2>'
    )
    update_file(
        os.path.join(root_dir, "src", "app", "main.cpp"),
        r'(app\.setApplicationVersion\(")[^"]+("\);)',
        rf'\g<1>{full_ver}\g<2>'
    )

    # 3. src/ui/AboutDialog.cpp
    update_file(
        os.path.join(root_dir, "src", "ui", "AboutDialog.cpp"),
        r'(auto\* titleLabel = new QLabel\(tr\("<h2>PDFinity)[^<]*</h2>"\), this\);',
        rf'\g<1> {display}</h2>"), this);'
    )

    # 4. src/mcp/McpTypes.hpp
    update_file(
        os.path.join(root_dir, "src", "mcp", "McpTypes.hpp"),
        r'(inline constexpr const char\* MCP_SERVER_VERSION = ")[^"]+(")',
        rf'\g<1>{full_ver}\g<2>'
    )

    # 5. packaging/windows/version.rc
    rc_path = os.path.join(root_dir, "packaging", "windows", "version.rc")
    if os.path.exists(rc_path):
        with open(rc_path, "r", encoding="utf-8") as f:
            rc_content = f.read()
        rc_content = re.sub(r"#define VER_FILEVERSION\s+[0-9,]+", f"#define VER_FILEVERSION             {major},{minor},{patch},0", rc_content)
        rc_content = re.sub(r'#define VER_FILEVERSION_STR\s+"[^"]+"', f'#define VER_FILEVERSION_STR         "{semver}.0\\\\0"', rc_content)
        rc_content = re.sub(r"#define VER_PRODUCTVERSION\s+[0-9,]+", f"#define VER_PRODUCTVERSION          {major},{minor},{patch},0", rc_content)
        rc_content = re.sub(r'#define VER_PRODUCTVERSION_STR\s+"[^"]+"', f'#define VER_PRODUCTVERSION_STR      "{display}\\\\0"', rc_content)
        with open(rc_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(rc_content)
        print(f"[+] Updated {rc_path}")

    # 6. packaging/windows/installer.iss
    update_file(
        os.path.join(root_dir, "packaging", "windows", "installer.iss"),
        r'(#define MyAppVersion ")[^"]+(")',
        rf'\g<1>{display}\g<2>'
    )

    # 7. packaging/windows/pdfinity.wxs
    update_file(
        os.path.join(root_dir, "packaging", "windows", "pdfinity.wxs"),
        r'(Version=")[0-9\.]+(\")',
        rf'\g<1>{semver}\g<2>'
    )

    # 8. packaging/macos/bundle.plist
    update_file(
        os.path.join(root_dir, "packaging", "macos", "bundle.plist"),
        r'(<key>CFBundleShortVersionString</key>\s*<string>)[^<]+(</string>)',
        rf'\g<1>{semver}\g<2>'
    )

    # 9. packaging/linux/deb/control
    update_file(
        os.path.join(root_dir, "packaging", "linux", "deb", "control"),
        r'(Version:\s*)[0-9\.]+',
        rf'\g<1>{semver}'
    )

    # 10. packaging/linux/arch/PKGBUILD
    update_file(
        os.path.join(root_dir, "packaging", "linux", "arch", "PKGBUILD"),
        r'(pkgver=)[0-9\.]+',
        rf'\g<1>{semver}'
    )

    # 11. packaging/linux/rpm/pdfinity.spec
    update_file(
        os.path.join(root_dir, "packaging", "linux", "rpm", "pdfinity.spec"),
        r'(Version:\s*)[0-9\.]+',
        rf'\g<1>{semver}'
    )

    # 12. packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.metainfo.xml
    update_file(
        os.path.join(root_dir, "packaging", "linux", "flatpak", "com.stacksyncsolutions.pdfinity.metainfo.xml"),
        r'(<release version=")[^"]+(" date="[^"]+">)',
        rf'\g<1>{semver}\g<2>'
    )

    # 13. android/build.gradle and packaging/android/build.gradle
    for bg_path in [os.path.join(root_dir, "android", "build.gradle"),
                    os.path.join(root_dir, "packaging", "android", "build.gradle")]:
        update_file(
            bg_path,
            r'(versionName\s+")[^"]+(")',
            rf'\g<1>{display}\g<2>'
        )

    # 14. android/AndroidManifest.xml and packaging/android/AndroidManifest.xml
    for mf_path in [os.path.join(root_dir, "android", "AndroidManifest.xml"),
                    os.path.join(root_dir, "packaging", "android", "AndroidManifest.xml")]:
        update_file(
            mf_path,
            r'(android:versionName=")[^"]+(")',
            rf'\g<1>{display}\g<2>'
        )

    # 15. README.md badges & links
    readme_path = os.path.join(root_dir, "README.md")
    if os.path.exists(readme_path):
        with open(readme_path, "r", encoding="utf-8") as f:
            readme_content = f.read()
        badge_release = display.replace(" ", "%20").replace("-", "--")
        badge_version = semver
        readme_content = re.sub(
            r'\[!\[Release\]\(https://img\.shields\.io/badge/Release-[^)]+\)\]\([^)]+\)',
            f'[![Release](https://img.shields.io/badge/Release-{badge_release}-blue.svg)](https://github.com/jakepanlilio2000/PDFinity/releases/tag/{tag})',
            readme_content
        )
        readme_content = re.sub(
            r'\[!\[Version\]\(https://img\.shields\.io/badge/Version-[^)]+\)\]\([^)]+\)',
            f'[![Version](https://img.shields.io/badge/Version-{badge_version}-emerald.svg)](https://github.com/jakepanlilio2000/PDFinity/releases/tag/{tag})',
            readme_content
        )
        with open(readme_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(readme_content)
        print(f"[+] Updated {readme_path}")

def main():
    parser = argparse.ArgumentParser(description="PDFinity Version Bumper")
    parser.add_argument("version", help="Version to bump to (e.g. 0.1.0-beta, 0.1.1-beta, 0.2.0-beta, 1.0.0)")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    ver_info = parse_version_info(args.version)
    bump_all(root, ver_info)

if __name__ == "__main__":
    main()
