#include "MysqlManager.h"
#include "../common/Logger.h"
#include "../common/Config.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QTimer>
#include <QThread>

const QString MysqlManager::CONNECTION_NAME = "modbus_scada";
QString upsertSql;
// QSqlQuery query2;
MysqlManager& MysqlManager::instance()
{
    static MysqlManager instance;
    return instance;
}

MysqlManager::MysqlManager(QObject *parent)
    : QObject(parent),
    m_port(3306),
    m_initialized(false),
    m_connected(false)
{
}

MysqlManager::~MysqlManager()
{
    QMutexLocker locker(&m_mutex);
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::database(CONNECTION_NAME).close();
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

bool MysqlManager::init(const QString &host, int port, const QString &user, const QString &password, const QString &database)
{
    QMutexLocker locker(&m_mutex);
    qDebug()<<host<<port<<user;
    m_host = host;
    m_port = port;
    m_user = user;
    m_password = password;
    m_database = database;

    // 创建MySQL数据库连接
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", CONNECTION_NAME);
    db.setHostName(m_host);
    db.setPort(m_port);
    db.setUserName(m_user);
    db.setPassword(m_password);
    db.setDatabaseName(m_database);
    db.setConnectOptions("MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=5");

    if (!db.open()) {
        QString error = db.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to open MySQL database: %1").arg(error));
        m_connected = false;
        emit sigConnectionLost();
        return false;
    }

    m_initialized = true;
    m_connected = true;
    LOG_INFO("MysqlManager", "MySQL database initialized successfully");
    emit sigConnectionRestored();
    return true;
}

bool MysqlManager::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return false;
    QSqlDatabase db = getConnection();
    return db.isOpen() && testConnection() ;
}

void MysqlManager::reconnect()
{
    LOG_INFO("MysqlManager", "Reconnecting to MySQL database...");
    init(m_host, m_port, m_user, m_password, m_database);
}

QSqlDatabase MysqlManager::getConnection() const
{
    return QSqlDatabase::database(CONNECTION_NAME);
}

bool MysqlManager::testConnection() const
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    return query.exec("SELECT 1");
}

bool MysqlManager::syncDeviceFromConfig(const QJsonArray &deviceArray)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) {
        LOG_ERROR("MysqlManager", "Database not connected");
        return false;
    }

    // 用你原来的连接，不新建、不修改、不搞线程
    QSqlDatabase db = getConnection();
    if (!db.isOpen()) {
        LOG_ERROR("MysqlManager", "Connection closed");
        return false;
    }

    QSqlQuery query(db);
    bool success = true;

    // 1. 软删除（无事务，基础SQL，绝对能执行）
    QString softDeleteSql = "UPDATE t_device SET is_deleted = 1 WHERE is_deleted = 0";
    if (!query.exec(softDeleteSql)) {
        LOG_ERROR("MysqlManager", "Delete error: " + query.lastError().text());
        return false;
    }

    // 2. 插入更新（基础写法，无全局变量，无事务）
    QString upsertSql = R"(
        INSERT INTO t_device (
            device_id, device_name, ip, port, slave_id, start_addr, reg_count,
            alarm_min, alarm_max, scale_factor, offset, unit, is_deleted
        ) VALUES (
            :device_id, :device_name, :ip, :port, :slave_id, :start_addr, :reg_count,
            :alarm_min, :alarm_max, :scale_factor, :offset, :unit, 0
        ) ON DUPLICATE KEY UPDATE
            device_name = VALUES(device_name),
            ip = VALUES(ip),
            port = VALUES(port),
            slave_id = VALUES(slave_id),
            start_addr = VALUES(start_addr),
            reg_count = VALUES(reg_count),
            alarm_min = VALUES(alarm_min),
            alarm_max = VALUES(alarm_max),
            scale_factor = VALUES(scale_factor),
            offset = VALUES(offset),
            unit = VALUES(unit),
            is_deleted = 0
    )";

    query.prepare(upsertSql);
    for (const QJsonValue& value : deviceArray) {
        QJsonObject obj = value.toObject();
        query.bindValue(":device_id", obj["device_id"].toInt());
        query.bindValue(":device_name", obj["device_name"].toString());
        query.bindValue(":ip", obj["ip"].toString());
        query.bindValue(":port", obj["port"].toInt());
        query.bindValue(":slave_id", obj["slave_id"].toInt());
        query.bindValue(":start_addr", obj["start_addr"].toInt());
        query.bindValue(":reg_count", obj["reg_count"].toInt());
        query.bindValue(":alarm_min", obj["alarm_min"].toDouble());
        query.bindValue(":alarm_max", obj["alarm_max"].toDouble());
        query.bindValue(":scale_factor", obj["scale_factor"].toDouble(0.1));
        query.bindValue(":offset", obj["offset"].toDouble(0.0));
        query.bindValue(":unit", obj["unit"].toString(""));

        if (!query.exec()) {
            LOG_ERROR("MysqlManager", "Insert error: " + query.lastError().text());
            return false;
        }
    }

    LOG_INFO("MysqlManager", "Sync success!");
    return true;
}
QList<DeviceInfoPtr> MysqlManager::loadDevicesFromDb()
{
    QMutexLocker locker(&m_mutex);
    QList<DeviceInfoPtr> devices;
    if (!m_connected) return devices;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = "SELECT * FROM t_device WHERE is_deleted = 0 ORDER BY device_id ASC";
    if (query.exec()) {
        while (query.next()) {
            DeviceInfoPtr device(new DeviceInfo());
            device->deviceId = query.value("device_id").toInt();
            device->deviceName = query.value("device_name").toString();
            device->ip = query.value("ip").toString();
            device->port = query.value("port").toInt();
            device->slaveId = query.value("slave_id").toInt();
            device->startAddr = query.value("start_addr").toInt();
            device->regCount = query.value("reg_count").toInt();
            device->alarmMin = query.value("alarm_min").toDouble();
            device->alarmMax = query.value("alarm_max").toDouble();
            // 暂存缩放系数和偏移量到rawData的前两个位置（预留扩展）
            device->rawData.append(static_cast<quint16>(query.value("scale_factor").toDouble() * 1000));
            device->rawData.append(static_cast<quint16>(query.value("offset").toDouble() * 1000));
            device->status = DeviceStatus::Offline;
            devices.append(device);
        }
    } else {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to load devices from DB: %1").arg(error));
    }

    return devices;
}

bool MysqlManager::insertHistoryData(const DataPointPtr &dataPoint, quint16 rawValue, double scaleFactor, double offset, const QString &unit)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 从tag_name解析reg_addr
    QStringList tagParts = dataPoint->tagName.split(".Reg");
    int regAddr = tagParts.size() == 2 ? tagParts[1].toInt() : 0;

    QString sql = R"(
        INSERT INTO t_history_data (
            device_id, reg_addr, tag_name, raw_value, scaled_value, unit, timestamp
        ) VALUES (
            :device_id, :reg_addr, :tag_name, :raw_value, :scaled_value, :unit, :timestamp
        )
    )";
    query.prepare(sql);
    query.bindValue(":device_id", dataPoint->deviceId);
    query.bindValue(":reg_addr", regAddr);
    query.bindValue(":tag_name", dataPoint->tagName);
    query.bindValue(":raw_value", rawValue);
    query.bindValue(":scaled_value", dataPoint->value);
    query.bindValue(":unit", unit);
    query.bindValue(":timestamp", dataPoint->timestamp);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to insert history data: %1").arg(error));
        return false;
    }

    return true;
}

bool MysqlManager::insertHistoryDataBatch(const QList<QPair<DataPointPtr, QVector<quint16>>> &dataList)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected || dataList.isEmpty()) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 开启事务
    if (!db.transaction()) {
        QString error = db.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to start transaction for batch insert: %1").arg(error));
        return false;
    }

    bool success = true;

    // 批量插入
    QString sql = R"(
        INSERT INTO t_history_data (
            device_id, reg_addr, tag_name, raw_value, scaled_value, unit, timestamp
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )";
    query.prepare(sql);

    QVariantList deviceIds, regAddrs, tagNames, rawValues, scaledValues, units, timestamps;

    for (const auto& pair : dataList) {
        const DataPointPtr& dataPoint = pair.first;
        // 从tag_name解析reg_addr
        QStringList tagParts = dataPoint->tagName.split(".Reg");
        int regAddr = tagParts.size() == 2 ? tagParts[1].toInt() : 0;
        // 简化处理：rawValue取第一个，scaleFactor和offset取默认
        quint16 rawValue = pair.second.isEmpty() ? 0 : pair.second[0];

        deviceIds << dataPoint->deviceId;
        regAddrs << regAddr;
        tagNames << dataPoint->tagName;
        rawValues << rawValue;
        scaledValues << dataPoint->value;
        units << "";
        timestamps << dataPoint->timestamp;
    }

    query.addBindValue(deviceIds);
    query.addBindValue(regAddrs);
    query.addBindValue(tagNames);
    query.addBindValue(rawValues);
    query.addBindValue(scaledValues);
    query.addBindValue(units);
    query.addBindValue(timestamps);

    if (!query.execBatch()) {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to execute batch insert: %1").arg(error));
        success = false;
        goto rollback;
    }

    // 提交事务
    if (!db.commit()) {
        QString error = db.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to commit batch insert: %1").arg(error));
        success = false;
        goto rollback;
    }

    LOG_INFO("MysqlManager", QString("Batch inserted %1 history data points").arg(dataList.size()));
    return success;

rollback:
    db.rollback();
    return success;
}

QList<DataPointPtr> MysqlManager::queryHistoryData(int deviceId, const QDateTime &startTime, const QDateTime &endTime, int limit)
{
    QMutexLocker locker(&m_mutex);
    QList<DataPointPtr> dataPoints;
    if (!m_connected) return dataPoints;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = R"(
        SELECT device_id, tag_name, scaled_value, timestamp, is_alarm, alarm_msg
        FROM t_history_data
        WHERE device_id = :device_id
          AND timestamp BETWEEN :start_time AND :end_time
        ORDER BY timestamp ASC
        LIMIT :limit
    )";
    query.prepare(sql);
    query.bindValue(":device_id", deviceId);
    query.bindValue(":start_time", startTime);
    query.bindValue(":end_time", endTime);
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            DataPointPtr dataPoint(new DataPoint());
            dataPoint->deviceId = query.value("device_id").toInt();
            dataPoint->tagName = query.value("tag_name").toString();
            dataPoint->value = query.value("scaled_value").toDouble();
            dataPoint->timestamp = query.value("timestamp").toDateTime();
            // 历史数据暂不存储is_alarm和alarm_msg，后续优化
            dataPoint->isAlarm = false;
            dataPoint->alarmMsg = "";
            dataPoints.append(dataPoint);
        }
    } else {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to query history data: %1").arg(error));
    }

    return dataPoints;
}

qint64 MysqlManager::insertAlarm(const DataPointPtr &dataPoint, int alarmType, int alarmLevel)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return -1;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 从tag_name解析reg_addr
    QStringList tagParts = dataPoint->tagName.split(".Reg");
    int regAddr = tagParts.size() == 2 ? tagParts[1].toInt() : 0;

    QString sql = R"(
        INSERT INTO t_alarm (
            device_id, reg_addr, tag_name, alarm_type, alarm_level,
            alarm_value, alarm_msg, start_time
        ) VALUES (
            :device_id, :reg_addr, :tag_name, :alarm_type, :alarm_level,
            :alarm_value, :alarm_msg, :start_time
        )
    )";
    query.prepare(sql);
    query.bindValue(":device_id", dataPoint->deviceId);
    query.bindValue(":reg_addr", regAddr);
    query.bindValue(":tag_name", dataPoint->tagName);
    query.bindValue(":alarm_type", alarmType);
    query.bindValue(":alarm_level", alarmLevel);
    query.bindValue(":alarm_value", dataPoint->value);
    query.bindValue(":alarm_msg", dataPoint->alarmMsg);
    query.bindValue(":start_time", dataPoint->timestamp);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to insert alarm: %1").arg(error));
        return -1;
    }

    qint64 alarmId = query.lastInsertId().toLongLong();
    LOG_WARNING("MysqlManager", QString("Alarm inserted: ID=%1, Device=%2, Msg=%3")
                                    .arg(alarmId).arg(dataPoint->deviceId).arg(dataPoint->alarmMsg));
    return alarmId;
}

bool MysqlManager::updateAlarmEndTime(qint64 alarmId, const QDateTime &endTime)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = "UPDATE t_alarm SET end_time = :end_time WHERE id = :id AND end_time IS NULL";
    query.prepare(sql);
    query.bindValue(":end_time", endTime);
    query.bindValue(":id", alarmId);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to update alarm end time: %1").arg(error));
        return false;
    }

    if (query.numRowsAffected() > 0) {
        LOG_INFO("MysqlManager", QString("Alarm ended: ID=%1").arg(alarmId));
    }
    return true;
}

bool MysqlManager::confirmAlarm(qint64 alarmId, const QString &confirmedBy, const QString &confirmRemark)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connected) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = R"(
        UPDATE t_alarm
        SET is_confirmed = 1, confirmed_by = :confirmed_by,
            confirm_time = :confirm_time, confirm_remark = :confirm_remark
        WHERE id = :id AND is_confirmed = 0
    )";
    query.prepare(sql);
    query.bindValue(":confirmed_by", confirmedBy);
    query.bindValue(":confirm_time", QDateTime::currentDateTime());
    query.bindValue(":confirm_remark", confirmRemark);
    query.bindValue(":id", alarmId);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to confirm alarm: %1").arg(error));
        return false;
    }

    if (query.numRowsAffected() > 0) {
        LOG_INFO("MysqlManager", QString("Alarm confirmed: ID=%1, By=%2").arg(alarmId).arg(confirmedBy));
    }
    return true;
}

QList<QPair<qint64, DataPointPtr>> MysqlManager::queryUnconfirmedAlarms(int limit)
{
    QMutexLocker locker(&m_mutex);
    QList<QPair<qint64, DataPointPtr>> alarms;
    if (!m_connected) return alarms;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = R"(
        SELECT id, device_id, tag_name, alarm_value, alarm_msg, start_time
        FROM t_alarm
        WHERE is_confirmed = 0
        ORDER BY start_time DESC
        LIMIT :limit
    )";
    query.prepare(sql);
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            qint64 alarmId = query.value("id").toLongLong();
            DataPointPtr dataPoint(new DataPoint());
            dataPoint->deviceId = query.value("device_id").toInt();
            dataPoint->tagName = query.value("tag_name").toString();
            dataPoint->value = query.value("alarm_value").toDouble();
            dataPoint->timestamp = query.value("start_time").toDateTime();
            dataPoint->isAlarm = true;
            dataPoint->alarmMsg = query.value("alarm_msg").toString();
            alarms.append(qMakePair(alarmId, dataPoint));
        }
    } else {
        QString error = query.lastError().text();
        LOG_ERROR("MysqlManager", QString("Failed to query unconfirmed alarms: %1").arg(error));
    }

    return alarms;
}