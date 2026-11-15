#!/usr/bin/env bash
# Disposable source-only dependency rehearsal. Produces no product release asset.
set -euo pipefail
cd /work
[ ! -e builder ] && [ ! -e depot_tools ] || { echo "ERROR: clean PDFium source workspace required" >&2; exit 1; }
apt-get update
apt-get install -y ca-certificates git curl python3 sudo lsb-release build-essential cmake ninja-build pkg-config xz-utils file binutils patch
mapfile -t source_locks < <(python3 - <<'PY'
import json,re
c=json.load(open('/input/source-lock.json'))
for key in ('pdfium_source_sha','upstream_recipe_sha','depot_tools_sha'):
    assert re.fullmatch('[0-9a-f]{40}',c[key])
    print(c[key])
assert re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+',c['version'])
print(c['version'])
PY
)
PDFIUM_SHA="${source_locks[0]:?}"
RECIPE_SHA="${source_locks[1]:?}"
DEPOT_SHA="${source_locks[2]:?}"
export PDFium_VERSION="${source_locks[3]:?}"
git clone --filter=blob:none --no-checkout https://github.com/bblanchon/pdfium-binaries.git builder
git -C builder checkout --detach "$RECIPE_SHA"
[ "$(git -C builder rev-parse HEAD)" = "$RECIPE_SHA" ]
git clone --filter=blob:none --no-checkout https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
git -C depot_tools checkout --detach "$DEPOT_SHA"
[ "$(git -C depot_tools rev-parse HEAD)" = "$DEPOT_SHA" ]
export PATH="/work/depot_tools:$PATH"
export DEPOT_TOOLS_UPDATE=0
export DEPOT_TOOLS_WIN_TOOLCHAIN=0
# Initialize pinned bootstrap packages explicitly. Root-container execution
# skips update_depot_tools, including its normal Python bootstrap side effect.
# ensure_bootstrap documents that it never updates the repository checkout.
bash /work/depot_tools/ensure_bootstrap
[ "$(git -C /work/depot_tools rev-parse HEAD)" = "$DEPOT_SHA" ]
export PDFium_TARGET_OS=linux PDFium_TARGET_CPU=x64 PDFium_TARGET_ENVIRONMENT=""
export PDFium_ENABLE_V8=false PDFium_IS_DEBUG=false PDFium_BUILD_TYPE=shared
cd builder
export PDFium_SOURCE_DIR="$PWD/pdfium" PDFium_BUILD_DIR="$PWD/pdfium/out"
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git --custom-var checkout_configuration=minimal
printf "target_os = ['linux']\n" >> .gclient
gclient sync --revision "pdfium@$PDFIUM_SHA" --no-history --shallow
[ "$(git -C pdfium rev-parse HEAD)" = "$PDFIUM_SHA" ]
export SOURCE_DATE_EPOCH="$(git -C pdfium show -s --format=%ct HEAD)"
bash steps/03-patch.sh
bash steps/04-install-extras.sh
bash steps/05-configure.sh
# Chromium's supported official-build setting uses SHA-1 instead of the
# eight-byte LLD fast ID. Keep all normal RPM and runtime checks enabled.
# Standalone PDFium has no Chromium PGO profile updater. Retain the
# upstream non-profiled optimization mode while selecting the SHA-1 ID.
printf 'is_official_build = true\nchrome_pgo_phase = 0\n' >> "$PDFium_BUILD_DIR/args.gn"
(cd pdfium && gn gen "$PDFium_BUILD_DIR" --fail-on-unused-args)
ninja -C "$PDFium_BUILD_DIR" -j2 pdfium
mkdir -p /work/output
gclient revinfo -a > /work/output/dependency-revisions.txt
dpkg-query -W '-f=${binary:Package}\t${Version}\t${Architecture}\n' > /work/output/build-packages.tsv
"$PDFium_SOURCE_DIR/third_party/llvm-build/Release+Asserts/bin/clang" --version > /work/output/clang-version.txt
bash steps/07-stage.sh
bash steps/08-licenses.sh
bash steps/09-test.sh
python3 /checks/record_pdfium_source_build.py
