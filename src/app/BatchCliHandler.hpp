// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include <vector>
#include <string>

namespace pdfinity::app {

class BatchCliHandler {
public:
    static bool IsBatchCommand(int argc, char* argv[]);
    static int Execute(int argc, char* argv[]);

private:
    static int HandleBatchProcess(const std::vector<std::string>& args);
    static int HandleBatchProfiles(const std::vector<std::string>& args);
    static int HandleBatchHistory(const std::vector<std::string>& args);
    static void PrintBatchHelp();
};

} // namespace pdfinity::app
