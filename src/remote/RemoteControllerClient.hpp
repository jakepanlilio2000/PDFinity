// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "RemoteTypes.hpp"
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <memory>
#include <string>

namespace pdfinity::remote {

struct ClientResponse {
    bool success{false};
    int errorCode{0};
    std::string errorMessage;
    QJsonObject result;
};

class RemoteControllerClient : public QObject {
    Q_OBJECT

public:
    explicit RemoteControllerClient(QObject* parent = nullptr);
    ~RemoteControllerClient() override;

    bool ConnectToHost(const std::string& host = "127.0.0.1", uint16_t port = DEFAULT_REMOTE_PORT, int timeoutMs = 3000);
    void DisconnectFromHost();
    bool IsConnected() const;

    // Pairing
    ClientResponse Pair(const std::string& pin,
                        const std::string& clientName = "PDFinity Controller",
                        const std::string& deviceId = "");

    void SetCredentials(const std::string& deviceId, const std::string& token);
    std::string GetDeviceId() const { return m_deviceId; }
    std::string GetToken() const { return m_token; }

    // Core Command Invocation (Synchronous with wait)
    ClientResponse SendCommand(const std::string& method, const QJsonObject& params = {}, int timeoutMs = 5000);

    // High-Level Domain Actions
    ClientResponse GetStatus();
    ClientResponse GotoPage(int page);
    ClientResponse NextPage();
    ClientResponse PrevPage();
    ClientResponse FirstPage();
    ClientResponse LastPage();
    ClientResponse SetZoom(double level, const std::string& mode = "");
    ClientResponse Search(const std::string& query, bool matchCase = false);
    ClientResponse GetThumbnail(int page, int width = 200);
    ClientResponse GetJobStatus();
    ClientResponse PauseJob(const std::string& jobId);
    ClientResponse ResumeJob(const std::string& jobId);
    ClientResponse CancelJob(const std::string& jobId);
    ClientResponse ToggleFullscreen();
    ClientResponse CloseDocument();
    ClientResponse StartBatch(const std::string& profileId, const std::string& inDir, const std::string& outDir);

private:
    QTcpSocket* m_socket{nullptr};
    std::string m_deviceId;
    std::string m_token;
    uint64_t m_nonce{0};
    int m_nextReqId{1};
};

} // namespace pdfinity::remote
