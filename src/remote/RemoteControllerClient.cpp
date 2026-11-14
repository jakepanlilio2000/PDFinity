// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteControllerClient.hpp"
#include <QDateTime>
#include <QElapsedTimer>
#include <QCoreApplication>

namespace pdfinity::remote {

RemoteControllerClient::RemoteControllerClient(QObject* parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
}

RemoteControllerClient::~RemoteControllerClient() {
    DisconnectFromHost();
}

bool RemoteControllerClient::ConnectToHost(const std::string& host, uint16_t port, int timeoutMs) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return true;
    }

    m_socket->connectToHost(QString::fromStdString(host), port);
    bool connected = m_socket->waitForConnected(timeoutMs);
    if (connected) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return connected;
}

void RemoteControllerClient::DisconnectFromHost() {
    if (m_socket && m_socket->isOpen()) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1000);
        }
    }
}

bool RemoteControllerClient::IsConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void RemoteControllerClient::SetCredentials(const std::string& deviceId, const std::string& token) {
    m_deviceId = deviceId;
    m_token = token;
}

ClientResponse RemoteControllerClient::Pair(const std::string& pin,
                                            const std::string& clientName,
                                            const std::string& deviceId) {
    if (!IsConnected()) {
        return {false, -32000, "Socket is not connected to host", {}};
    }

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = m_nextReqId++;
    req["method"] = "pair";

    QJsonObject params;
    params["pin"] = QString::fromStdString(pin);
    params["clientName"] = QString::fromStdString(clientName);
    if (!deviceId.empty()) {
        params["deviceId"] = QString::fromStdString(deviceId);
    }
    req["params"] = params;

    QByteArray bytes = QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(bytes);
    m_socket->flush();

    // Wait for response line
    QByteArray line;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (m_socket->canReadLine()) {
            line = m_socket->readLine();
            break;
        }
        m_socket->waitForReadyRead(20);
    }

    if (line.isEmpty()) {
        return {false, -32000, "Pairing request timed out waiting for host response", {}};
    }

    QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        return {false, -32700, "Invalid JSON response from host", {}};
    }

    QJsonObject resp = doc.object();
    if (resp.contains("error")) {
        QJsonObject err = resp["error"].toObject();
        return {false, err["code"].toInt(), err["message"].toString().toStdString(), {}};
    }

    QJsonObject res = resp["result"].toObject();
    m_token = res["token"].toString().toStdString();
    m_deviceId = res["deviceId"].toString().toStdString();
    m_nonce = 0;

    return {true, 0, "", res};
}

ClientResponse RemoteControllerClient::SendCommand(const std::string& method, const QJsonObject& params, int timeoutMs) {
    if (!IsConnected()) {
        return {false, -32000, "Socket is not connected to host", {}};
    }

    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["id"] = m_nextReqId++;
    req["method"] = QString::fromStdString(method);
    req["params"] = params;

    // Attach authentication block
    QJsonObject auth;
    auth["deviceId"] = QString::fromStdString(m_deviceId);
    auth["token"] = QString::fromStdString(m_token);
    auth["nonce"] = static_cast<qint64>(++m_nonce);
    auth["timestamp"] = static_cast<qint64>(QDateTime::currentSecsSinceEpoch());
    req["auth"] = auth;

    QByteArray bytes = QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(bytes);
    m_socket->flush();

    // Read response line
    QByteArray line;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (m_socket->canReadLine()) {
            line = m_socket->readLine();
            break;
        }
        m_socket->waitForReadyRead(20);
    }

    if (line.isEmpty()) {
        return {false, -32000, "Command '" + method + "' timed out waiting for host response", {}};
    }

    QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        return {false, -32700, "Invalid JSON response from host", {}};
    }

    QJsonObject resp = doc.object();
    if (resp.contains("error")) {
        QJsonObject err = resp["error"].toObject();
        return {false, err["code"].toInt(), err["message"].toString().toStdString(), {}};
    }

    return {true, 0, "", resp["result"].toObject()};
}

ClientResponse RemoteControllerClient::GetStatus() {
    return SendCommand("status");
}

ClientResponse RemoteControllerClient::GotoPage(int page) {
    return SendCommand("goto_page", QJsonObject{{"page", page}});
}

ClientResponse RemoteControllerClient::NextPage() {
    return SendCommand("next_page");
}

ClientResponse RemoteControllerClient::PrevPage() {
    return SendCommand("prev_page");
}

ClientResponse RemoteControllerClient::FirstPage() {
    return SendCommand("first_page");
}

ClientResponse RemoteControllerClient::LastPage() {
    return SendCommand("last_page");
}

ClientResponse RemoteControllerClient::SetZoom(double level, const std::string& mode) {
    QJsonObject p;
    p["level"] = level;
    if (!mode.empty()) {
        p["mode"] = QString::fromStdString(mode);
    }
    return SendCommand("set_zoom", p);
}

ClientResponse RemoteControllerClient::Search(const std::string& query, bool matchCase) {
    return SendCommand("search", QJsonObject{{"query", QString::fromStdString(query)}, {"matchCase", matchCase}});
}

ClientResponse RemoteControllerClient::GetThumbnail(int page, int width) {
    return SendCommand("thumbnail", QJsonObject{{"page", page}, {"width", width}});
}

ClientResponse RemoteControllerClient::GetJobStatus() {
    return SendCommand("job_status");
}

ClientResponse RemoteControllerClient::PauseJob(const std::string& jobId) {
    return SendCommand("job_pause", QJsonObject{{"jobId", QString::fromStdString(jobId)}});
}

ClientResponse RemoteControllerClient::ResumeJob(const std::string& jobId) {
    return SendCommand("job_resume", QJsonObject{{"jobId", QString::fromStdString(jobId)}});
}

ClientResponse RemoteControllerClient::CancelJob(const std::string& jobId) {
    return SendCommand("job_cancel", QJsonObject{{"jobId", QString::fromStdString(jobId)}});
}

ClientResponse RemoteControllerClient::ToggleFullscreen() {
    return SendCommand("toggle_fullscreen");
}

ClientResponse RemoteControllerClient::CloseDocument() {
    return SendCommand("close_document");
}

ClientResponse RemoteControllerClient::StartBatch(const std::string& profileId, const std::string& inDir, const std::string& outDir) {
    return SendCommand("batch_start", QJsonObject{
        {"profileId", QString::fromStdString(profileId)},
        {"inputDir", QString::fromStdString(inDir)},
        {"outputDir", QString::fromStdString(outDir)}
    });
}

} // namespace pdfinity::remote
