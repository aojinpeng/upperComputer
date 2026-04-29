#include "AlarmManager.h"
#include "../db/MysqlManager.h"
#include "../common/Logger.h"

AlarmManager& AlarmManager::instance()
{
    static AlarmManager instance;
    return instance;
}

AlarmManager::AlarmManager(QObject *parent)
    : QObject(parent)
{
}

void AlarmManager::processDataPoint(const DataPointPtr &dataPoint, DeviceInfoPtr device)
{
    // 从tag_name解析reg_addr
    QStringList tagParts = dataPoint->tagName.split(".Reg");
    int regAddr = tagParts.size() == 2 ? tagParts[1].toInt() : 0;
    QString key = QString("%1_%2").arg(dataPoint->deviceId).arg(regAddr);

    AlarmStatusPtr status = getOrCreateAlarmStatus(dataPoint->deviceId, regAddr);
    QMutexLocker locker(&status->mutex);

    if (dataPoint->isAlarm) {
        // 有报警
        if (status->state == AlarmState::Normal || status->state == AlarmState::Ended) {
            // 新报警
            int alarmType = determineAlarmType(dataPoint->value, device->alarmMin, device->alarmMax);
            int alarmLevel = determineAlarmLevel(alarmType);
            qint64 alarmId = MysqlManager::instance().insertAlarm(dataPoint, alarmType, alarmLevel);
            if (alarmId != -1) {
                status->state = AlarmState::Active;
                status->activeAlarmId = alarmId;
                status->lastAlarmTime = dataPoint->timestamp;
                emit sigNewAlarm(alarmId, dataPoint);
            }
        }
        // 否则：报警已存在，忽略
    } else {
        // 无报警
        if (status->state == AlarmState::Active) {
            // 报警结束
            if (MysqlManager::instance().updateAlarmEndTime(status->activeAlarmId, dataPoint->timestamp)) {
                emit sigAlarmEnded(status->activeAlarmId);
                status->state = AlarmState::Ended;
                status->activeAlarmId = -1;
            }
        }
    }
}

void AlarmManager::processDeviceOffline(int deviceId)
{
    QMutexLocker locker(&m_mutex);
    // 遍历该设备的所有报警状态
    for (auto it = m_alarmStatusMap.begin(); it != m_alarmStatusMap.end(); ++it) {
        QString key = it.key();
        if (key.startsWith(QString("%1_").arg(deviceId))) {
            AlarmStatusPtr status = it.value();
            QMutexLocker statusLocker(&status->mutex);
            if (status->state != AlarmState::Active) {
                // 生成离线报警
                DataPointPtr offlineData(new DataPoint());
                offlineData->deviceId = deviceId;
                offlineData->tagName = QString("Device_%1.Offline").arg(deviceId);
                offlineData->value = 0.0;
                offlineData->timestamp = QDateTime::currentDateTime();
                offlineData->isAlarm = true;
                offlineData->alarmMsg = QString("Device %1 is offline").arg(deviceId);

                int alarmType = 3; // 离线报警
                int alarmLevel = 1; // 紧急
                qint64 alarmId = MysqlManager::instance().insertAlarm(offlineData, alarmType, alarmLevel);
                if (alarmId != -1) {
                    status->state = AlarmState::Active;
                    status->activeAlarmId = alarmId;
                    status->lastAlarmTime = offlineData->timestamp;
                    emit sigNewAlarm(alarmId, offlineData);
                }
            }
        }
    }
}

void AlarmManager::processDeviceOnline(int deviceId)
{
    QMutexLocker locker(&m_mutex);
    // 遍历该设备的所有报警状态
    for (auto it = m_alarmStatusMap.begin(); it != m_alarmStatusMap.end(); ++it) {
        QString key = it.key();
        if (key.startsWith(QString("%1_").arg(deviceId))) {
            AlarmStatusPtr status = it.value();
            QMutexLocker statusLocker(&status->mutex);
            if (status->state == AlarmState::Active) {
                // 检查是否是离线报警
                // 简化处理：直接结束所有该设备的活跃报警
                if (MysqlManager::instance().updateAlarmEndTime(status->activeAlarmId, QDateTime::currentDateTime())) {
                    emit sigAlarmEnded(status->activeAlarmId);
                    status->state = AlarmState::Ended;
                    status->activeAlarmId = -1;
                }
            }
        }
    }
}

bool AlarmManager::confirmAlarm(qint64 alarmId, const QString &confirmedBy, const QString &confirmRemark)
{
    if (MysqlManager::instance().confirmAlarm(alarmId, confirmedBy, confirmRemark)) {
        emit sigAlarmConfirmed(alarmId);
        return true;
    }
    return false;
}

AlarmStatusPtr AlarmManager::getOrCreateAlarmStatus(int deviceId, int regAddr)
{
    QMutexLocker locker(&m_mutex);
    QString key = QString("%1_%2").arg(deviceId).arg(regAddr);
    if (!m_alarmStatusMap.contains(key)) {
        AlarmStatusPtr status(new AlarmStatus());
        status->deviceId = deviceId;
        status->regAddr = regAddr;
        m_alarmStatusMap[key] = status;
    }
    return m_alarmStatusMap[key];
}

int AlarmManager::determineAlarmType(double value, double min, double max)
{
    if (value < min) return 1; // 下限报警
    if (value > max) return 2; // 上限报警
    return 0; // 无报警
}

int AlarmManager::determineAlarmLevel(int alarmType)
{
    switch (alarmType) {
    case 1: // 下限
    case 2: // 上限
        return 2; // 重要
    case 3: // 离线
        return 1; // 紧急
    default:
        return 3; // 一般
    }
}
