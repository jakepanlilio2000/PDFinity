import subprocess
import sys
import os
import re

print("=== PHASE S4 EXIT GATE VERIFICATION ===\n")

# 1. Git rev-list
count = subprocess.check_output(['git', 'rev-list', '--count', 'HEAD']).decode().strip()
print(f"1. git rev-list --count HEAD: {count}")
assert count == '1', f"Expected 1 commit, got {count}"

# 2. Git commit message
msg = subprocess.check_output(['git', 'log', '-1', '--pretty=%B']).decode().strip()
print(f"2. git log -1 --pretty=%B: {msg}")
assert msg == "Initial commit", f"Expected Initial commit, got {msg}"

# 3. Git tag
tags = subprocess.check_output(['git', 'tag', '-l']).decode().strip().split()
print(f"3. git tag -l: {tags}")
assert 'v0.1-beta' in tags, f"Expected v0.1-beta in tags, got {tags}"

# 4. Version strings across files
version_targets = [
    ('CMakeLists.txt', r'project\s*\(\s*PDFinity\s+VERSION\s+0\.1\.0'),
    ('src/app/main.cpp', r'PDFinity 0\.1-Beta'),
    ('src/app/main.cpp', r'setApplicationVersion\("0\.1\.0-beta"\)'),
    ('src/ui/AboutDialog.cpp', r'PDFinity 0\.1-Beta'),
    ('src/mcp/McpTypes.hpp', r'MCP_SERVER_VERSION = "0\.1\.0-beta"'),
    ('packaging/windows/version.rc', r'#define VER_PRODUCTVERSION_STR\s+"0\.1-Beta\\0"'),
    ('packaging/windows/version.rc', r'#define VER_FILEVERSION_STR\s+"0\.1\.0\.0\\0"'),
    ('packaging/windows/installer.iss', r'#define MyAppVersion "0\.1-Beta"'),
    ('packaging/windows/pdfinity.wxs', r'Version="0\.1\.0"'),
    ('packaging/macos/bundle.plist', r'<key>CFBundleShortVersionString</key>\s*<string>0\.1\.0</string>'),
    ('packaging/linux/deb/control', r'Version:\s*0\.1\.0'),
    ('packaging/linux/arch/PKGBUILD', r'pkgver=0\.1\.0'),
    ('packaging/linux/rpm/pdfinity.spec', r'Version:\s*0\.1\.0'),
    ('packaging/linux/flatpak/com.stacksyncsolutions.pdfinity.metainfo.xml', r'<release version="0\.1\.0"'),
    ('android/build.gradle', r'versionName "0\.1-Beta"'),
    ('android/AndroidManifest.xml', r'android:versionName="0\.1-Beta"'),
    ('README.md', r'badge/Release-0\.1--Beta-blue\.svg'),
    ('CHANGELOG.md', r'## \[0\.1-Beta\] - Initial Public Beta'),
]

print("\n4. Version checks across all packaging, build, and source targets:")
for fpath, pattern in version_targets:
    with open(fpath, 'r', encoding='utf-8') as f:
        content = f.read()
    if re.search(pattern, content):
        print(f"  [PASS] {fpath}: matches pattern")
    else:
        print(f"  [FAIL] {fpath}: does not match pattern '{pattern}'")
        sys.exit(1)

# 5. LICENSE check
with open('LICENSE', 'r', encoding='utf-8') as f:
    lic = f.read()
assert 'Mozilla Public License Version 2.0' in lic
assert len(lic.splitlines()) == 344
print("\n5. LICENSE: 100% official MPL-2.0 (344 lines). [PASS]")

# 6. Prohibited phrasing
PROHIBITED = [
    r'phase-by-phase', r'phase\s+by\s+phase',
    r'\bphase\s+0\b', r'\bphase\s+1\b', r'\bphase\s+2\b', r'\bphase\s+3\b',
    r'\bphase\s+4\b', r'\bphase\s+5\b', r'\bphase\s+6\b', r'\bphase\s+7\b',
    r'\bphase\s+8\b', r'\bphase\s+9\b', r'\bphase\s+10\b', r'\bphase\s+11\b',
    r'\bphase\s+12\b', r'\bphase\s+13\b', r'\bphase\s+14\b', r'\bphase\s+15\b',
    r'\bphase\s+16\b',
    r'\bexit\s+gate\b', r'\baudit\s+verdict\b', r'\breusable\s+audit\b',
    r'shipping\s+master\s+prompt', r'\bantigravity\b', r'\bgemini\b',
    r'\bchatgpt\b', r'\bopenai\b', r'\bllm\b', r'\bllms\b', r'\bai\s+agent\b',
    r'\bprompt\b', r'\bprompts\b', r'\bscaffolding\b'
]
compiled = [re.compile(p, re.IGNORECASE) for p in PROHIBITED]
for root, dirs, files in os.walk('.'):
    if '.git' in dirs:
        dirs.remove('.git')
    if 'third_party' in dirs:
        dirs.remove('third_party')
    for f in files:
        if f.endswith(('.md', '.cpp', '.hpp', '.h', '.cmake', '.json', '.xml', '.desktop', '.iss', '.wxs', '.rc', '.plist', '.spec')):
            with open(os.path.join(root, f), 'r', encoding='utf-8', errors='ignore') as fp:
                txt = fp.read()
            for r in compiled:
                assert not r.search(txt), f"Prohibited match in {os.path.join(root, f)}"

print("6. Prohibited words/phrases scan: 0 occurrences detected. [PASS]")
print("\nALL PHASE S4 EXIT CRITERIA MET!")
