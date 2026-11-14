// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#include "RemoteControllerServer.hpp"
#include "RemoteSecurityPolicy.hpp"
#include "RemotePairingManager.hpp"
#include "RemoteCommandDispatcher.hpp"
#include "application/Logger.hpp"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace pdfinity::remote {

RemoteControllerServer& RemoteControllerServer::Instance() {
    static RemoteControllerServer instance;
    return instance;
}

RemoteControllerServer::RemoteControllerServer()
    : m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &RemoteControllerServer::onNewConnection);
}

RemoteControllerServer::~RemoteControllerServer() {
    Stop();
}

bool RemoteControllerServer::Start(const std::string& address, uint16_t port) {
    Stop();

    auto& sec = RemoteSecurityPolicy::Instance();
    std::string bindAddrStr = address.empty() ? sec.GetBindAddress() : address;
    uint16_t bindPort = port == 0 ? sec.GetServerPort() : port;

    QHostAddress hostAddr;
    if (bindAddrStr == "127.0.0.1" || bindAddrStr == "localhost") {
        hostAddr = QHostAddress::LocalHost;
    } else if (bindAddrStr == "0.0.0.0") {
        hostAddr = QHostAddress::AnyIPv4;
    } else {
        hostAddr.setAddress(QString::fromStdString(bindAddrStr));
        if (hostAddr.isNull()) {
            hostAddr = QHostAddress::LocalHost;
        }
    }

    if (!m_server->listen(hostAddr, bindPort)) {
        LOG_ERROR("RemoteServer", "Failed to bind Remote Controller server to " + bindAddrStr +
                                  ":" + std::to_string(bindPort) + " - Error: " + m_server->errorString().toStdString());
        emit serverStateChanged(false);
        return false;
    }

    LOG_INFO("RemoteServer", "Remote Controller server listening on " + hostAddr.toString().toStdString() +
                             ":" + std::to_string(m_server->serverPort()));
    emit serverStateChanged(true);
    return true;
}

void RemoteControllerServer::Stop() {
    if (!m_server->isListening()) {
        return;
    }

    DisconnectAll();
    m_server->close();
    LOG_INFO("RemoteServer", "Remote Controller server stopped.");
    emit serverStateChanged(false);
}

bool RemoteControllerServer::IsListening() const {
    return m_server && m_server->isListening();
}

std::string RemoteControllerServer::GetBoundAddress() const {
    if (!IsListening()) return "";
    return m_server->serverAddress().toString().toStdString();
}

uint16_t RemoteControllerServer::GetBoundPort() const {
    if (!IsListening()) return 0;
    return m_server->serverPort();
}

void RemoteControllerServer::DisconnectAll() {
    QList<QTcpSocket*> sockets;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        sockets = m_socketBuffers.keys();
        m_socketBuffers.clear();
        m_sessionState.isConnected = false;
        m_sessionState.activeClientName.clear();
        m_sessionState.activeDeviceId.clear();
        m_sessionState.activeIp.clear();
    }

    for (QTcpSocket* sock : sockets) {
        if (sock) {
            sock->disconnect(this);
            if (sock->isOpen()) {
                sock->disconnectFromHost();
            }
            sock->deleteLater();
        }
    }

    emit sessionDisconnected();
}

void RemoteControllerServer::RevokeDevice(const std::string& deviceId) {
    RemotePairingManager::Instance().RevokeDevice(deviceId);

    bool shouldDisconnect = false;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (m_sessionState.activeDeviceId == deviceId) {
            shouldDisconnect = true;
        }
    }
    if (shouldDisconnect) {
        DisconnectAll();
    }
}

void RemoteControllerServer::EmergencyDisable() {
    RemoteSecurityPolicy::Instance().SetEnabled(false);
    Stop();
    RemotePairingManager::Instance().CancelPairing();
    LOG_WARN("RemoteServer", "EMERGENCY DISABLE: Remote Controller service immediately unbound and disabled by operator.");
}

RemoteSessionState RemoteControllerServer::GetSessionState() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_sessionState;
}

void RemoteControllerServer::ResetSessionState() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_sessionState = RemoteSessionState{};
}

void RemoteControllerServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) continue;

        m_socketBuffers.insert(socket, QByteArray{});
        connect(socket, &QTcpSocket::readyRead, this, &RemoteControllerServer::onSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &RemoteControllerServer::onSocketDisconnected);

        LOG_INFO("RemoteServer", "Client connected from " + socket->peerAddress().toString().toStdString() +
                                 ":" + std::to_string(socket->peerPort()));
    }
}

void RemoteControllerServer::onSocketReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray& buffer = m_socketBuffers[socket];
    buffer.append(socket->readAll());

    // Enforce max payload limit against DoS attacks
    if (buffer.size() > static_cast<int>(MAX_REMOTE_FRAME_BYTES)) {
        LOG_WARN("RemoteServer", "Closing socket: Payload exceeded maximum frame limit of 2MB.");
        socket->disconnectFromHost();
        return;
    }

    int newlineIdx;
    while ((newlineIdx = buffer.indexOf('\n')) != -1) {
        QByteArray lineBytes = buffer.left(newlineIdx);
        buffer.remove(0, newlineIdx + 1);

        if (lineBytes.endsWith('\r')) {
            lineBytes.chop(1);
        }

        if (lineBytes.isEmpty()) {
            continue;
        }

        std::string peerIp = socket->peerAddress().toString().toStdString();
        std::string response = ProcessMessage(lineBytes.toStdString(), peerIp, socket);

        if (!response.empty()) {
            socket->write(QByteArray::fromStdString(response + "\n"));
            socket->flush();
        }

        if (lineBytes.contains("\"disconnect\"")) {
            socket->disconnectFromHost();
        }
    }
}

void RemoteControllerServer::onSocketDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    bool allDisconnected = false;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_socketBuffers.remove(socket);
        if (m_socketBuffers.isEmpty()) {
            m_sessionState.isConnected = false;
            m_sessionState.activeClientName.clear();
            m_sessionState.activeDeviceId.clear();
            m_sessionState.activeIp.clear();
            allDisconnected = true;
        }
    }

    socket->deleteLater();
    LOG_INFO("RemoteServer", "Client socket disconnected.");

    if (allDisconnected) {
        emit sessionDisconnected();
    }
}

std::string RemoteControllerServer::ProcessMessage(const std::string& line, const std::string& peerIp, QTcpSocket* /*socket*/) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(line), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject err;
        err["jsonrpc"] = "2.0";
        err["id"] = QJsonValue::Null;
        err["error"] = QJsonObject{{"code", -32700}, {"message", "Parse error: Invalid JSON payload."}};
        return QString::fromUtf8(QJsonDocument(err).toJson(QJsonDocument::Compact)).toStdString();
    }

    QJsonObject req = doc.object();
    QJsonValue id = req.value("id");

    if (req.value("jsonrpc").toString() != "2.0") {
        QJsonObject err;
        err["jsonrpc"] = "2.0";
        err["id"] = id;
        err["error"] = QJsonObject{{"code", -32600}, {"message", "Invalid Request: 'jsonrpc' must be '2.0'"}};
        return QString::fromUtf8(QJsonDocument(err).toJson(QJsonDocument::Compact)).toStdString();
    }

    QString method = req.value("method").toString();
    QJsonObject params = req.value("params").toObject();

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_sessionState.requestsHandled++;
    }

    // 1. Pairing Method (Unauthenticated initial handshake)
    if (method == "pair") {
        std::string pin = params.value("pin").toString().toStdString();
        std::string clientName = params.value("clientName").toString("Remote Client").toStdString();
        std::string deviceId = params.value("deviceId").toString().toStdString();

        auto pairRes = RemotePairingManager::Instance().AttemptPairing(peerIp, pin, clientName, deviceId);
        if (!pairRes.success) {
            RemoteSecurityPolicy::Instance().RecordAudit(
                deviceId, clientName, peerIp, "pair", "DENIED", pairRes.errorMessage
            );
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                m_sessionState.violationsCount++;
            }
            QJsonObject errResp;
            errResp["jsonrpc"] = "2.0";
            errResp["id"] = id;
            errResp["error"] = QJsonObject{{"code", pairRes.errorCode}, {"message", QString::fromStdString(pairRes.errorMessage)}};
            return QString::fromUtf8(QJsonDocument(errResp).toJson(QJsonDocument::Compact)).toStdString();
        }

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_sessionState.isConnected = true;
            m_sessionState.activeClientName = clientName;
            m_sessionState.activeDeviceId = pairRes.deviceInfo.deviceId;
            m_sessionState.activeIp = peerIp;
            m_sessionState.connectedAtIso = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
        }

        RemoteSecurityPolicy::Instance().RecordAudit(
            pairRes.deviceInfo.deviceId, clientName, peerIp, "pair", "ALLOWED", "Pairing completed successfully"
        );

        emit sessionConnected(QString::fromStdString(clientName), QString::fromStdString(peerIp));

        QJsonObject okResp;
        okResp["jsonrpc"] = "2.0";
        okResp["id"] = id;
        QJsonObject resObj;
        resObj["status"] = "paired";
        resObj["token"] = QString::fromStdString(pairRes.token);
        resObj["deviceId"] = QString::fromStdString(pairRes.deviceInfo.deviceId);
        okResp["result"] = resObj;
        return QString::fromUtf8(QJsonDocument(okResp).toJson(QJsonDocument::Compact)).toStdString();
    }

    // 2. All subsequent methods require authentication in 'auth' object
    QJsonObject authObj = req.value("auth").toObject();
    std::string deviceId = authObj.value("deviceId").toString().toStdString();
    std::string token = authObj.value("token").toString().toStdString();
    uint64_t nonce = static_cast<uint64_t>(authObj.value("nonce").toInteger(0));
    int64_t timestamp = authObj.value("timestamp").toInteger(0);

    auto authRes = RemoteSecurityPolicy::Instance().ValidateRequest(deviceId, token, nonce, timestamp, peerIp);
    if (!authRes.allowed) {
        RemoteSecurityPolicy::Instance().RecordAudit(
            deviceId, "unknown", peerIp, method.toStdString(), "BLOCKED", authRes.errorMessage
        );
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_sessionState.violationsCount++;
        }
        QJsonObject errResp;
        errResp["jsonrpc"] = "2.0";
        errResp["id"] = id;
        errResp["error"] = QJsonObject{{"code", authRes.errorCode}, {"message", QString::fromStdString(authRes.errorMessage)}};
        return QString::fromUtf8(QJsonDocument(errResp).toJson(QJsonDocument::Compact)).toStdString();
    }

    // Update active session metadata
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_sessionState.isConnected = true;
        m_sessionState.activeClientName = authRes.deviceInfo.clientName;
        m_sessionState.activeDeviceId = authRes.deviceInfo.deviceId;
        m_sessionState.activeIp = peerIp;
    }

    // 3. Dispatch allowlisted command
    auto execRes = RemoteCommandDispatcher::Instance().Execute(
        method.toStdString(), params, authRes.deviceInfo
    );

    if (!execRes.success) {
        emit commandHandled(method, false);
        QJsonObject errResp;
        errResp["jsonrpc"] = "2.0";
        errResp["id"] = id;
        errResp["error"] = QJsonObject{{"code", execRes.errorCode}, {"message", QString::fromStdString(execRes.errorMessage)}};
        return QString::fromUtf8(QJsonDocument(errResp).toJson(QJsonDocument::Compact)).toStdString();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_sessionState.commandsExecuted++;
    }

    emit commandHandled(method, true);

    QJsonObject okResp;
    okResp["jsonrpc"] = "2.0";
    okResp["id"] = id;
    okResp["result"] = execRes.result;

    return QString::fromUtf8(QJsonDocument(okResp).toJson(QJsonDocument::Compact)).toStdString();
}

} // namespace pdfinity::remote
