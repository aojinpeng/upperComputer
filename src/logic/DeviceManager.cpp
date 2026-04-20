 
#include "DeviceManager.h"
#include "../common/Logger.h"

DeviceManager& DeviceManager::instance()
{
    static DeviceManager instance;
    return instance;
}

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
}

void DeviceManager::loadDevicesFromConfig()
{
    QMutexLocker locker(&m_mutex);
    m_devices.clear();

    QJsonArray deviceArray = Config::instance().deviceList();
    for (const QJsonValue& value : deviceArray) {
        QJsonObject obj = value.toObject();
        DeviceInfoPtr device(new DeviceInfo());
        device->deviceId = obj["device_id"].toInt();
        device->deviceName = obj["device_name"].toString();
        device->ip = obj["ip"].toString();
        device->port = obj["port"].toInt();
        device->slaveId = obj["slave_id"].toInt();
        device->startAddr = obj["start_addr"].toInt();
        device->regCount = obj["reg_count"].toInt();
        device->alarmMin = obj["alarm_min"].toDouble();
        device->alarmMax = obj["alarm_max"].toDouble();
        device->status = DeviceStatus::Offline;

        m_devices[device->deviceId] = device;
        LOG_INFO("DeviceManager", QString("Loaded device: %1 (ID: %2)").arg(device->deviceName).arg(device->deviceId));
        emit sigDeviceAdded(device);
    }

    LOG_INFO("DeviceManager", QString("Loaded %1 devices from config").arg(m_devices.size()));
}

QList<DeviceInfoPtr> DeviceManager::getAllDevices() const
{
    QMutexLocker locker(&m_mutex);
    return m_devices.values();
}

DeviceInfoPtr DeviceManager::getDeviceById(int deviceId) const
{
    QMutexLocker locker(&m_mutex);
    return m_devices.value(deviceId, nullptr);
}

int DeviceManager::getDeviceCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_devices.size();
}