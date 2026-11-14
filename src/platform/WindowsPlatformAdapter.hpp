// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "IPlatformAdapter.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace pdfinity::platform {

class WindowsPlatformAdapter : public IPlatformAdapter {
public:
    WindowsPlatformAdapter();
    ~WindowsPlatformAdapter() override = default;

    std::string GetPlatformName() const override;
    std::string GetPlatformVersion() const override;
    std::string GetArchitecture() const override;

    bool HasCapability(PlatformCapability capability) const override;

    SystemTheme GetSystemTheme() const override;
    double GetPrimaryScreenDpi() const override;

    bool IsPortableMode() const override;
    std::filesystem::path GetAppDataDirectory() const override;
    std::filesystem::path GetCacheDirectory() const override;
    std::filesystem::path GetDocumentsDirectory() const override;

    bool OpenFolderAndSelectFile(const std::filesystem::path& filePath) const override;

private:
    uint32_t m_majorVersion{0};
    uint32_t m_minorVersion{0};
    uint32_t m_buildNumber{0};
    bool m_isWindows10OrGreater{false};
    bool m_isPortable{false};
    std::filesystem::path m_exeDir;
};

} // namespace pdfinity::platform
