// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "application/AppConfig.hpp"
#include <QString>

namespace pdfinity::ui {

class ThemeManager {
public:
    static void ApplyTheme(app::AppThemeMode mode);
    static QString GetStyleSheet(bool isDark);
    static bool IsEffectiveDark(app::AppThemeMode mode);
};

} // namespace pdfinity::ui
