// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "AppConfig.hpp"

namespace pdfinity::app {

std::string AppConfig::ThemeModeToString(AppThemeMode mode) {
    switch (mode) {
        case AppThemeMode::Light:  return "Light";
        case AppThemeMode::Dark:   return "Dark";
        case AppThemeMode::System:
        default:                   return "System";
    }
}

AppThemeMode AppConfig::StringToThemeMode(const std::string& str) {
    if (str == "Light") return AppThemeMode::Light;
    if (str == "Dark")  return AppThemeMode::Dark;
    return AppThemeMode::System;
}

} // namespace pdfinity::app
