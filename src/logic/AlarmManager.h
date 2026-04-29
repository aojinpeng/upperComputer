#ifndef ALARMMANAGER_H
#define ALARMMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include "../common/Defines.h"

// 报警状态枚举
enum class AlarmState {
    Normal = 0,
    Pending = 1,
    Active = 2,
    Ended = 3
};

// 单个数据点的报警状态
struct AlarmStatus {
    int deviceId;
    int regAddr;
    AlarmState state;
    qint64 activeAlarmId;
    QDateTime lastAlarmTime;
    QMutex mutex;

    AlarmStatus() : deviceId(0), regAddr(0), state(AlarmState::Normal), activeAlarmId(-1) {}
};

using AlarmStatusPtr = QSharedPointer<AlarmStatus>;

class AlarmManager : public QObject
{
    Q_OBJECT
public:
    static AlarmManager& instance();
    AlarmManager(const AlarmManager&) = delete;
    AlarmManager& operator=(const AlarmManager&) = delete;

    // 报警处理接口
    void processDataPoint(const DataPointPtr& dataPoint, DeviceInfoPtr device);
    void processDeviceOffline(int deviceId);
    void processDeviceOnline(int deviceId);

    // 报警确认接口
    bool confirmAlarm(qint64 alarmId, const QString& confirmedBy, const QString& confirmRemark = "");

signals:
    void sigNewAlarm(qint64 alarmId, DataPointPtr dataPoint);
    void sigAlarmEnded(qint64 alarmId);
    void sigAlarmConfirmed(qint64 alarmId);

private:
    explicit AlarmManager(QObject *parent = nullptr);
    ~AlarmManager() override = default;

    // 内部辅助函数
    AlarmStatusPtr getOrCreateAlarmStatus(int deviceId, int regAddr);
    int determineAlarmType(double value, double min, double max);
    int determineAlarmLevel(int alarmType);

private:
    mutable QMutex m_mutex;
    QMap<QString, AlarmStatusPtr> m_alarmStatusMap; // key: "deviceId_regAddr"
};

#endif // ALARMMANAGER_H
