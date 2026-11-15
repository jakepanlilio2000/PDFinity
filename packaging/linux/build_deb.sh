#!/bin/bash
set -euo pipefail

# PDFinity Debian/Ubuntu (.deb) Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

BUILD_DIR="${1:-build/linux-x64-release}"
SOURCE_VERSION="$(python3 scripts/release_versions.py --print)"
VERSION="${2:-${SOURCE_VERSION}}"
ARCH="${3:-amd64}"
DIST_DIR="dist"
STAGE_DIR="dist/deb-staging"
DEB_NAME="pdfinity_${VERSION}_${ARCH}.deb"

echo "=== Building Debian Package: ${DEB_NAME} ==="
[ "${VERSION}" = "${SOURCE_VERSION}" ] || { echo "ERROR: package/source version mismatch" >&2; exit 1; }
[ "${ARCH}" = "$(dpkg --print-architecture)" ] || { echo "ERROR: package/native architecture mismatch" >&2; exit 1; }

# Use CMake's installed executables: raw build-tree copies retain development RPATHs.
INSTALL_ROOT="$(mktemp -d "$(realpath "${BUILD_DIR}")/deb-install.XXXXXX")"
trap 'rm -rf -- "${INSTALL_ROOT}"' EXIT
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_ROOT}/usr" --strip

rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/DEBIAN"
mkdir -p "${STAGE_DIR}/usr/bin"
mkdir -p "${STAGE_DIR}/usr/lib/pdfinity"
mkdir -p "${STAGE_DIR}/usr/share/applications"
mkdir -p "${STAGE_DIR}/usr/share/metainfo"
mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${STAGE_DIR}/usr/share/doc/pdfinity"
mkdir -p "${DIST_DIR}"

# 1. Control and maintainer scripts
sed -e "s/^Version:.*/Version: ${VERSION}/" -e "s/^Architecture:.*/Architecture: ${ARCH}/" \
    packaging/linux/deb/control > "${STAGE_DIR}/DEBIAN/control"

if [ -f packaging/linux/deb/postinst ]; then
    install -m 755 packaging/linux/deb/postinst "${STAGE_DIR}/DEBIAN/postinst"
fi
if [ -f packaging/linux/deb/postrm ]; then
    install -m 755 packaging/linux/deb/postrm "${STAGE_DIR}/DEBIAN/postrm"
fi

# 2. Main binary
if [ -f "${INSTALL_ROOT}/usr/bin/PDFinity" ]; then
    install -m 755 "${INSTALL_ROOT}/usr/bin/PDFinity" "${STAGE_DIR}/usr/bin/pdfinity"
else
    echo "ERROR: native PDFinity executable missing; no placeholder will be generated" >&2
    exit 1
fi

test -s "third_party/pdfium/lib/libpdfium.so"
test -s "${INSTALL_ROOT}/usr/bin/pdfinity-cli"
if [ -f "third_party/pdfium/lib/libpdfium.so" ]; then
    install -m 755 "third_party/pdfium/lib/libpdfium.so" "${STAGE_DIR}/usr/lib/libpdfium.so"
fi

install -m 755 "${INSTALL_ROOT}/usr/bin/pdfinity-cli" "${STAGE_DIR}/usr/bin/pdfinity-cli"

# 3. Desktop integration & AppStream metainfo
install -m 644 packaging/linux/com.stacksyncsolutions.pdfinity.desktop "${STAGE_DIR}/usr/share/applications/com.stacksyncsolutions.pdfinity.desktop"
ln -sf com.stacksyncsolutions.pdfinity.desktop "${STAGE_DIR}/usr/share/applications/pdfinity.desktop"
install -m 644 packaging/linux/com.stacksyncsolutions.pdfinity.metainfo.xml "${STAGE_DIR}/usr/share/metainfo/com.stacksyncsolutions.pdfinity.metainfo.xml"

# 4. Multi-resolution hicolor icons
for res in 16 24 32 48 64 128 256 512; do
    mkdir -p "${STAGE_DIR}/usr/share/icons/hicolor/${res}x${res}/apps"
    if [ -f "packaging/resources/hicolor/${res}x${res}/apps/pdfinity.png" ]; then
        install -m 644 "packaging/resources/hicolor/${res}x${res}/apps/pdfinity.png" "${STAGE_DIR}/usr/share/icons/hicolor/${res}x${res}/apps/pdfinity.png"
        install -m 644 "packaging/resources/hicolor/${res}x${res}/apps/com.stacksyncsolutions.pdfinity.png" "${STAGE_DIR}/usr/share/icons/hicolor/${res}x${res}/apps/com.stacksyncsolutions.pdfinity.png"
    fi
done
install -m 644 packaging/resources/pdfinity.svg "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/pdfinity.svg"
install -m 644 packaging/resources/pdfinity.svg "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/com.stacksyncsolutions.pdfinity.svg"

# 5. Documentation and licenses
install -m 644 LICENSE "${STAGE_DIR}/usr/share/doc/pdfinity/copyright"
install -m 644 README.md "${STAGE_DIR}/usr/share/doc/pdfinity/README.md"
install -m 644 THIRD_PARTY_LICENSES.md "${STAGE_DIR}/usr/share/doc/pdfinity/THIRD_PARTY_LICENSES.md"
install -m 644 packaging/linux/METADATA_LICENSE "${STAGE_DIR}/usr/share/doc/pdfinity/METADATA_LICENSE"

# Derive ABI/version dependencies from all shipped ELF files, including PDFium.
# SDK-only libraries with no dpkg ownership must fail; never ignore missing info.
STAGE_ABS="$(realpath "${STAGE_DIR}")"
mkdir -p "${INSTALL_ROOT}/dependency-work/debian"
cat > "${INSTALL_ROOT}/dependency-work/debian/control" <<'EOF'
Source: pdfinity
Maintainer: StackSync Solutions <support@stacksyncsolutions.com>
Section: text
Priority: optional

Package: pdfinity
Architecture: any
Depends: ${shlibs:Depends}
Description: PDFinity runtime dependency analysis
EOF
DEPENDENCIES="$(cd "${INSTALL_ROOT}/dependency-work" && dpkg-shlibdeps -O \
    -l"${STAGE_ABS}/usr/lib" -S"${STAGE_ABS}" -xpdfinity \
    -e"${STAGE_ABS}/usr/bin/pdfinity" -e"${STAGE_ABS}/usr/bin/pdfinity-cli" \
    -e"${STAGE_ABS}/usr/lib/libpdfium.so" | sed -n 's/^shlibs:Depends=//p')"
test -n "${DEPENDENCIES}"
INSTALLED_SIZE="$(du -sk "${STAGE_DIR}/usr" | cut -f1)"
sed -e "s|^Depends:.*|Depends: ${DEPENDENCIES}, qt6-qpa-plugins|" \
    -e "s|^Installed-Size:.*|Installed-Size: ${INSTALLED_SIZE}|" \
    "${STAGE_DIR}/DEBIAN/control" > "${STAGE_DIR}/DEBIAN/control.generated"
mv "${STAGE_DIR}/DEBIAN/control.generated" "${STAGE_DIR}/DEBIAN/control"

# 6. Build and validate package
if command -v dpkg-deb >/dev/null 2>&1; then
    # Qt's worker uses Python 3.11: its stdlib tar reader supports XZ, not Zstd.
    # Do not depend on the host distribution's default archive compression.
    dpkg-deb -Zxz --build --root-owner-group "${STAGE_DIR}" "${DIST_DIR}/${DEB_NAME}"
    echo "=== Validating Debian Package ==="
    dpkg-deb --info "${DIST_DIR}/${DEB_NAME}"
    dpkg-deb --contents "${DIST_DIR}/${DEB_NAME}"
    echo "[+] DEB built and parsed by dpkg-deb; native install/runtime checks remain required: ${DIST_DIR}/${DEB_NAME}"
else
    echo "ERROR: dpkg-deb is required" >&2
    exit 1
fi
