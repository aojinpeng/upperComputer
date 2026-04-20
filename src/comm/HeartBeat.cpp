 
#include "HeartBeat.h"
#include "../common/Logger.h"

HeartBeat::HeartBeat(DeviceInfoPtr device, ModbusTcpMaster* master, QObject *parent)
    : QObject(parent),
    m_device(device),
    m_master(master),
    m_heartbeatTimer(new QTimer(this)),
    m_intervalMs(5000)
{
    connect(m_heartbeatTimer, &QTimer::timeout, this, &HeartBeat::onHeartbeatTimer);
}

HeartBeat::~HeartBeat()
{
    stop();
}

void HeartBeat::start(int intervalMs)
{
    m_intervalMs = intervalMs;
    if (!m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start(m_intervalMs);
        LOG_INFO("HeartBeat", QString("Heartbeat started for device %1 (interval: %2ms)")
                                  .arg(m_device->deviceName).arg(m_intervalMs));
    }
}

void HeartBeat::stop()
{
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
        LOG_INFO("HeartBeat", QString("Heartbeat stopped for device %1").arg(m_device->deviceName));
    }
}

void HeartBeat::onHeartbeatTimer()
{
    if (!m_master->isConnected()) {
        emit sigHeartbeatFailed("Device not connected");
        return;
    }
    // 心跳包：读1个保持寄存器（通常是设备状态寄存器）
    m_master->readHoldingRegisters(m_device->startAddr, 1);
}

void HeartBeat::onReadSuccess(int startAddr, const QVector<quint16>& data)
{
    Q_UNUSED(startAddr);
    Q_UNUSED(data);
    m_device->lastUpdateTime = QDateTime::currentDateTime();
    emit sigHeartbeatSuccess();
}

void HeartBeat::onReadError(const QString& error)
{
    LOG_WARNING("HeartBeat", QString("Heartbeat failed for device %1: %2")
                    .arg(m_device->deviceName, error));
    emit sigHeartbeatFailed(error);
}