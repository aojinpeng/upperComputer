 
#include "CollectManager.h"
#include "../common/Logger.h"
#include "../common/Config.h"
#include "../common/Utils.h"

CollectManager::CollectManager(QObject *parent)
    : QObject(parent),
    m_collectThread(new QThread(this)),
    m_collectTimer(new QTimer(this)),
    m_running(false)
{
    // 将自己移动到采集线程
    this->moveToThread(m_collectThread);

    // 连接线程启动信号
    connect(m_collectThread, &QThread::started, this, [this]() {
        LOG_INFO("CollectManager", "Collect thread started");
        initDevices();
        m_collectTimer->start(Config::instance().collectIntervalMs());
        m_running = true;
    });

    connect(m_collectThread, &QThread::finished, this, [this]() {
        LOG_INFO("CollectManager", "Collect thread finished");
    });

    // 连接采集定时器
    connect(m_collectTimer, &QTimer::timeout, this, &CollectManager::onCollectTimer);

    // 连接配置变更信号
    connect(&Config::instance(), &Config::configChanged, this, &CollectManager::onConfigChanged);
}

CollectManager::~CollectManager()
{
    stop();
}

void CollectManager::start()
{
    if (!m_collectThread->isRunning()) {
        m_collectThread->start();
    }
}

void CollectManager::stop()
{
    if (m_running) {
        m_running = false;
        m_collectTimer->stop();
        cleanupDevices();
        m_collectThread->quit();
        m_collectThread->wait();
    }
}

void CollectManager::initDevices()
{
    QList<DeviceInfoPtr> devices = DeviceManager::instance().getAllDevices();
    for (DeviceInfoPtr device : devices) {
        // 创建Modbus主站
        ModbusTcpMaster* master = new ModbusTcpMaster(device, this);
        m_modbusMasters[device->deviceId] = master;

        // 连接Modbus信号
        connect(master, &ModbusTcpMaster::sigConnected, this, [this, deviceId = device->deviceId]() {
            onDeviceConnected(deviceId);
        });
        connect(master, &ModbusTcpMaster::sigDisconnected, this, [this, deviceId = device->deviceId]() {
            onDeviceDisconnected(deviceId);
        });
        connect(master, &ModbusTcpMaster::sigErrorOccurred, this, [this, deviceId = device->deviceId](const QString& error) {
            onDeviceError(deviceId, error);
        });
        connect(master, &ModbusTcpMaster::sigReadHoldingRegistersSuccess, this, [this, deviceId = device->deviceId](int startAddr, const QVector<quint16>& data) {
            onReadSuccess(deviceId, startAddr, data);
        });

        // 创建心跳
        HeartBeat* heartbeat = new HeartBeat(device, master, this);
        m_heartbeats[device->deviceId] = heartbeat;

        // 连接心跳信号
        connect(heartbeat, &HeartBeat::sigHeartbeatSuccess, this, [this, deviceId = device->deviceId]() {
            onHeartbeatSuccess(deviceId);
        });
        connect(heartbeat, &HeartBeat::sigHeartbeatFailed, this, [this, deviceId = device->deviceId](const QString& error) {
            onHeartbeatFailed(deviceId, error);
        });

        // 启动连接和心跳
        master->connectToDevice();
        heartbeat->start(Config::instance().heartbeatIntervalMs());
    }
}

void CollectManager::cleanupDevices()
{
    // 停止所有心跳
    for (auto it = m_heartbeats.begin(); it != m_heartbeats.end(); ++it) {
        it.value()->stop();
        it.value()->deleteLater();
    }
    m_heartbeats.clear();

    // 断开所有Modbus连接
    for (auto it = m_modbusMasters.begin(); it != m_modbusMasters.end(); ++it) {
        it.value()->disconnectFromDevice();
        it.value()->deleteLater();
    }
    m_modbusMasters.clear();
}

void CollectManager::onCollectTimer()
{
    if (!m_running) return;

    // 轮询所有在线设备
    QList<DeviceInfoPtr> devices = DeviceManager::instance().getAllDevices();
    for (DeviceInfoPtr device : devices) {
        if (device->status == DeviceStatus::Online) {
            ModbusTcpMaster* master = m_modbusMasters.value(device->deviceId, nullptr);
            if (master) {
                master->readHoldingRegisters(device->startAddr, device->regCount);
            }
        }
    }
}

void CollectManager::onDeviceConnected(int deviceId)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (device) {
        LOG_INFO("CollectManager", QString("Device %1 connected").arg(device->deviceName));
        emit sigDeviceStatusChanged(deviceId, DeviceStatus::Online);
    }
}

void CollectManager::onDeviceDisconnected(int deviceId)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (device) {
        LOG_WARNING("CollectManager", QString("Device %1 disconnected, trying to reconnect...").arg(device->deviceName));
        emit sigDeviceStatusChanged(deviceId, DeviceStatus::Offline);

        // 自动重连
        ModbusTcpMaster* master = m_modbusMasters.value(deviceId, nullptr);
        if (master) {
            QTimer::singleShot(Config::instance().reconnectIntervalMs(), this, [master]() {
                master->connectToDevice();
            });
        }
    }
}

void CollectManager::onDeviceError(int deviceId, const QString& error)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (device) {
        LOG_ERROR("CollectManager", QString("Device %1 error: %2").arg(device->deviceName, error));
        emit sigDeviceStatusChanged(deviceId, DeviceStatus::Error);
    }
}

void CollectManager::onReadSuccess(int deviceId, int startAddr, const QVector<quint16>& data)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (!device) return;

    // 更新设备原始数据（加锁保护）
    {
        QMutexLocker locker(&device->mutex);
        device->rawData = data;
        device->lastUpdateTime = QDateTime::currentDateTime();
    }

    // 解析数据点并发送信号
    for (int i = 0; i < data.size(); ++i) {
        DataPointPtr dataPoint = parseDataPoint(device, startAddr + i, data[i]);
        if (dataPoint) {
            emit sigDataCollected(dataPoint);
        }
    }
}

void CollectManager::onHeartbeatSuccess(int deviceId)
{
    Q_UNUSED(deviceId);
    // 心跳成功，设备保持在线状态
}

void CollectManager::onHeartbeatFailed(int deviceId, const QString& error)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (device) {
        LOG_WARNING("CollectManager", QString("Heartbeat failed for device %1: %2").arg(device->deviceName, error));
        // 心跳失败，触发重连
        ModbusTcpMaster* master = m_modbusMasters.value(deviceId, nullptr);
        if (master) {
            master->disconnectFromDevice();
        }
    }
}

void CollectManager::onConfigChanged()
{
    LOG_INFO("CollectManager", "Config changed, reloading devices...");
    // 停止当前采集
    m_collectTimer->stop();
    cleanupDevices();

    // 重新加载设备
    DeviceManager::instance().loadDevicesFromConfig();

    // 重新初始化并启动
    initDevices();
    m_collectTimer->start(Config::instance().collectIntervalMs());
}

DataPointPtr CollectManager::parseDataPoint(DeviceInfoPtr device, int regIndex, quint16 rawValue)
{
    DataPointPtr dataPoint(new DataPoint());
    dataPoint->deviceId = device->deviceId;
    dataPoint->tagName = QString("%1.Reg%2").arg(device->deviceName).arg(regIndex);
    dataPoint->timestamp = QDateTime::currentDateTime();

    // 简单的物理量转换（示例：原始值 * 0.1）
    // 实际项目中应根据设备配置文件中的缩放系数进行转换
    double scaledValue = static_cast<double>(rawValue) * 0.1;

    // 异常值滤波
    dataPoint->value = Utils::filterOutlier(scaledValue, device->alarmMin - 100, device->alarmMax + 100, 0.0);

    // 报警判断
    if (dataPoint->value < device->alarmMin) {
        dataPoint->isAlarm = true;
        dataPoint->alarmMsg = QString("Value below minimum: %1 < %2").arg(dataPoint->value).arg(device->alarmMin);
    } else if (dataPoint->value > device->alarmMax) {
        dataPoint->isAlarm = true;
        dataPoint->alarmMsg = QString("Value above maximum: %1 > %2").arg(dataPoint->value).arg(device->alarmMax);
    } else {
        dataPoint->isAlarm = false;
        dataPoint->alarmMsg = "";
    }

    return dataPoint;
}