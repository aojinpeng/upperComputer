  
#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include "../common/Defines.h"
#include "../common/Config.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    static DeviceManager& instance();
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    // 设备管理接口
    void loadDevicesFromConfig();
    QList<DeviceInfoPtr> getAllDevices() const;
    DeviceInfoPtr getDeviceById(int deviceId) const;
    int getDeviceCount() const;

signals:
    void sigDeviceAdded(DeviceInfoPtr device);
    void sigDeviceRemoved(int deviceId);
    void sigDeviceStatusChanged(int deviceId, DeviceStatus status);
public slots:
    void onConfigReloaded();

private:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager() override = default;

    mutable QRecursiveMutex  m_mutex;
    QMap<int, DeviceInfoPtr> m_devices; // deviceId -> DeviceInfoPtr
};

#endif // DEVICEMANAGER_H