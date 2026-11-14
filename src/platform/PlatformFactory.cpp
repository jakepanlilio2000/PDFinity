// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "PlatformFactory.hpp"

#if defined(_WIN32)
#include "WindowsPlatformAdapter.hpp"
#endif

namespace pdfinity::platform {

#if !defined(_WIN32)
class GenericPlatformAdapter : public IPlatformAdapter {
public:
    GenericPlatformAdapter() {
        const char* portableEnv = std::getenv("PDFINITY_PORTABLE");
        if (portableEnv && std::string(portableEnv) == "1") {
            m_isPortable = true;
        } else if (std::filesystem::exists("portable.dat")) {
            m_isPortable = true;
        }
    }

    std::string GetPlatformName() const override {
#if defined(__APPLE__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#else
        return "Generic Unix";
#endif
    }

    std::string GetPlatformVersion() const override { return "1.0.0"; }

    std::string GetArchitecture() const override {
#if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "ARM64";
#else
        return "Unknown";
#endif
    }

    bool HasCapability(PlatformCapability capability) const override {
        switch (capability) {
            case PlatformCapability::HardwareAcceleratedRendering:
            case PlatformCapability::SystemPrintSupport:
            case PlatformCapability::LocalMcpPipes:
            case PlatformCapability::Tls13Support:
                return true;
            default:
                return false;
        }
    }

    SystemTheme GetSystemTheme() const override { return SystemTheme::Light; }
    double GetPrimaryScreenDpi() const override { return 96.0; }

    bool IsPortableMode() const override {
        return m_isPortable;
    }

    std::filesystem::path GetAppDataDirectory() const override {
        if (m_isPortable) {
            return std::filesystem::current_path() / "data";
        }
#if defined(__APPLE__)
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Library" / "Application Support" / "PDFinity";
        }
#else
        const char* xdgData = std::getenv("XDG_DATA_HOME");
        if (xdgData && *xdgData) {
            return std::filesystem::path(xdgData) / "PDFinity";
        }
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / ".local" / "share" / "PDFinity";
        }
#endif
        return std::filesystem::current_path() / ".pdfinity";
    }

    std::filesystem::path GetCacheDirectory() const override {
        if (m_isPortable) {
            return std::filesystem::current_path() / "data" / "cache";
        }
#if defined(__APPLE__)
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Library" / "Caches" / "PDFinity";
        }
#else
        const char* xdgCache = std::getenv("XDG_CACHE_HOME");
        if (xdgCache && *xdgCache) {
            return std::filesystem::path(xdgCache) / "PDFinity";
        }
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / ".cache" / "PDFinity";
        }
#endif
        return GetAppDataDirectory() / "cache";
    }

    std::filesystem::path GetDocumentsDirectory() const override {
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Documents";
        }
        return std::filesystem::current_path();
    }

    bool OpenFolderAndSelectFile(const std::filesystem::path&) const override {
        return false;
    }

private:
    bool m_isPortable{false};
};
#endif

std::unique_ptr<IPlatformAdapter> PlatformFactory::CreateAdapter() {
#if defined(_WIN32)
    return std::make_unique<WindowsPlatformAdapter>();
#else
    return std::make_unique<GenericPlatformAdapter>();
#endif
}

} // namespace pdfinity::platform
