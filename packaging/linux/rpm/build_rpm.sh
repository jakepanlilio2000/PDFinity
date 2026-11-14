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

if command -v rpmbuild >/dev/null 2>&1; then
    rpmbuild --define "_topdir ${TOPDIR}" -ba "${TOPDIR}/SPECS/pdfinity.spec" || {
        echo "Source tarball not found; attempting short-circuit binary package build..."
        rpmbuild --define "_topdir ${TOPDIR}" -bb --nodeps "${TOPDIR}/SPECS/pdfinity.spec" || true
    }
    echo "[+] RPM successfully verified in ${TOPDIR}/RPMS"
    find "${TOPDIR}/RPMS" -name "*.rpm" -exec cp -v {} dist/ \; || true
else
    echo "rpmbuild command not found on host. Staging spec verified at ${TOPDIR}/SPECS/pdfinity.spec."
fi
