#ifndef COLLECTMANAGER_H
#define COLLECTMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QElapsedTimer>
#include "ModbusTcpMaster.h"
#include "HeartBeat.h"
#include "../logic/DeviceManager.h"

class CollectManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectManager(QObject *parent = nullptr);
    ~CollectManager();

    void start();
    void stop();

signals:
    void sigDeviceStatusChanged(int deviceId, DeviceStatus status);
    void sigDataCollected(DataPointPtr dataPoint);

private slots:
    void onCollectTimer();
    void onDeviceConnected(int deviceId);
    void onDeviceDisconnected(int deviceId);
    void onDeviceError(int deviceId, const QString& error);
    void onReadSuccess(int deviceId, int startAddr, const QVector<quint16>& data);
    void onHeartbeatSuccess(int deviceId);
    void onHeartbeatFailed(int deviceId, const QString& error);
    void onConfigChanged();

private:
    void initDevices();
    void cleanupDevices();
    DataPointPtr parseDataPoint(DeviceInfoPtr device, int regIndex, quint16 rawValue);

    QThread* m_collectThread;
    QTimer* m_collectTimer;
    bool m_running;

    QMap<int, ModbusTcpMaster*> m_modbusMasters;
    QMap<int, HeartBeat*> m_heartbeats;

    // 防抖：记录每个设备上次重连尝试的时间（毫秒时间戳）
    QMap<int, qint64> m_lastReconnectTime;
    QMap<int, int> m_heartbeatFailCount;  // 心跳连续失败次数
};

#endif // COLLECTMANAGER_H