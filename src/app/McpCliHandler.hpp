// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <vector>
#include <string>

namespace pdfinity::app {

class McpCliHandler {
public:
    static bool IsMcpCommand(int argc, char* argv[]);
    static int Execute(int argc, char* argv[]);
    static void PrintMcpHelp();
};

} // namespace pdfinity::app
