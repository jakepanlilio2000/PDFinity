// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>
#include <filesystem>
#include <cstdint>

namespace pdfinity::platform {

enum class PlatformCapability {
    PerMonitorDpiV2,
    HardwareAcceleratedRendering,
    DirectWriteFonts,
    SystemPrintSupport,
    ModernCryptoCng,
    LocalMcpPipes,
    Tls13Support
};

enum class SystemTheme {
    Light,
    Dark,
    Unknown
};

class IPlatformAdapter {
public:
    virtual ~IPlatformAdapter() = default;

    // Platform Identity
    virtual std::string GetPlatformName() const = 0;
    virtual std::string GetPlatformVersion() const = 0;
    virtual std::string GetArchitecture() const = 0;

    // Capabilities
    virtual bool HasCapability(PlatformCapability capability) const = 0;

    // Theme and Display
    virtual SystemTheme GetSystemTheme() const = 0;
    virtual double GetPrimaryScreenDpi() const = 0;

    // Standard Directories
    virtual bool IsPortableMode() const = 0;
    virtual std::filesystem::path GetAppDataDirectory() const = 0;
    virtual std::filesystem::path GetCacheDirectory() const = 0;
    virtual std::filesystem::path GetDocumentsDirectory() const = 0;

    // Shell Operations
    virtual bool OpenFolderAndSelectFile(const std::filesystem::path& filePath) const = 0;
};

} // namespace pdfinity::platform
