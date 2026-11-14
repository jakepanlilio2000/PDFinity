// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "IPlatformAdapter.hpp"
#include <memory>

namespace pdfinity::platform {

class PlatformFactory {
public:
    static std::unique_ptr<IPlatformAdapter> CreateAdapter();
};

} // namespace pdfinity::platform
