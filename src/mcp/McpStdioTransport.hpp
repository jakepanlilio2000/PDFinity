// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <atomic>
#include <iostream>

namespace pdfinity::mcp {

class McpStdioTransport {
public:
    static int RunStdioServer();
    static void RequestStop();

private:
    static inline std::atomic<bool> s_stopRequested{false};
};

} // namespace pdfinity::mcp
