// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteCliHandler.hpp"
#include "remote/RemoteControllerClient.hpp"
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <algorithm>

namespace pdfinity::app {

bool RemoteCliHandler::IsRemoteCommand(int argc, char* argv[]) {
    if (argc < 2) return false;
    std::string cmd = argv[1];
    return (cmd == "remote-client" || cmd == "--remote-client");
}

void RemoteCliHandler::PrintRemoteHelp() {
    std::cout << "PDFinity - Secure Remote Controller Client\n\n"
              << "Usage: PDFinity remote-client [options] --command <cmd>\n\n"
              << "Options:\n"
              << "  --host <ip>          Target host address (default: 127.0.0.1)\n"
              << "  --port <port>        Target port (default: 8443)\n"
              << "  --pin <PIN>          6-digit pairing PIN (if initiating pairing)\n"
              << "  --device-id <id>     Paired device ID\n"
              << "  --token <token>      Saved bearer session token\n"
              << "  --client-name <name> Friendly client identifier (default: 'PDFinity Remote CLI')\n"
              << "  --command <cmd>      Command to execute (e.g. status, goto_page, next_page, prev_page, search)\n"
              << "  --param <key=val>    Command argument (can be specified multiple times)\n"
              << "  --json               Output raw JSON response to stdout\n"
              << "  -h, --help           Show this help message and exit\n\n"
              << "Supported Commands:\n"
              << "  status, goto_page, next_page, prev_page, first_page, last_page, set_zoom,\n"
              << "  search, thumbnail, job_status, job_pause, job_resume, job_cancel,\n"
              << "  toggle_fullscreen, close_document, batch_start, disconnect\n";
}

int RemoteCliHandler::Execute(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    uint16_t port = remote::DEFAULT_REMOTE_PORT;
    std::string pin;
    std::string deviceId;
    std::string token;
    std::string clientName = "PDFinity Remote CLI";
    std::string command;
    QJsonObject params;
    bool jsonOutput = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintRemoteHelp();
            return 0;
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--pin" && i + 1 < argc) {
            pin = argv[++i];
        } else if (arg == "--device-id" && i + 1 < argc) {
            deviceId = argv[++i];
        } else if (arg == "--token" && i + 1 < argc) {
            token = argv[++i];
        } else if (arg == "--client-name" && i + 1 < argc) {
            clientName = argv[++i];
        } else if (arg == "--command" && i + 1 < argc) {
            command = argv[++i];
        } else if (arg == "--json") {
            jsonOutput = true;
        } else if (arg == "--param" && i + 1 < argc) {
            std::string pStr = argv[++i];
            size_t eqPos = pStr.find('=');
            if (eqPos != std::string::npos) {
                std::string k = pStr.substr(0, eqPos);
                std::string v = pStr.substr(eqPos + 1);
                // Try integer parse, otherwise string
                try {
                    int intVal = std::stoi(v);
                    params[QString::fromStdString(k)] = intVal;
                } catch (...) {
                    params[QString::fromStdString(k)] = QString::fromStdString(v);
                }
            }
        }
    }

    if (pin.empty() && token.empty()) {
        std::cerr << "Error: Either --pin <PIN> or --token <token> (with --device-id) is required to authenticate.\n";
        return 1;
    }

    // Ensure Qt application instance exists for socket event dispatching
    std::unique_ptr<QCoreApplication> localApp;
    if (!QCoreApplication::instance()) {
        static int dummyArgc = 1;
        static char appName[] = "PDFinity";
        static char* dummyArgv[] = { appName, nullptr };
        localApp = std::make_unique<QCoreApplication>(dummyArgc, dummyArgv);
    }

    remote::RemoteControllerClient client;
    if (!client.ConnectToHost(host, port, 3000)) {
        std::cerr << "Error: Could not connect to host at " << host << ":" << port << "\n";
        return 1;
    }

    // Pair if PIN provided
    if (!pin.empty()) {
        auto pairRes = client.Pair(pin, clientName, deviceId);
        if (!pairRes.success) {
            std::cerr << "Pairing Failed: [" << pairRes.errorCode << "] " << pairRes.errorMessage << "\n";
            return 1;
        }
        if (command.empty()) {
            if (jsonOutput) {
                std::cout << QString::fromUtf8(QJsonDocument(pairRes.result).toJson(QJsonDocument::Compact)).toStdString() << "\n";
            } else {
                std::cout << "Successfully Paired!\nDevice ID: " << client.GetDeviceId() << "\nToken: " << client.GetToken() << "\n";
            }
            return 0;
        }
    } else {
        client.SetCredentials(deviceId, token);
    }

    if (command.empty()) {
        command = "status";
    }

    auto cmdRes = client.SendCommand(command, params, 5000);
    if (!cmdRes.success) {
        std::cerr << "Command Failed: [" << cmdRes.errorCode << "] " << cmdRes.errorMessage << "\n";
        return 1;
    }

    if (jsonOutput) {
        std::cout << QString::fromUtf8(QJsonDocument(cmdRes.result).toJson(QJsonDocument::Compact)).toStdString() << "\n";
    } else {
        std::cout << "Success: " << QString::fromUtf8(QJsonDocument(cmdRes.result).toJson(QJsonDocument::Indented)).toStdString() << "\n";
    }

    return 0;
}

} // namespace pdfinity::app
