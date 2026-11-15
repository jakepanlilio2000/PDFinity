#!/usr/bin/env bash
# Executed only in the pinned disposable build container; no host SDK or secrets.
set -euo pipefail
apt-get update
apt-get install -y --no-install-recommends ca-certificates python3 build-essential ninja-build \
    qt6-base-dev qt6-base-dev-tools qt6-qpa-plugins libqt6svg6 qt6-image-formats-plugins \
    libgl1-mesa-dev libxkbcommon-dev libcups2-dev xvfb xauth file binutils git
mkdir /opt/cmake
tar -xf /input/cmake.tar.gz -C /opt/cmake --strip-components=1
export PATH="/opt/cmake/bin:${PATH}"
cd /source
python3 scripts/download_pdfium.py --platform linux
cmake --preset linux-x64-release
cmake --build --preset linux-x64-release --parallel 2
QT_QPA_PLATFORM=offscreen xvfb-run -a ctest --preset linux-x64-release --output-on-failure \
    --no-tests=error --output-junit /output/ctest.xml
bash packaging/linux/appimage/build-appimage.sh build/linux-x64-release "${PDFINITY_VERSION}"
python3 - <<'PY'
import hashlib, json, os, pathlib, subprocess, xml.etree.ElementTree as ET
package = pathlib.Path('dist') / ('PDFinity-' + os.environ['PDFINITY_VERSION'] + '-x86_64.AppImage')
junit = pathlib.Path('/output/ctest.xml')
suites = list(ET.fromstring(junit.read_text()).iter('testsuite'))
total = sum(int(s.get('tests', 0)) for s in suites)
assert total >= 51 and all(int(s.get(k, 0)) == 0 for s in suites for k in ('failures', 'errors', 'disabled', 'skipped'))
proof = {'source_sha': os.environ['PDFINITY_SOURCE_SHA'],
    'source_archive_sha256': hashlib.sha256(pathlib.Path('/input/source.tar').read_bytes()).hexdigest(),
    'source_date_epoch': os.environ['SOURCE_DATE_EPOCH'],
    'toolchain': json.loads(pathlib.Path('packaging/linux/appimage/build-toolchain.json').read_text()),
    'package_sha256': hashlib.sha256(package.read_bytes()).hexdigest(),
    'cmake_version': subprocess.check_output(['cmake', '--version'], text=True).splitlines()[0],
    'compiler_version': subprocess.check_output(['c++', '--version'], text=True).splitlines()[0],
    'build_packages': subprocess.check_output(['dpkg-query', '-W',
        '-f=${binary:Package}\t${Version}\t${Architecture}\t${source:Package}\t${source:Version}\n'], text=True),
    'cpp_tests': 'passed', 'cpp_test_suites': total,
    'cpp_junit_sha256': hashlib.sha256(junit.read_bytes()).hexdigest(), 'release_eligible': False}
pathlib.Path('/output/appimage-build.json').write_text(json.dumps(proof, indent=2) + '\n')
PY
cp "dist/PDFinity-${PDFINITY_VERSION}-x86_64.AppImage" /output/
