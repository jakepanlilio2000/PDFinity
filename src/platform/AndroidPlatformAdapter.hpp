// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "IPlatformAdapter.hpp"
#include <QObject>

namespace pdfinity::platform {

class AndroidPlatformAdapter : public QObject, public IPlatformAdapter {
    Q_OBJECT

public:
    AndroidPlatformAdapter();
    ~AndroidPlatformAdapter() override = default;

    // Platform Identity
    std::string GetPlatformName() const override { return "Android"; }
    std::string GetPlatformVersion() const override;
    std::string GetArchitecture() const override;

    // Capabilities
    bool HasCapability(PlatformCapability capability) const override;

    // Theme and Display
    SystemTheme GetSystemTheme() const override;
    double GetPrimaryScreenDpi() const override;

    // Standard Directories
    bool IsPortableMode() const override { return false; }
    std::filesystem::path GetAppDataDirectory() const override;
    std::filesystem::path GetCacheDirectory() const override;
    std::filesystem::path GetDocumentsDirectory() const override;

    // Shell & Mobile System Operations
    bool OpenFolderAndSelectFile(const std::filesystem::path& filePath) const override;
    bool PrintDocument(const std::filesystem::path& filePath) const override;
    bool ShareDocument(const std::filesystem::path& filePath, const std::string& mimeType = "application/pdf") const override;

private slots:
    void OnApplicationStateChanged(Qt::ApplicationState state);

private:
    void RegisterJniHooks();
};

} // namespace pdfinity::platform
