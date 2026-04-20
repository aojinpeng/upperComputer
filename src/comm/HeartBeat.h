  
#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <QObject>
#include <QTimer>
#include "../common/Defines.h"
#include "ModbusTcpMaster.h"

class HeartBeat : public QObject
{
    Q_OBJECT
public:
    explicit HeartBeat(DeviceInfoPtr device, ModbusTcpMaster* master, QObject *parent = nullptr);
    ~HeartBeat() override;

    void start(int intervalMs = 5000);
    void stop();

signals:
    void sigHeartbeatSuccess();
    void sigHeartbeatFailed(const QString& error);

private slots:
    void onHeartbeatTimer();
    void onReadSuccess(int startAddr, const QVector<quint16>& data);
    void onReadError(const QString& error);

private:
    DeviceInfoPtr m_device;
    ModbusTcpMaster* m_master;
    QTimer* m_heartbeatTimer;
    int m_intervalMs;
};

#endif // HEARTBEAT_H