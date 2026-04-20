#ifndef DEFINES_H
#define DEFINES_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QSharedPointer>
#include <QMutex>

// 设备状态枚举
enum class DeviceStatus {
    Offline = 0,
    Connecting = 1,
    Online = 2,
    Error = 3
};

// 设备信息结构体（对应config.json中的设备配置）
struct DeviceInfo {
    int deviceId;
    QString deviceName;
    QString ip;
    int port;
    int slaveId;
    int startAddr;
    int regCount;
    double alarmMin;
    double alarmMax;
    DeviceStatus status;
    QVector<quint16> rawData; // 原始寄存器数据
    QDateTime lastUpdateTime;
    QMutex mutex; // 每个设备独立的锁，减少锁竞争

    DeviceInfo() : deviceId(0), port(502), slaveId(1), startAddr(0), regCount(0),
        alarmMin(0.0), alarmMax(0.0), status(DeviceStatus::Offline) {}
};

// 采集数据点结构体（用于数据库存储和UI展示）
struct DataPoint {
    int deviceId;
    QString tagName;
    double value;
    QDateTime timestamp;
    bool isAlarm;
    QString alarmMsg;
};

// 智能指针别名（统一管理内存）
using DeviceInfoPtr = QSharedPointer<DeviceInfo>;
using DataPointPtr = QSharedPointer<DataPoint>;

#endif // DEFINES_H