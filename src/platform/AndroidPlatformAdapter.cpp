// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AndroidPlatformAdapter.hpp"
#include "application/Logger.hpp"
#include "application/AppContext.hpp"
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

#if defined(__ANDROID__)
#include <QJniObject>
#include <QJniEnvironment>
#include <jni.h>
#endif

namespace pdfinity::platform {

#if defined(__ANDROID__)
static void JNICALL NativeDocumentReceived(JNIEnv* env, jclass /*clazz*/, jstring jpath) {
    if (!jpath) return;
    const char* nativePath = env->GetStringUTFChars(jpath, nullptr);
    if (nativePath) {
        std::string pathStr(nativePath);
        env->ReleaseStringUTFChars(jpath, nativePath);
        LOG_INFO("AndroidPlatform", "Received document from Android Intent: " + pathStr);
        // Post event or notify application context
    }
}
#endif

AndroidPlatformAdapter::AndroidPlatformAdapter() {
    if (qApp) {
        connect(qApp, &QGuiApplication::applicationStateChanged,
                this, &AndroidPlatformAdapter::OnApplicationStateChanged);
    }
    RegisterJniHooks();
}

void AndroidPlatformAdapter::RegisterJniHooks() {
#if defined(__ANDROID__)
    JNINativeMethod methods[] = {
        {"onNativeDocumentReceived", "(Ljava/lang/String;)V", reinterpret_cast<void*>(NativeDocumentReceived)}
    };

    QJniEnvironment env;
    jclass activityClass = env.findClass("com/stacksyncsolutions/pdfinity/PDFinityActivity");
    if (activityClass) {
        env->RegisterNatives(activityClass, methods, sizeof(methods) / sizeof(methods[0]));
        LOG_INFO("AndroidPlatform", "Registered JNI callbacks for PDFinityActivity");
    }
#endif
}

void AndroidPlatformAdapter::OnApplicationStateChanged(Qt::ApplicationState state) {
    switch (state) {
        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
            LOG_INFO("AndroidPlatform", "Android app suspended/inactive. Persisting recovery state and flushing caches...");
            app::AppContext::Instance().SaveCurrentState();
            break;
        case Qt::ApplicationActive:
            LOG_INFO("AndroidPlatform", "Android app resumed to active foreground.");
            break;
        default:
            break;
    }
}

std::string AndroidPlatformAdapter::GetPlatformVersion() const {
    auto version = QOperatingSystemVersion::current();
    return "Android API " + std::to_string(version.majorVersion());
}

std::string AndroidPlatformAdapter::GetArchitecture() const {
#if defined(__aarch64__)
    return "arm64-v8a";
#elif defined(__arm__)
    return "armeabi-v7a";
#elif defined(__x86_64__)
    return "x86_64";
#elif defined(__i386__)
    return "x86";
#else
    return "Unknown";
#endif
}

bool AndroidPlatformAdapter::HasCapability(PlatformCapability capability) const {
    switch (capability) {
        case PlatformCapability::HardwareAcceleratedRendering:
        case PlatformCapability::SystemPrintSupport:
        case PlatformCapability::Tls13Support:
            return true;
        case PlatformCapability::PerMonitorDpiV2:
        case PlatformCapability::LocalMcpPipes:
        case PlatformCapability::DirectWriteFonts:
        case PlatformCapability::ModernCryptoCng:
        default:
            return false;
    }
}

SystemTheme AndroidPlatformAdapter::GetSystemTheme() const {
    return SystemTheme::Light;
}

double AndroidPlatformAdapter::GetPrimaryScreenDpi() const {
    if (auto* screen = QGuiApplication::primaryScreen()) {
        return screen->logicalDotsPerInch();
    }
    return 160.0; // Standard Android mdpi baseline
}

std::filesystem::path AndroidPlatformAdapter::GetAppDataDirectory() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty()) {
        path = "/data/data/com.stacksyncsolutions.pdfinity/files";
    }
    return std::filesystem::path(path.toStdString());
}

std::filesystem::path AndroidPlatformAdapter::GetCacheDirectory() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (path.isEmpty()) {
        path = "/data/data/com.stacksyncsolutions.pdfinity/cache";
    }
    return std::filesystem::path(path.toStdString());
}

std::filesystem::path AndroidPlatformAdapter::GetDocumentsDirectory() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    return std::filesystem::path(path.toStdString());
}

bool AndroidPlatformAdapter::OpenFolderAndSelectFile(const std::filesystem::path& /*filePath*/) const {
    return false;
}

bool AndroidPlatformAdapter::PrintDocument(const std::filesystem::path& filePath) const {
#if defined(__ANDROID__)
    QJniObject jPath = QJniObject::fromString(QString::fromStdString(filePath.string()));
    jboolean res = QJniObject::callStaticMethod<jboolean>(
        "com/stacksyncsolutions/pdfinity/PDFinityActivity",
        "printDocument",
        "(Ljava/lang/String;)Z",
        jPath.object<jstring>()
    );
    return static_cast<bool>(res);
#else
    (void)filePath;
    return false;
#endif
}

bool AndroidPlatformAdapter::ShareDocument(const std::filesystem::path& filePath, const std::string& /*mimeType*/) const {
#if defined(__ANDROID__)
    QJniObject jPath = QJniObject::fromString(QString::fromStdString(filePath.string()));
    jboolean res = QJniObject::callStaticMethod<jboolean>(
        "com/stacksyncsolutions/pdfinity/PDFinityActivity",
        "shareDocument",
        "(Ljava/lang/String;)Z",
        jPath.object<jstring>()
    );
    return static_cast<bool>(res);
#else
    (void)filePath;
    return false;
#endif
}

} // namespace pdfinity::platform
