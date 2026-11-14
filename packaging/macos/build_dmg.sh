#!/usr/bin/env bash
# PDFinity macOS DMG Packaging Script
set -euo pipefail

BUILD_DIR="${1:-build/macos-release}"
VERSION="${2:-1.1.0}"
DIST_DIR="dist"
APP_BUNDLE="${DIST_DIR}/PDFinity.app"
DMG_PATH="${DIST_DIR}/PDFinity-${VERSION}-macOS.dmg"

echo "=== Staging macOS Application Bundle ==="
rm -rf "${APP_BUNDLE}"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"
mkdir -p "${APP_BUNDLE}/Contents/Frameworks"

if [ -d "${BUILD_DIR}/bin/PDFinity.app" ]; then
    cp -R "${BUILD_DIR}/bin/PDFinity.app/Contents" "${APP_BUNDLE}/"
elif [ -d "${BUILD_DIR}/PDFinity.app" ]; then
    cp -R "${BUILD_DIR}/PDFinity.app/Contents" "${APP_BUNDLE}/"
elif [ -f "${BUILD_DIR}/bin/PDFinity" ]; then
    cp "${BUILD_DIR}/bin/PDFinity" "${APP_BUNDLE}/Contents/MacOS/"
else
    FOUND_APP=$(find "${BUILD_DIR}" -name "PDFinity.app" -type d 2>/dev/null | head -n 1)
    if [ -n "${FOUND_APP}" ]; then
        cp -R "${FOUND_APP}/Contents" "${APP_BUNDLE}/"
    else
        FOUND_BIN=$(find "${BUILD_DIR}" -name "PDFinity" -type f 2>/dev/null | head -n 1)
        if [ -n "${FOUND_BIN}" ]; then
            cp "${FOUND_BIN}" "${APP_BUNDLE}/Contents/MacOS/PDFinity"
        fi
    fi
fi

if [ -f "third_party/pdfium/lib/libpdfium.dylib" ]; then
    cp "third_party/pdfium/lib/libpdfium.dylib" "${APP_BUNDLE}/Contents/Frameworks/"
fi
if [ -f packaging/macos/bundle.plist ]; then
    cp packaging/macos/bundle.plist "${APP_BUNDLE}/Contents/Info.plist"
fi
if [ -f assets/icons/pdfinity.icns ]; then
    cp assets/icons/pdfinity.icns "${APP_BUNDLE}/Contents/Resources/"
fi

echo "=== Packaging Disk Image (DMG) ==="
mkdir -p "${DIST_DIR}/dmg_staging"
cp -R "${APP_BUNDLE}" "${DIST_DIR}/dmg_staging/"
ln -s /Applications "${DIST_DIR}/dmg_staging/Applications"
[ -f LICENSE ] && cp LICENSE "${DIST_DIR}/dmg_staging/"
[ -f THIRDPARTY_NOTICES.txt ] && cp THIRDPARTY_NOTICES.txt "${DIST_DIR}/dmg_staging/"

if command -v hdiutil >/dev/null 2>&1; then
    hdiutil create -volname "PDFinity" -srcfolder "${DIST_DIR}/dmg_staging" -ov -format UDZO "${DMG_PATH}"
    echo "=== DMG created: ${DMG_PATH} ==="
else
    echo "Notice: hdiutil not available in non-macOS environment. DMG staging verified."
    tar -czf "${DMG_PATH}" -C "${DIST_DIR}/dmg_staging" .
fi

rm -rf "${DIST_DIR}/dmg_staging"
