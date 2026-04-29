 
#include "ModbusTcpMaster.h"
#include "../common/Logger.h"

ModbusTcpMaster::ModbusTcpMaster(DeviceInfoPtr device, QObject *parent)
    : QObject(parent),
    m_device(device),
    m_modbusClient(new QModbusTcpClient(this)),
    m_connectionTimer(new QTimer(this)),
    m_retryCount(0)
{
    // 配置Modbus客户端
    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, m_device->port);
    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, m_device->ip);
    m_modbusClient->setTimeout(3000); // 单请求超时3秒
    m_modbusClient->setNumberOfRetries(2); // 单请求重试2次

    // 连接信号槽
    connect(m_modbusClient, &QModbusDevice::stateChanged, this, &ModbusTcpMaster::onStateChanged);
    connect(m_modbusClient, &QModbusDevice::errorOccurred, this, &ModbusTcpMaster::onErrorOccurred);

    // 配置连接超时定时器
    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout, this, &ModbusTcpMaster::onConnectionTimeout);
}

ModbusTcpMaster::~ModbusTcpMaster()
{
    disconnectFromDevice();
}

void ModbusTcpMaster::connectToDevice()
{
    if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
        return;
    }

    LOG_INFO("ModbusTcpMaster", QString("Connecting to device %1 (%2:%3)...")
                                    .arg(m_device->deviceName, m_device->ip).arg(m_device->port));

    m_device->status = DeviceStatus::Connecting;
    m_retryCount = 0;
    m_connectionTimer->start(CONNECTION_TIMEOUT_MS);

    if (!m_modbusClient->connectDevice()) {
        QString error = m_modbusClient->errorString();
        LOG_ERROR("ModbusTcpMaster", QString("Failed to connect to device %1: %2")
                                         .arg(m_device->deviceName, error));
        emit sigErrorOccurred(error);
        // 重要：不要立即发射 sigDisconnected，因为 QModbusClient 内部状态可能还未变为 UnconnectedState
        // 延迟一小段时间后再检查状态，避免同步循环
        QTimer::singleShot(10, this, [this]() {
            if (m_modbusClient->state() != QModbusDevice::ConnectedState) {
                // 此时状态已经稳定，触发断开信号
                onStateChanged(QModbusDevice::UnconnectedState);
            }
        });
    }
}

void ModbusTcpMaster::onStateChanged(QModbusDevice::State state)
{
    switch (state) {
    case QModbusDevice::ConnectedState:
        m_connectionTimer->stop();
        m_retryCount = 0;
        m_device->status = DeviceStatus::Online;
        LOG_INFO("ModbusTcpMaster", QString("Device %1 connected successfully").arg(m_device->deviceName));
        emit sigConnected();
        break;
    case QModbusDevice::UnconnectedState:
        // 如果当前已经是 Offline 状态且没有 pending 重连，则发射信号
        if (m_device->status != DeviceStatus::Offline) {
            m_device->status = DeviceStatus::Offline;
            LOG_WARNING("ModbusTcpMaster", QString("Device %1 disconnected").arg(m_device->deviceName));
            emit sigDisconnected();
        }
        break;
    default:
        break;
    }
}

void ModbusTcpMaster::disconnectFromDevice()
{
    m_connectionTimer->stop();
    if (m_modbusClient->state() != QModbusDevice::UnconnectedState) {
        m_modbusClient->disconnectDevice();
        LOG_INFO("ModbusTcpMaster", QString("Disconnected from device %1").arg(m_device->deviceName));
    }
    m_device->status = DeviceStatus::Offline;
    emit sigDisconnected();
}

bool ModbusTcpMaster::isConnected() const
{
    return m_modbusClient->state() == QModbusDevice::ConnectedState;
}

void ModbusTcpMaster::readHoldingRegisters(int startAddr, int count)
{
    if (!isConnected()) {
        LOG_WARNING("ModbusTcpMaster", QString("Cannot read registers: device %1 is not connected")
                        .arg(m_device->deviceName));
        return;
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddr, count);
    // readUnit.setUnitId(m_device->slaveId);

    if (auto* reply = m_modbusClient->sendReadRequest(readUnit, m_device->slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    QVector<quint16> data;
                    const QModbusDataUnit unit = reply->result();
                    for (int i = 0; i < unit.valueCount(); ++i) {
                        data.append(unit.value(i));
                    }
                    LOG_DEBUG("ModbusTcpMaster", QString("Read %1 registers from device %2")
                                                     .arg(data.size()).arg(m_device->deviceName));
                    emit sigReadHoldingRegistersSuccess(unit.startAddress(), data);
                } else {
                    QString error = reply->errorString();
                    LOG_ERROR("ModbusTcpMaster", QString("Read error from device %1: %2")
                                                     .arg(m_device->deviceName, error));
                    emit sigErrorOccurred(error);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        QString error = m_modbusClient->errorString();
        LOG_ERROR("ModbusTcpMaster", QString("Failed to send read request to device %1: %2")
                                         .arg(m_device->deviceName, error));
        emit sigErrorOccurred(error);
    }
}

void ModbusTcpMaster::writeSingleRegister(int addr, quint16 value)
{
    if (!isConnected()) {
        LOG_WARNING("ModbusTcpMaster", QString("Cannot write register: device %1 is not connected")
                        .arg(m_device->deviceName));
        return;
    }

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, addr, 1);
    // writeUnit.setUnitId(m_device->slaveId);
    writeUnit.setValue(0, value);

    // if (auto* reply = m_modbusClient->sendWriteRequest(writeUnit, m_device->ip)) {
    if (auto* reply = m_modbusClient->sendWriteRequest(writeUnit, m_device->slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply, addr, value]() {
                if (reply->error() == QModbusDevice::NoError) {
                    LOG_INFO("ModbusTcpMaster", QString("Wrote register %1 = %2 to device %3")
                                 .arg(addr).arg(value).arg(m_device->deviceName));
                    emit sigWriteSingleRegisterSuccess(addr, value);
                } else {
                    QString error = reply->errorString();
                    LOG_ERROR("ModbusTcpMaster", QString("Write error to device %1: %2")
                                                     .arg(m_device->deviceName, error));
                    emit sigErrorOccurred(error);
                }
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    } else {
        QString error = m_modbusClient->errorString();
        LOG_ERROR("ModbusTcpMaster", QString("Failed to send write request to device %1: %2")
                                         .arg(m_device->deviceName, error));
        emit sigErrorOccurred(error);
    }
}


void ModbusTcpMaster::onErrorOccurred(QModbusDevice::Error error)
{
    Q_UNUSED(error);
    // 错误处理已在其他地方完成，这里仅记录日志
}
void ModbusTcpMaster::onReadReady()
{
    // 预留 Modbus 数据就绪处理逻辑
}

void ModbusTcpMaster::onConnectionTimeout()
{
    if (m_retryCount < MAX_RETRY_COUNT) {
        m_retryCount++;
        LOG_WARNING("ModbusTcpMaster", QString("Connection timeout for device %1, retrying (%2/%3)...")
                                           .arg(m_device->deviceName).arg(m_retryCount).arg(MAX_RETRY_COUNT));
        m_modbusClient->disconnectDevice();
        QTimer::singleShot(1000, this, [this]() { connectToDevice(); });
    } else {
        QString error = QString("Connection failed after %1 retries").arg(MAX_RETRY_COUNT);
        LOG_ERROR("ModbusTcpMaster", QString("Device %1: %2").arg(m_device->deviceName, error));
        m_device->status = DeviceStatus::Error;
        emit sigErrorOccurred(error);
    }
}
