// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <string>

namespace pdfinity::app {

class RemoteCliHandler {
public:
    static bool IsRemoteCommand(int argc, char* argv[]);
    static int Execute(int argc, char* argv[]);
    static void PrintRemoteHelp();
};

} // namespace pdfinity::app
