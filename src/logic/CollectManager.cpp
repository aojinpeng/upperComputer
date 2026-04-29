#include "CollectManager.h"
#include "../common/Logger.h"
#include "../common/Config.h"
#include "../common/Utils.h"
#include <QMutexLocker>
#include <QDateTime>
#include <QTimer>

CollectManager::CollectManager(QObject *parent)
    : QObject(nullptr),  // 🔥 核心修复1：自己必须无父，才能移动线程
    m_collectThread(new QThread),  // 🔥 无父对象
    m_collectTimer(nullptr),       // 🔥 核心修复2：先不创建定时器！
    m_running(false)
{
    // 🔥 核心修复3：把自己整个移到子线程（这是唯一正确用法）
    this->moveToThread(m_collectThread);

    // 🔥 核心修复4：定时器【在子线程内部创建】，绝对不跨线程
    connect(m_collectThread, &QThread::started, this, [this]() {
        LOG_INFO("CollectManager", "Collect thread started");

        // ✅ 在子线程里创建定时器！！！（这才是根本解决）
        m_collectTimer = new QTimer;
        m_collectTimer->setInterval(Config::instance().collectIntervalMs());
        connect(m_collectTimer, &QTimer::timeout, this, &CollectManager::onCollectTimer);

        initDevices();
        m_collectTimer->start();  // ✅ 子线程创建 + 子线程启动 = 完美合法
        m_running = true;
    });

    connect(m_collectThread, &QThread::finished, this, [this]() {
        m_collectTimer->stop();
        m_collectTimer->deleteLater();
        m_collectThread->deleteLater();
        LOG_INFO("CollectManager", "Collect thread finished");
    });

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

// ====================== 以下代码完全不用改 ======================
void CollectManager::initDevices()
{
    QList<DeviceInfoPtr> devices = DeviceManager::instance().getAllDevices();
    for (DeviceInfoPtr device : devices) {
        ModbusTcpMaster* master = new ModbusTcpMaster(device, this);
        m_modbusMasters[device->deviceId] = master;

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

        HeartBeat* heartbeat = new HeartBeat(device, master, this);
        m_heartbeats[device->deviceId] = heartbeat;

        connect(heartbeat, &HeartBeat::sigHeartbeatSuccess, this, [this, deviceId = device->deviceId]() {
            onHeartbeatSuccess(deviceId);
        });
        connect(heartbeat, &HeartBeat::sigHeartbeatFailed, this, [this, deviceId = device->deviceId](const QString& error) {
            onHeartbeatFailed(deviceId, error);
        });

        master->connectToDevice();
    }
}

void CollectManager::cleanupDevices()
{
    m_heartbeatFailCount.clear();

    for (auto it = m_heartbeats.begin(); it != m_heartbeats.end(); ++it) {
        it.value()->stop();
        it.value()->disconnect(this);
        it.value()->deleteLater();
    }
    m_heartbeats.clear();

    for (auto it = m_modbusMasters.begin(); it != m_modbusMasters.end(); ++it) {
        it.value()->disconnectFromDevice();
        it.value()->disconnect(this);
        it.value()->deleteLater();
    }
    m_modbusMasters.clear();
}

void CollectManager::onCollectTimer()
{
    if (!m_running) return;

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

        m_heartbeatFailCount[deviceId] = 0;

        HeartBeat* hb = m_heartbeats.value(deviceId, nullptr);
        if (hb) {
            hb->stop();
            QTimer::singleShot(1000, this, [this, deviceId, hb]() {
                if (m_heartbeats.contains(deviceId) && m_heartbeats.value(deviceId) == hb) {
                    hb->start(Config::instance().heartbeatIntervalMs());
                }
            });
        }
    }
}

void CollectManager::onDeviceDisconnected(int deviceId)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (!device) return;

    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastReconnectTime.contains(deviceId)) {
        qint64 elapsed = nowMs - m_lastReconnectTime[deviceId];
        if (elapsed < 5000) {
            LOG_DEBUG("CollectManager", QString("Device %1 reconnect throttled, skip (elapsed %2 ms)")
                          .arg(device->deviceName).arg(elapsed));
            return;
        }
    }
    m_lastReconnectTime[deviceId] = nowMs;

    LOG_WARNING("CollectManager", QString("Device %1 disconnected, will reconnect after %2 ms")
                                      .arg(device->deviceName).arg(Config::instance().reconnectIntervalMs()));
    emit sigDeviceStatusChanged(deviceId, DeviceStatus::Offline);

    int delay = Config::instance().reconnectIntervalMs();
    if (delay < 1000) delay = 1000;

    QTimer::singleShot(delay, this, [this, deviceId]() {
        ModbusTcpMaster* currentMaster = m_modbusMasters.value(deviceId, nullptr);
        if (!currentMaster) {
            return;
        }
        if (currentMaster->isConnected()) {
            LOG_DEBUG("CollectManager", QString("Device %1 already connected, skip reconnect").arg(deviceId));
            return;
        }
        currentMaster->connectToDevice();
    });
}

void CollectManager::onHeartbeatFailed(int deviceId, const QString& error)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (!device) return;

    int failCount = m_heartbeatFailCount.value(deviceId, 0) + 1;
    m_heartbeatFailCount[deviceId] = failCount;

    LOG_WARNING("CollectManager", QString("Heartbeat failed for device %1 (count %2/3): %3")
                                      .arg(device->deviceName).arg(failCount).arg(error));

    if (failCount >= 3) {
        LOG_ERROR("CollectManager", QString("Device %1 heartbeat failed consecutively %2 times, disconnecting.")
                      .arg(device->deviceName).arg(failCount));
        ModbusTcpMaster* master = m_modbusMasters.value(deviceId, nullptr);
        if (master) {
            master->disconnectFromDevice();
        }
        m_heartbeatFailCount[deviceId] = 0;
    }
}

void CollectManager::onConfigChanged()
{
    LOG_INFO("CollectManager", "Config changed, reloading devices...");
    m_collectTimer->stop();
    cleanupDevices();

    m_lastReconnectTime.clear();
    m_heartbeatFailCount.clear();

    DeviceManager::instance().loadDevicesFromConfig();
    initDevices();
    m_collectTimer->start(Config::instance().collectIntervalMs());
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

    {
        QMutexLocker locker(&device->mutex);
        device->rawData = data;
        device->lastUpdateTime = QDateTime::currentDateTime();
    }

    for (int i = 0; i < data.size(); ++i) {
        DataPointPtr dataPoint = parseDataPoint(device, startAddr + i, data[i]);
        if (dataPoint) {
            emit sigDataCollected(dataPoint);
        }
    }
}

void CollectManager::onHeartbeatSuccess(int deviceId)
{
    m_heartbeatFailCount[deviceId] = 0;
}

DataPointPtr CollectManager::parseDataPoint(DeviceInfoPtr device, int regIndex, quint16 rawValue)
{
    DataPointPtr dataPoint(new DataPoint());
    dataPoint->deviceId = device->deviceId;
    dataPoint->tagName = QString("%1.Reg%2").arg(device->deviceName).arg(regIndex);
    dataPoint->timestamp = QDateTime::currentDateTime();

    double scaledValue = static_cast<double>(rawValue) * 0.1;
    dataPoint->value = Utils::filterOutlier(scaledValue, device->alarmMin - 100, device->alarmMax + 100, 0.0);

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