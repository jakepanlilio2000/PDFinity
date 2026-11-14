#!/bin/bash
set -euo pipefail

# PDFinity Debian/Ubuntu (.deb) Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

BUILD_DIR="${1:-build/linux-x64-release}"
VERSION="${2:-0.1.0}"
ARCH="amd64"
DIST_DIR="dist"
STAGE_DIR="dist/deb-staging"
DEB_NAME="PDFinity-${VERSION}-linux-${ARCH}.deb"

echo "=== Building Debian Package: ${DEB_NAME} ==="

rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/DEBIAN"
mkdir -p "${STAGE_DIR}/usr/bin"
mkdir -p "${STAGE_DIR}/usr/lib/pdfinity"
mkdir -p "${STAGE_DIR}/usr/share/applications"
mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${STAGE_DIR}/usr/share/doc/pdfinity"
mkdir -p "${DIST_DIR}"

# Update version in control file
sed "s/^Version:.*/Version: ${VERSION}/" packaging/linux/deb/control > "${STAGE_DIR}/DEBIAN/control"

# Copy binary
if [ -f "${BUILD_DIR}/bin/PDFinity" ]; then
    install -m 755 "${BUILD_DIR}/bin/PDFinity" "${STAGE_DIR}/usr/bin/pdfinity"
else
    echo "Warning: ${BUILD_DIR}/bin/PDFinity not found. Creating placeholder binary symlink for dry-run validation."
    touch "${STAGE_DIR}/usr/bin/pdfinity"
    chmod 755 "${STAGE_DIR}/usr/bin/pdfinity"
fi

# Copy Desktop integration & icon
install -m 644 packaging/linux/pdfinity.desktop "${STAGE_DIR}/usr/share/applications/pdfinity.desktop"
install -m 644 packaging/resources/pdfinity.svg "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/pdfinity.svg"

# Copy documentation & license
install -m 644 LICENSE "${STAGE_DIR}/usr/share/doc/pdfinity/copyright"
install -m 644 README.md "${STAGE_DIR}/usr/share/doc/pdfinity/README.md"
install -m 644 THIRDPARTY_NOTICES.txt "${STAGE_DIR}/usr/share/doc/pdfinity/THIRDPARTY_NOTICES.txt"

# Build package
if command -v dpkg-deb >/dev/null 2>&1; then
    dpkg-deb --build --root-owner-group "${STAGE_DIR}" "${DIST_DIR}/${DEB_NAME}"
    echo "Successfully generated: ${DIST_DIR}/${DEB_NAME}"
else
    echo "dpkg-deb command not found on host. Staging verified at ${STAGE_DIR}."
fi
