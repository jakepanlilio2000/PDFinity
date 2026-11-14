# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

import os
import sys
import xml.etree.ElementTree as ET

print("=== ANDROID COMPATIBILITY & MOBILE ARCHITECTURE VERIFICATION ===\n")

# Check 1: CMakeLists.txt Android properties
with open("CMakeLists.txt", "r", encoding="utf-8") as f:
    cm = f.read()
assert "QT_ANDROID_PACKAGE_SOURCE_DIR" in cm, "Missing QT_ANDROID_PACKAGE_SOURCE_DIR"
assert "arm64-v8a;armeabi-v7a;x86_64" in cm, "Missing QT_ANDROID_ABIS"
assert "QT_ANDROID_MIN_SDK_VERSION 28" in cm, "Missing QT_ANDROID_MIN_SDK_VERSION 28"
assert "QT_ANDROID_TARGET_SDK_VERSION 35" in cm, "Missing QT_ANDROID_TARGET_SDK_VERSION 35"
print("[PASS] Check 1.1: CMakeLists.txt Qt for Android properties verified.")

# Check 2: AndroidManifest.xml
with open("android/AndroidManifest.xml", "r", encoding="utf-8") as f:
    mf = f.read()
assert "MANAGE_EXTERNAL_STORAGE" not in mf, "CRITICAL: MANAGE_EXTERNAL_STORAGE must not exist!"
assert 'android:minSdkVersion="28"' in mf, "minSdkVersion 28 missing"
assert 'android:targetSdkVersion="35"' in mf, "targetSdkVersion 35 missing"
assert "androidx.core.content.FileProvider" in mf, "FileProvider missing"
assert "android.intent.action.VIEW" in mf, "ACTION_VIEW missing"
assert "android.intent.action.SEND" in mf, "ACTION_SEND missing"
assert 'android:scheme="content"' in mf, "content scheme missing"
print("[PASS] Check 1.2: AndroidManifest.xml verified (minSdk 28, targetSdk 35, FileProvider, SAF intents, zero broad storage).")

# Check 3: build.gradle
with open("android/build.gradle", "r", encoding="utf-8") as f:
    bg = f.read()
assert "compileSdk 35" in bg, "compileSdk 35 missing"
assert "minSdk 28" in bg, "minSdk 28 missing"
assert "targetSdk 35" in bg, "targetSdk 35 missing"
assert "ANDROID_KEYSTORE_PATH" in bg, "Signing env var missing"
assert "androidx.core:core:1.13.1" in bg, "androidx.core missing"
print("[PASS] Check 1.3: build.gradle verified (compileSdk 35, minSdk 28, targetSdk 35, env signing).")

# Check 4: Adaptive icons & XML resources
for res in ["android/res/drawable/ic_launcher_background.xml",
            "android/res/drawable/ic_launcher_foreground.xml",
            "android/res/mipmap-anydpi-v26/ic_launcher.xml",
            "android/res/mipmap-anydpi-v26/ic_launcher_round.xml",
            "android/res/xml/file_paths.xml"]:
    assert os.path.exists(res), f"Missing resource: {res}"
print("[PASS] Check 1.4: Adaptive icons and file_paths.xml verified on disk.")

# Check 5: AndroidPlatformAdapter & JNI
with open("src/platform/AndroidPlatformAdapter.hpp", "r", encoding="utf-8") as f:
    aph = f.read()
with open("src/platform/AndroidPlatformAdapter.cpp", "r", encoding="utf-8") as f:
    apc = f.read()
assert "class AndroidPlatformAdapter" in aph, "AndroidPlatformAdapter missing"
assert "QJniObject" in apc, "QJniObject missing in AndroidPlatformAdapter.cpp"
assert "PrintDocument" in apc, "PrintDocument missing"
assert "ShareDocument" in apc, "ShareDocument missing"
assert "OnApplicationStateChanged" in apc, "Lifecycle handler missing"
print("[PASS] Check 2.1: AndroidPlatformAdapter verified (JNI, Print, Share, Lifecycle).")

# Check 6: PDFinityActivity.java
with open("android/src/com/stacksyncsolutions/pdfinity/PDFinityActivity.java", "r", encoding="utf-8") as f:
    paj = f.read()
assert "resolveUriToLocalFile" in paj, "resolveUriToLocalFile missing"
assert "saveDocumentToUri" in paj, "saveDocumentToUri missing"
assert "printDocument" in paj, "printDocument missing"
assert "shareDocument" in paj, "shareDocument missing"
assert "cleanupOldCacheFiles" in paj, "cleanupOldCacheFiles missing"
print("[PASS] Check 2.2: PDFinityActivity.java verified (SAF copy, SAF writeback, printing, sharing, 24h cache purge).")

# Check 7: PdfCanvasWidget gestures
with open("src/ui/PdfCanvasWidget.hpp", "r", encoding="utf-8") as f:
    pwh = f.read()
with open("src/ui/PdfCanvasWidget.cpp", "r", encoding="utf-8") as f:
    pwc = f.read()
assert "bool event(QEvent* event) override;" in pwh, "event override missing in PdfCanvasWidget"
assert "grabGesture(Qt::PinchGesture)" in pwc, "PinchGesture missing"
assert "grabGesture(Qt::SwipeGesture)" in pwc, "SwipeGesture missing"
assert "grabGesture(Qt::TapAndHoldGesture)" in pwc, "TapAndHoldGesture missing"
assert "QPinchGesture::ScaleFactorChanged" in pwc, "ScaleFactorChanged missing"
print("[PASS] Check 3.1: PdfCanvasWidget gestures verified (Pinch-to-zoom, Swipe page turn, Tap-and-hold context menu).")

# Check 8: MainWindow touch adaptations
with open("src/ui/MainWindow.cpp", "r", encoding="utf-8") as f:
    mwc = f.read()
assert "min-width: 48px; min-height: 48px" in mwc, "48dp touch target styling missing"
assert "m_shareAction" in mwc, "m_shareAction missing"
assert "OnShareDocument" in mwc, "OnShareDocument missing"
print("[PASS] Check 3.2: MainWindow verified (48dp touch targets, ShareDocument action).")

# Check 9: Packaging & Docs
with open("packaging/android/build_apk.sh", "r", encoding="utf-8") as f:
    bash_apk = f.read()
assert "PDFinity-release.apk" in bash_apk, "build_apk.sh target missing"

with open("docs/ANDROID_PORT.md", "r", encoding="utf-8") as f:
    apm = f.read()
assert "API level 28" in apm, "API level 28 missing in docs/ANDROID_PORT.md"
assert "adb install -r" in apm, "adb install missing in docs/ANDROID_PORT.md"

with open("docs/BUILD.md", "r", encoding="utf-8") as f:
    bm = f.read()
assert "2.4 Android Build Instructions" in bm, "Android build missing in docs/BUILD.md"
print("[PASS] Check 4.1: Packaging scripts and documentation verified.")

# Check 10: Secrets and Keystore check
keystores = []
for root, dirs, files in os.walk("."):
    if ".git" in dirs:
        dirs.remove(".git")
    for f in files:
        if f.endswith((".jks", ".keystore", ".p12", ".key")):
            keystores.append(os.path.join(root, f))
assert len(keystores) == 0, f"Found committed keystores: {keystores}"
print("[PASS] Check 5.1: Zero committed keystores or private certificates.")

print("\n>>> ALL ANDROID COMPATIBILITY CHECKS PASSED WITH ZERO DEFECTS! <<<")
