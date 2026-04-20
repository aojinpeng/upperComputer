  
#ifndef COLLECTMANAGER_H
#define COLLECTMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMap>
#include "../common/Defines.h"
#include "DeviceManager.h"
#include "../comm/ModbusTcpMaster.h"
#include "../comm/HeartBeat.h"

class CollectManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectManager(QObject *parent = nullptr);
    ~CollectManager() override;

    // 控制接口（必须在moveToThread之后调用）
    void start();
    void stop();

signals:
    void sigDataCollected(DataPointPtr dataPoint);
    void sigDeviceStatusChanged(int deviceId, DeviceStatus status);

public slots:
    // 配置变更槽（用于热加载）
    void onConfigChanged();

private slots:
    void onCollectTimer();
    void onDeviceConnected(int deviceId);
    void onDeviceDisconnected(int deviceId);
    void onDeviceError(int deviceId, const QString& error);
    void onReadSuccess(int deviceId, int startAddr, const QVector<quint16>& data);
    void onHeartbeatSuccess(int deviceId);
    void onHeartbeatFailed(int deviceId, const QString& error);

private:
    void initDevices();
    void cleanupDevices();
    DataPointPtr parseDataPoint(DeviceInfoPtr device, int regIndex, quint16 rawValue);

    QThread* m_collectThread;
    QTimer* m_collectTimer;
    QMap<int, ModbusTcpMaster*> m_modbusMasters; // deviceId -> ModbusTcpMaster
    QMap<int, HeartBeat*> m_heartbeats; // deviceId -> HeartBeat
    bool m_running;
};

#endif // COLLECTMANAGER_H