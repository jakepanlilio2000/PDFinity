#!/usr/bin/env bash
# PDFinity Linux Portable Tarball Packaging Script
set -euo pipefail

BUILD_DIR="${1:-build/linux-x64-release}"
VERSION="${2:-1.1.0}"
DIST_DIR="dist"
STAGE_DIR="${DIST_DIR}/PDFinity-${VERSION}-linux-x64"

echo "=== Staging Linux Portable Tarball ==="
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/bin"
mkdir -p "${STAGE_DIR}/lib"

cp "${BUILD_DIR}/bin/PDFinity" "${STAGE_DIR}/bin/"
if [ -f "third_party/pdfium/lib/libpdfium.so" ]; then
    cp "third_party/pdfium/lib/libpdfium.so" "${STAGE_DIR}/lib/"
fi
cp LICENSE "${STAGE_DIR}/"
cp THIRDPARTY_NOTICES.txt "${STAGE_DIR}/"
cp README.md "${STAGE_DIR}/"

# Launcher wrapper
cat << 'EOF' > "${STAGE_DIR}/pdfinity.sh"
#!/usr/bin/env bash
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="${HERE}/lib:${LD_LIBRARY_PATH:-}"
exec "${HERE}/bin/PDFinity" "$@"
EOF
chmod +x "${STAGE_DIR}/pdfinity.sh"

echo "=== Creating tar.gz archive ==="
tar -czf "${DIST_DIR}/PDFinity-${VERSION}-linux-x64.tar.gz" -C "${DIST_DIR}" "PDFinity-${VERSION}-linux-x64"
echo "=== Staged: ${DIST_DIR}/PDFinity-${VERSION}-linux-x64.tar.gz ==="
