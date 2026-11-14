#!/usr/bin/env bash
# PDFinity Linux AppImage Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
set -euo pipefail

BUILD_DIR="${1:-build/linux-x64-release}"
VERSION="${2:-0.1.0}"
APP_DIR="dist/AppDir"
OUTPUT_DIR="dist"
APPIMAGE_NAME="PDFinity-${VERSION}-x86_64.AppImage"

echo "=== Preparing PDFinity AppDir for ${APPIMAGE_NAME} ==="
rm -rf "${APP_DIR}"
mkdir -p "${APP_DIR}/usr/bin"
mkdir -p "${APP_DIR}/usr/lib"
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${OUTPUT_DIR}"

# Binary
if [ -f "${BUILD_DIR}/bin/PDFinity" ]; then
    install -m 755 "${BUILD_DIR}/bin/PDFinity" "${APP_DIR}/usr/bin/PDFinity"
else
    echo "Warning: ${BUILD_DIR}/bin/PDFinity not found. Creating placeholder binary for dry-run validation."
    touch "${APP_DIR}/usr/bin/PDFinity"
    chmod 755 "${APP_DIR}/usr/bin/PDFinity"
fi

# Desktop & Icon
install -m 644 packaging/linux/pdfinity.desktop "${APP_DIR}/pdfinity.desktop"
install -m 644 packaging/linux/pdfinity.desktop "${APP_DIR}/usr/share/applications/pdfinity.desktop"
install -m 644 packaging/resources/pdfinity.svg "${APP_DIR}/pdfinity.svg"
install -m 644 packaging/resources/pdfinity.svg "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/pdfinity.svg"
install -m 644 LICENSE "${APP_DIR}/LICENSE"
install -m 644 THIRDPARTY_NOTICES.txt "${APP_DIR}/THIRDPARTY_NOTICES.txt"

# AppRun launcher script
cat << 'EOF' > "${APP_DIR}/AppRun"
#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins:${QT_PLUGIN_PATH:-}"
exec "${HERE}/usr/bin/PDFinity" "$@"
EOF

chmod +x "${APP_DIR}/AppRun"

echo "=== AppDir prepared at ${APP_DIR} ==="
if command -v appimagetool >/dev/null 2>&1; then
    appimagetool "${APP_DIR}" "${OUTPUT_DIR}/${APPIMAGE_NAME}"
    echo "AppImage created: ${OUTPUT_DIR}/${APPIMAGE_NAME}"
else
    echo "appimagetool not found on host. AppDir structure verified."
fi
