// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "RemoteTypes.hpp"
#include <QJsonObject>
#include <string>
#include <functional>

namespace pdfinity::remote {

struct CommandExecutionResult {
    bool success{false};
    int errorCode{0};
    std::string errorMessage;
    QJsonObject result;

    static CommandExecutionResult Success(const QJsonObject& res) {
        return {true, 0, "", res};
    }

    static CommandExecutionResult Error(int code, const std::string& msg) {
        return {false, code, msg, {}};
    }
};

class RemoteCommandDispatcher {
public:
    static RemoteCommandDispatcher& Instance();

    // Hook to wire into MainWindow or DocumentWorkspace
    struct ExecutionHooks {
        std::function<QJsonObject()> getStatus;
        std::function<bool(int)> gotoPage;
        std::function<bool()> nextPage;
        std::function<bool()> prevPage;
        std::function<bool()> firstPage;
        std::function<bool()> lastPage;
        std::function<bool(double, const std::string&)> setZoom;
        std::function<QJsonObject(const std::string&, bool)> search;
        std::function<std::string(int, int)> getThumbnailBase64;
        std::function<bool()> toggleFullscreen;
        std::function<bool()> closeDocument;
    };

    void SetExecutionHooks(const ExecutionHooks& hooks);

    // Executes allowlisted command
    CommandExecutionResult Execute(const std::string& commandName,
                                   const QJsonObject& params,
                                   const RemoteDeviceInfo& clientDevice);

private:
    RemoteCommandDispatcher() = default;

    ExecutionHooks m_hooks;
};

} // namespace pdfinity::remote
