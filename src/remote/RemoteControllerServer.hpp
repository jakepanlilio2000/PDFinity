// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 StackSync Solutions. Lead Programmer: Jake Ashley Panlilio.

#pragma once

#include "RemoteTypes.hpp"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <mutex>
#include <memory>

namespace pdfinity::remote {

class RemoteControllerServer : public QObject {
    Q_OBJECT

public:
    static RemoteControllerServer& Instance();

    bool Start(const std::string& address = "", uint16_t port = 0);
    void Stop();
    bool IsListening() const;
    std::string GetBoundAddress() const;
    uint16_t GetBoundPort() const;

    void DisconnectAll();
    void RevokeDevice(const std::string& deviceId);
    void EmergencyDisable();

    RemoteSessionState GetSessionState() const;
    void ResetSessionState();

signals:
    void sessionConnected(const QString& clientName, const QString& ip);
    void sessionDisconnected();
    void commandHandled(const QString& command, bool success);
    void serverStateChanged(bool isListening);

private slots:
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();

private:
    RemoteControllerServer();
    ~RemoteControllerServer() override;

    std::string ProcessMessage(const std::string& line, const std::string& peerIp, QTcpSocket* socket);

    QTcpServer* m_server{nullptr};
    mutable std::recursive_mutex m_mutex;
    RemoteSessionState m_sessionState;

    QMap<QTcpSocket*, QByteArray> m_socketBuffers;
};

} // namespace pdfinity::remote
