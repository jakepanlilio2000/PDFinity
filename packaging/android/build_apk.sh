#!/usr/bin/env bash
# ==============================================================================
# PDFinity - Android APK Build & Packaging Script
# Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
# Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
# License: Mozilla Public License Version 2.0 (MPL-2.0)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
if [ -d "${REPO_ROOT}/packaging/android" ] && [ -f "${REPO_ROOT}/packaging/android/build.gradle" ]; then
    ANDROID_DIR="${REPO_ROOT}/packaging/android"
else
    ANDROID_DIR="${REPO_ROOT}/android"
fi
DIST_DIR="${REPO_ROOT}/dist"
BUILD_TYPE="${1:-Release}"
VERSION="$(python3 "${REPO_ROOT}/scripts/release_versions.py" --print)"

echo "=========================================================="
echo " PDFinity Android APK Packaging"
echo " Publisher: StackSync Solutions"
echo " Lead Programmer: Jake Ashley Panlilio"
echo " Build Type: ${BUILD_TYPE}"
echo "=========================================================="

if [ -z "${ANDROID_SDK_ROOT:-}" ] && [ -z "${ANDROID_HOME:-}" ]; then
    echo "[-] Error: ANDROID_SDK_ROOT or ANDROID_HOME environment variable is not set."
    echo "[-] Please export ANDROID_SDK_ROOT=/path/to/android-sdk"
    exit 1
fi

ANDROID_SDK="${ANDROID_SDK_ROOT:-$ANDROID_HOME}"
echo "[+] Using Android SDK: ${ANDROID_SDK}"

if [ -n "${ANDROID_NDK_ROOT:-}" ]; then
    echo "[+] Using Android NDK: ${ANDROID_NDK_ROOT}"
fi

mkdir -p "${DIST_DIR}"

cd "${ANDROID_DIR}"

if [ -f "./gradlew" ]; then
    GRADLE_CMD="./gradlew"
else
    GRADLE_CMD="gradle"
fi

echo "[+] Building Android APK via Gradle..."
if [ "${BUILD_TYPE}" = "Debug" ]; then
    ${GRADLE_CMD} assembleDebug
    APK_CANDIDATE=$(find "${ANDROID_DIR}/build/outputs/apk" -name "*debug*.apk" 2>/dev/null | head -n 1)
    TARGET_APK="${DIST_DIR}/PDFinity-debug.apk"
else
    ${GRADLE_CMD} assembleRelease bundleRelease
    APK_CANDIDATE=$(find "${ANDROID_DIR}/build/outputs/apk" -name "*release*.apk" 2>/dev/null | head -n 1)
    TARGET_APK="${DIST_DIR}/PDFinity-${VERSION}-android-universal-release.apk"
fi

if [ -n "${APK_CANDIDATE:-}" ] && [ -f "${APK_CANDIDATE}" ]; then
    cp -v "${APK_CANDIDATE}" "${TARGET_APK}"
    echo "[+] Successfully packaged APK: ${TARGET_APK}"
    ls -lh "${TARGET_APK}"
else
    echo "ERROR: required APK missing after Gradle success" >&2
    exit 1
fi

if [ "${BUILD_TYPE}" != "Debug" ]; then
    mapfile -t bundles < <(find "${ANDROID_DIR}/build/outputs/bundle" -name '*release*.aab' -type f)
    [ "${#bundles[@]}" -eq 1 ] || { echo "ERROR: required unique release AAB missing" >&2; exit 1; }
    test -s "${bundles[0]}"
    cp "${bundles[0]}" "${DIST_DIR}/PDFinity-${VERSION}-android-universal-release.aab"
fi

echo "=========================================================="
echo " Android build process completed successfully."
echo "=========================================================="
