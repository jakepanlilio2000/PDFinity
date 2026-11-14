#!/usr/bin/env python3
"""Read/validate the existing version fields; CMake is the numeric source of truth."""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]


def version(root=ROOT):
    return re.search(r"project\s*\(\s*PDFinity\s+VERSION\s+(\d+\.\d+\.\d+)",
                     (root / "CMakeLists.txt").read_text(encoding="utf-8")).group(1)


def fields(v):
    if not re.fullmatch(r"(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)", v):
        raise ValueError("The shipping lane requires a stable MAJOR.MINOR.PATCH version")
    major, minor, patch = map(int, v.split("."))
    if minor >= 100 or patch >= 100 or major > 65535:
        raise ValueError("Version cannot map uniquely to Android code / Windows resource fields")
    code = str(major * 10000 + minor * 100 + patch)
    if int(code) > 2100000000:
        raise ValueError("Android versionCode overflow")
    display = v + " Stable"
    specs = [
        ("CMakeLists.txt", r"(?s)(project\s*\(\s*PDFinity\s+VERSION\s+)(\d+\.\d+\.\d+)", v),
        ("src/app/main.cpp", r'(PDFinity )([0-9.]+ Stable)(\\n")', display),
        ("src/app/main.cpp", r'(setApplicationVersion\(")([^"]+)', v),
        ("src/app/cli_main.cpp", r'(PDFinity CLI )([0-9.]+ Stable)(\\n")', display),
        ("src/ui/AboutDialog.cpp", r'(<h2>PDFinity )([^<]+)', display),
        ("src/mcp/McpTypes.hpp", r'(MCP_SERVER_VERSION = ")([^"]+)', v),
        ("packaging/windows/version.rc", r'(#define VER_FILEVERSION\s+)([0-9,]+)', v.replace('.', ',') + ',0'),
        ("packaging/windows/version.rc", r'(#define VER_PRODUCTVERSION\s+)([0-9,]+)', v.replace('.', ',') + ',0'),
        ("packaging/windows/version.rc", r'(#define VER_FILEVERSION_STR\s+")([^"\\]+)', v + '.0'),
        ("packaging/windows/version.rc", r'(#define VER_PRODUCTVERSION_STR\s+")([^"\\]+)', display),
        ("packaging/windows/installer.iss", r'(#define MyAppVersion ")([^"]+)', v),
        ("packaging/windows/pdfinity.wxs", r'(Version=")([0-9.]+)', v),
        ("packaging/macos/bundle.plist", r'(?s)(<key>CFBundleShortVersionString</key>\s*<string>)([^<]+)', v),
        ("packaging/macos/bundle.plist", r'(?s)(<key>CFBundleVersion</key>\s*<string>)([^<]+)', v),
        ("packaging/linux/deb/control", r'(?m)(^Version:\s*)([0-9.]+)', v),
        ("packaging/linux/rpm/pdfinity.spec", r'(?m)(^Version:\s*)([0-9.]+)', v),
        ("packaging/linux/rpm/pdfinity.spec", r'(%global tag_version )([0-9.]+)', v),
        ("packaging/linux/arch/PKGBUILD", r'(pkgver=)([0-9.]+)', v),
        ("packaging/linux/arch/.SRCINFO", r'(pkgver = )([0-9.]+)', v),
        ("packaging/linux/arch/PKGBUILD", r'(archive/v)([^/" ]+)(\.tar\.gz)', v),
        ("packaging/linux/arch/.SRCINFO", r'(archive/v)([^/" ]+)(\.tar\.gz)', v),
        ("packaging/linux/arch/PKGBUILD", r'(srcdir\}/PDFinity-)([^" ]+)', v),
        ("README.md", r'(badge/Release-)([^)]+?)(-blue.svg)', v + '%20Stable'),
        ("README.md", r'(badge/Version-)([^)]+?)(-emerald.svg)', v),
    ]
    for path in ("README.md",):
        # Both badges must point to the same tag; historical prose links are excluded.
        specs.append((path, r'(releases/tag/v)([0-9.]+)(\))', v))
    for path in ("android/build.gradle", "packaging/android/build.gradle"):
        specs += [(path, r'(versionName\s+")([^"]+)', display),
                  (path, r'(versionCode\s+)(\d+)', code)]
    for path in ("android/AndroidManifest.xml", "packaging/android/AndroidManifest.xml"):
        specs += [(path, r'(android:versionName=")([^"]+)', display),
                  (path, r'(android:versionCode=")(\d+)', code)]
    for path in ("packaging/linux/com.stacksyncsolutions.pdfinity.metainfo.xml",
                 "packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.metainfo.xml"):
        specs.append((path, r'(<release version=")([^"]+)', v))
    return specs


def plan(root, v):
    planned = {}
    for name, pattern, expected in fields(v):
        text = planned.get(name, (root / name).read_text(encoding="utf-8"))
        matches = list(re.finditer(pattern, text))
        count = 2 if name == "README.md" and "releases/tag" in pattern else 1
        if len(matches) != count:
            raise ValueError(f"{name}: expected {count} version field(s), found {len(matches)}: {pattern}")
        # Replace only the value span; preserves literal C++/RC escapes and formatting.
        for match in reversed(matches):
            text = text[:match.start(2)] + expected + text[match.end(2):]
        planned[name] = text
    return planned


def validate(root=ROOT, expected=None):
    v = version(root)
    errors = []
    if expected is not None and expected != v:
        errors.append(f"CMake {v} != requested version {expected}")
    for name, planned in plan(root, v).items():
        if planned != (root / name).read_text(encoding="utf-8"):
            errors.append(f"{name}: version metadata disagrees with {v}")
    pkgrel = re.search(r'(?m)^pkgrel=(\d+)$', (root / 'packaging/linux/arch/PKGBUILD').read_text()).group(1)
    srcinfo = (root / 'packaging/linux/arch/.SRCINFO').read_text()
    if not re.search(r'(?m)^\s*pkgrel = ' + pkgrel + r'$', srcinfo):
        errors.append("Arch pkgrel differs from .SRCINFO")
    latest = re.search(r'(?m)^## \[([0-9.]+)\]', (root / 'CHANGELOG.md').read_text(encoding='utf-8'))
    if not latest or latest.group(1) != v:
        errors.append("Latest changelog entry does not match CMake")
    if errors:
        raise ValueError("\n".join(errors))
    return v


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=pathlib.Path, default=ROOT)
    parser.add_argument('--expected')
    parser.add_argument('--print', action='store_true', dest='print_only')
    args = parser.parse_args()
    try:
        print(version(args.root) if args.print_only else validate(args.root, args.expected))
    except (ValueError, AttributeError, OSError) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
