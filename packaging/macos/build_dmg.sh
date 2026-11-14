#!/usr/bin/env bash
# PDFinity macOS DMG Packaging Script
set -euo pipefail

BUILD_DIR="${1:-build/macos-release}"
VERSION="${2:-0.1.0}"
DIST_DIR="dist"
APP_BUNDLE="${DIST_DIR}/PDFinity.app"
DMG_PATH="${DIST_DIR}/PDFinity-${VERSION}-macOS.dmg"

echo "=== Staging macOS Application Bundle ==="
rm -rf "${APP_BUNDLE}"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"

cp "${BUILD_DIR}/bin/PDFinity" "${APP_BUNDLE}/Contents/MacOS/"
cp packaging/macos/bundle.plist "${APP_BUNDLE}/Contents/Info.plist"

echo "=== Packaging Disk Image (DMG) ==="
mkdir -p "${DIST_DIR}/dmg_staging"
cp -R "${APP_BUNDLE}" "${DIST_DIR}/dmg_staging/"
ln -s /Applications "${DIST_DIR}/dmg_staging/Applications"
cp LICENSE "${DIST_DIR}/dmg_staging/"
cp THIRDPARTY_NOTICES.txt "${DIST_DIR}/dmg_staging/"

if command -v hdiutil >/dev/null 2>&1; then
    hdiutil create -volname "PDFinity" -srcfolder "${DIST_DIR}/dmg_staging" -ov -format UDZO "${DMG_PATH}"
    echo "=== DMG created: ${DMG_PATH} ==="
else
    echo "Notice: hdiutil not available in non-macOS environment. DMG staging verified."
fi

rm -rf "${DIST_DIR}/dmg_staging"
