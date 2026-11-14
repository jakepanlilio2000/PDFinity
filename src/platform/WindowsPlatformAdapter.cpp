// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "WindowsPlatformAdapter.hpp"

#ifdef _WIN32
#include <shlobj.h>
#include <shellapi.h>
#include <sstream>

namespace pdfinity::platform {

typedef LONG(NTAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

WindowsPlatformAdapter::WindowsPlatformAdapter() {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto rtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
            if (rtlGetVersion(&osvi) == 0) {
                m_majorVersion = osvi.dwMajorVersion;
                m_minorVersion = osvi.dwMinorVersion;
                m_buildNumber = osvi.dwBuildNumber;
            }
        }
    }
    m_isWindows10OrGreater = (m_majorVersion > 10) || (m_majorVersion == 10 && m_buildNumber >= 10240);

    // Check for portable mode marker (portable.dat adjacent to executable)
    wchar_t exeBuffer[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::filesystem::path exePath(exeBuffer);
        m_exeDir = exePath.parent_path();
        std::error_code ec;
        if (std::filesystem::exists(m_exeDir / "portable.dat", ec)) {
            m_isPortable = true;
        }
    }
}

std::string WindowsPlatformAdapter::GetPlatformName() const {
    if (m_majorVersion == 10 && m_buildNumber >= 22000) {
        return "Windows 11";
    }
    if (m_majorVersion == 10) {
        return "Windows 10";
    }
    if (m_majorVersion == 6 && m_minorVersion == 1) {
        return "Windows 7 SP1 (Legacy)";
    }
    return "Windows";
}

std::string WindowsPlatformAdapter::GetPlatformVersion() const {
    std::ostringstream oss;
    oss << m_majorVersion << "." << m_minorVersion << "." << m_buildNumber;
    return oss.str();
}

std::string WindowsPlatformAdapter::GetArchitecture() const {
#if defined(_M_X64) || defined(__x86_64__)
    return "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
    return "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
    return "ARM64";
#else
    return "Unknown";
#endif
}

bool WindowsPlatformAdapter::HasCapability(PlatformCapability capability) const {
    switch (capability) {
        case PlatformCapability::PerMonitorDpiV2:
            return m_buildNumber >= 15063; // Windows 10 Creators Update 1703+
        case PlatformCapability::HardwareAcceleratedRendering:
            return true;
        case PlatformCapability::DirectWriteFonts:
            return true;
        case PlatformCapability::SystemPrintSupport:
            return true;
        case PlatformCapability::ModernCryptoCng:
            return m_isWindows10OrGreater;
        case PlatformCapability::LocalMcpPipes:
            return true;
        case PlatformCapability::Tls13Support:
            return m_buildNumber >= 20348; // Windows 11 / Server 2022+ native TLS 1.3
        default:
            return false;
    }
}

SystemTheme WindowsPlatformAdapter::GetSystemTheme() const {
    HKEY hKey = nullptr;
    const wchar_t* subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD appsUseLightTheme = 1;
        DWORD dataSize = sizeof(appsUseLightTheme);
        DWORD type = REG_DWORD;
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, &type, reinterpret_cast<LPBYTE>(&appsUseLightTheme), &dataSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (appsUseLightTheme == 0) ? SystemTheme::Dark : SystemTheme::Light;
        }
        RegCloseKey(hKey);
    }
    return SystemTheme::Light;
}

double WindowsPlatformAdapter::GetPrimaryScreenDpi() const {
    HDC screenDc = GetDC(nullptr);
    if (screenDc) {
        int dpiX = GetDeviceCaps(screenDc, LOGPIXELSX);
        ReleaseDC(nullptr, screenDc);
        return static_cast<double>(dpiX);
    }
    return 96.0;
}

bool WindowsPlatformAdapter::IsPortableMode() const {
    return m_isPortable;
}

std::filesystem::path WindowsPlatformAdapter::GetAppDataDirectory() const {
    if (m_isPortable) {
        std::filesystem::path portableDir = m_exeDir / "data";
        std::error_code ec;
        std::filesystem::create_directories(portableDir, ec);
        return portableDir;
    }
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        result /= "PDFinity";
        return result;
    }
    return std::filesystem::current_path() / ".pdfinity";
}

std::filesystem::path WindowsPlatformAdapter::GetCacheDirectory() const {
    if (m_isPortable) {
        std::filesystem::path portableCache = m_exeDir / "data" / "cache";
        std::error_code ec;
        std::filesystem::create_directories(portableCache, ec);
        return portableCache;
    }
    return GetAppDataDirectory() / "cache";
}

std::filesystem::path WindowsPlatformAdapter::GetDocumentsDirectory() const {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path))) {
        std::filesystem::path result(path);
        CoTaskMemFree(path);
        return result;
    }
    return std::filesystem::current_path();
}

bool WindowsPlatformAdapter::OpenFolderAndSelectFile(const std::filesystem::path& filePath) const {
    if (!std::filesystem::exists(filePath)) {
        return false;
    }
    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(filePath.wstring().c_str());
    if (pidl) {
        HRESULT hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ILFree(pidl);
        return SUCCEEDED(hr);
    }
    return false;
}

} // namespace pdfinity::platform
#endif
