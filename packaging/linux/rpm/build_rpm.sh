#!/bin/bash
set -euo pipefail

# PDFinity RPM Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)

SOURCE_VERSION="$(python3 scripts/release_versions.py --print)"
VERSION="${1:-${SOURCE_VERSION}}"
TOPDIR="$(pwd)/dist/rpmbuild"

echo "=== Staging RPM build for PDFinity ${VERSION} ==="

[ "${VERSION}" = "${SOURCE_VERSION}" ] || { echo "ERROR: RPM/source version mismatch" >&2; exit 1; }
[ "$(uname -m)" = x86_64 ] || { echo "ERROR: native RPM recipe requires x86_64" >&2; exit 1; }
[ ! -e "${TOPDIR}" ] || { echo "ERROR: stale RPM work directory; use a clean checkout/build" >&2; exit 1; }
mkdir -p "${TOPDIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp packaging/linux/rpm/pdfinity.spec "${TOPDIR}/SPECS/"

command -v docker >/dev/null
# Build the exact checkout, never fetch a mutable version URL or substitute current source.
git diff --quiet HEAD --
git archive --format=tar.gz --prefix="PDFinity-${VERSION}/" HEAD > "${TOPDIR}/SOURCES/v${VERSION}.tar.gz"
python3 - "${TOPDIR}/SOURCES" <<'PY'
import pathlib, sys
sys.path.insert(0, 'scripts')
from download_pdfium import PLATFORM_ARCHIVES, download_with_retry, verified_members
config = PLATFORM_ARCHIVES['linux']
data = download_with_retry(config['url'])
verified_members(data, config)
(pathlib.Path(sys.argv[1]) / config['archive']).write_bytes(data)
print('Verified locked PDFium RPM source archive: ' + config['sha256'])
PY
IMAGE="$(python3 -c 'import json; print(json.load(open("packaging/linux/rpm/native-runtime.json"))["image"])')"
[[ "${IMAGE}" =~ @sha256:[0-9a-f]{64}$ ]] || { echo "ERROR: RPM image must be pinned" >&2; exit 1; }
# Only the exact source archive/spec and output directory enter the disposable
# native build. No .git, host SDK, credentials or signing secrets are mounted.
docker run --rm --platform linux/amd64 --env QT_QPA_PLATFORM=offscreen \
    --mount "type=bind,src=${TOPDIR},dst=/build" "${IMAGE}" bash -euc '
    dnf -y install rpm-build redhat-rpm-config cmake cmake-rpm-macros ninja-build gcc-c++ \
        qt6-qtbase-devel xorg-x11-server-Xvfb xauth python3 binutils diffutils findutils patch
    rpm -qa --qf "%{NAME}\t%{VERSION}-%{RELEASE}\t%{ARCH}\n" | sort > /build/build-packages.tsv
    rpmbuild --define "_topdir /build" -ba /build/SPECS/pdfinity.spec
    '

mapfile -t packages < <(find "${TOPDIR}/RPMS" -name "pdfinity-${VERSION}-1.*.rpm" -type f)
[ "${#packages[@]}" -eq 1 ] || { echo "ERROR: expected exactly one binary RPM" >&2; exit 1; }
test -s "${packages[0]}"
cp -v "${packages[0]}" dist/
