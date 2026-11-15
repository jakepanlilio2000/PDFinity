#!/usr/bin/env bash
# Build the existing AppImage contract with the reviewed, hash-locked Qt deployer.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
cd "${REPO_ROOT}"
BUILD_DIR="$(realpath "${1:-build/linux-x64-release}")"
SOURCE_VERSION="$(python3 scripts/release_versions.py --print)"
VERSION="${2:-${SOURCE_VERSION}}"
APP_DIR="${REPO_ROOT}/dist/AppDir"
OUTPUT="${REPO_ROOT}/dist/PDFinity-${VERSION}-x86_64.AppImage"
TOOLS="${REPO_ROOT}/build/appimage-tools"

[ "${VERSION}" = "${SOURCE_VERSION}" ] || { echo "ERROR: AppImage/source version mismatch" >&2; exit 1; }
[ "$(uname -m)" = x86_64 ] || { echo "ERROR: AppImage requires the native x86_64 toolchain" >&2; exit 1; }
[ ! -e "${APP_DIR}" ] && [ ! -e "${OUTPUT}" ] || { echo "ERROR: stale AppImage output; use a clean build" >&2; exit 1; }
python3 scripts/download_pdfium.py --platform linux --verify-only
python3 scripts/download_appimage_tools.py --directory "${TOOLS}"

# Install RPATHs rather than ship executables pointing into the build tree.
mkdir -p "${APP_DIR}"
cmake --install "${BUILD_DIR}" --prefix "${APP_DIR}/usr" --strip
mv "${APP_DIR}/usr/bin/PDFinity" "${APP_DIR}/usr/bin/pdfinity"
test -s "${APP_DIR}/usr/bin/pdfinity-cli"
rm "${APP_DIR}/usr/lib/libpdfinity_core.a" "${APP_DIR}/usr/lib/libpdfinity_ui.a"
install -m 755 third_party/pdfium/lib/libpdfium.so "${APP_DIR}/usr/lib/libpdfium.so"
mkdir -p "${APP_DIR}/usr/share/licenses/pdfinity/pdfium"
cp -a third_party/pdfium/licenses/. "${APP_DIR}/usr/share/licenses/pdfinity/pdfium/"
install -m 644 packaging/linux/appimage/TYPE2_RUNTIME_LICENSE "${APP_DIR}/usr/share/licenses/pdfinity/TYPE2_RUNTIME_LICENSE"
install -m 644 packaging/linux/appimage/toolchain.json "${APP_DIR}/usr/share/doc/pdfinity/appimage-toolchain.json"

export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="$(command -v qmake6)"
export EXTRA_QT_MODULES=svg
export VERSION
export SOURCE_DATE_EPOCH="$(git show -s --format=%ct HEAD)"
# Keep deployment of transitive library copyrights enabled in both tools.
unset DISABLE_COPYRIGHT_FILES_DEPLOYMENT
"${TOOLS}/linuxdeploy-x86_64.AppImage" --appdir "${APP_DIR}" \
    --executable "${APP_DIR}/usr/bin/pdfinity" \
    --executable "${APP_DIR}/usr/bin/pdfinity-cli" \
    --library "${APP_DIR}/usr/lib/libpdfium.so" \
    --desktop-file "${APP_DIR}/usr/share/applications/com.stacksyncsolutions.pdfinity.desktop" \
    --icon-file "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/com.stacksyncsolutions.pdfinity.svg" \
    --plugin qt
# An explicit locked runtime prevents appimagetool's automatic latest download.
ARCH=x86_64 "${TOOLS}/appimagetool-x86_64.AppImage" \
    --runtime-file "${TOOLS}/runtime-x86_64" "${APP_DIR}" "${OUTPUT}"
test -s "${OUTPUT}"
echo "Built ${OUTPUT}; clean native runtime qualification remains required"
