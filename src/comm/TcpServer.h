  
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include "../common/Defines.h"

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer() override;

    bool start(int port);
    void stop();
    void broadcastData(const DataPointPtr& dataPoint);

signals:
    void sigClientConnected(int clientId);
    void sigClientDisconnected(int clientId);
    void sigDataReceived(int clientId, const QByteArray& data);

private slots:
    void onNewConnection();
    void onClientDataReady();
    void onClientDisconnected();

private:
    QTcpServer* m_tcpServer;
    QMap<int, QTcpSocket*> m_clients; // clientId -> socket
    int m_nextClientId;
};

#endif // TCPSERVER_H