  
#ifndef MODBUSTCPMASTER_H
#define MODBUSTCPMASTER_H

#include <QObject>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QTimer>
#include "../common/Defines.h"

class ModbusTcpMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTcpMaster(DeviceInfoPtr device, QObject *parent = nullptr);
    ~ModbusTcpMaster() override;

    // 禁止拷贝
    ModbusTcpMaster(const ModbusTcpMaster&) = delete;
    ModbusTcpMaster& operator=(const ModbusTcpMaster&) = delete;

    // 核心控制接口
    void connectToDevice();
    void disconnectFromDevice();
    bool isConnected() const;

    // Modbus功能码接口（03读保持寄存器，06写单个寄存器）
    void readHoldingRegisters(int startAddr, int count);
    void writeSingleRegister(int addr, quint16 value);

signals:
    // 连接状态信号
    void sigConnected();
    void sigDisconnected();
    void sigErrorOccurred(const QString& error);

    // 数据信号
    void sigReadHoldingRegistersSuccess(int startAddr, const QVector<quint16>& data);
    void sigWriteSingleRegisterSuccess(int addr, quint16 value);

private slots:
    // 内部槽函数
    void onStateChanged(QModbusDevice::State state);
    void onErrorOccurred(QModbusDevice::Error error);
    void onReadReady();
    void onConnectionTimeout();

private:
    DeviceInfoPtr m_device;
    QModbusTcpClient* m_modbusClient;
    QTimer* m_connectionTimer; // 连接超时定时器
    int m_retryCount;
    static const int MAX_RETRY_COUNT = 3;
    static const int CONNECTION_TIMEOUT_MS = 5000;
};

#endif // MODBUSTCPMASTER_H