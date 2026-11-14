#!/bin/bash
set -euo pipefail

# PDFinity RPM Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

VERSION="${1:-1.1.0}"
TOPDIR="$(pwd)/dist/rpmbuild"

echo "=== Staging RPM build for PDFinity ${VERSION} ==="

mkdir -p "${TOPDIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp packaging/linux/rpm/pdfinity.spec "${TOPDIR}/SPECS/"

command -v rpmbuild >/dev/null
# Build the exact checkout, never fetch a mutable version URL or substitute current source.
git diff --quiet HEAD --
git archive --format=tar.gz --prefix="PDFinity-${VERSION}/" HEAD > "${TOPDIR}/SOURCES/v${VERSION}.tar.gz"
rpmbuild --define "_topdir ${TOPDIR}" -ba "${TOPDIR}/SPECS/pdfinity.spec"
mapfile -t packages < <(find "${TOPDIR}/RPMS" -name "pdfinity-${VERSION}-1.*.rpm" -type f)
[ "${#packages[@]}" -eq 1 ] || { echo "ERROR: expected exactly one binary RPM" >&2; exit 1; }
test -s "${packages[0]}"
cp -v "${packages[0]}" dist/
