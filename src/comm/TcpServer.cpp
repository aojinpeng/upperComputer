 
#include "TcpServer.h"
#include "../common/Logger.h"
#include <QJsonObject>
#include <QJsonDocument>

TcpServer::TcpServer(QObject *parent)
    : QObject(parent),
    m_tcpServer(new QTcpServer(this)),
    m_nextClientId(1)
{
    connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(int port)
{
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        LOG_INFO("TcpServer", QString("TCP Server started on port %1").arg(port));
        return true;
    } else {
        QString error = m_tcpServer->errorString();
        LOG_ERROR("TcpServer", QString("Failed to start TCP Server on port %1: %2").arg(port).arg(error));
        return false;
    }
}

void TcpServer::stop()
{
    // 断开所有客户端
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.value()->disconnectFromHost();
        it.value()->deleteLater();
    }
    m_clients.clear();

    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
        LOG_INFO("TcpServer", "TCP Server stopped");
    }
}

void TcpServer::broadcastData(const DataPointPtr& dataPoint)
{
    if (m_clients.isEmpty()) {
        return;
    }

    // 简单的JSON格式数据广播
    QJsonObject json;
    json["device_id"] = dataPoint->deviceId;
    json["tag_name"] = dataPoint->tagName;
    json["value"] = dataPoint->value;
    json["timestamp"] = dataPoint->timestamp.toString(Qt::ISODate);
    json["is_alarm"] = dataPoint->isAlarm;
    json["alarm_msg"] = dataPoint->alarmMsg;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.value()->write(data);
        it.value()->flush();
    }
}

void TcpServer::onNewConnection()
{
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
    int clientId = m_nextClientId++;
    m_clients[clientId] = socket;

    connect(socket, &QTcpSocket::readyRead, this, [this, clientId]() { onClientDataReady(); });
    connect(socket, &QTcpSocket::disconnected, this, [this, clientId]() { onClientDisconnected(); });

    LOG_INFO("TcpServer", QString("Client %1 connected").arg(clientId));
    emit sigClientConnected(clientId);
}

void TcpServer::onClientDataReady()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    int clientId = m_clients.key(socket, -1);
    if (clientId == -1) return;

    QByteArray data = socket->readAll();
    LOG_DEBUG("TcpServer", QString("Received data from client %1: %2").arg(clientId).arg(QString(data)));
    emit sigDataReceived(clientId, data);
}

void TcpServer::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    int clientId = m_clients.key(socket, -1);
    if (clientId != -1) {
        m_clients.remove(clientId);
        LOG_INFO("TcpServer", QString("Client %1 disconnected").arg(clientId));
        emit sigClientDisconnected(clientId);
    }

    socket->deleteLater();
}