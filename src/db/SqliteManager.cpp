#include "SqliteManager.h"
#include "../common/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

const QString SqliteManager::CONNECTION_NAME = "scada_local_sqlite";

SqliteManager& SqliteManager::instance()
{
    static SqliteManager instance;
    return instance;
}

SqliteManager::SqliteManager(QObject *parent)
    : QObject(parent),
    m_initialized(false)
{
}

SqliteManager::~SqliteManager()
{
    QMutexLocker locker(&m_mutex);
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::database(CONNECTION_NAME).close();
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

bool SqliteManager::init(const QString &dbPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        return true;
    }

    m_dbPath = dbPath;

    // 创建SQLite数据库连接
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        QString error = db.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to open SQLite database: %1").arg(error));
        return false;
    }

    // 创建表
    if (!createTables()) {
        db.close();
        return false;
    }

    m_initialized = true;
    LOG_INFO("SqliteManager", "SQLite database initialized successfully");
    return true;
}

bool SqliteManager::createTables()
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 用户配置表
    QString createUserConfigSql = R"(
        CREATE TABLE IF NOT EXISTS t_user_config (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            config_json TEXT NOT NULL,
            update_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (!query.exec(createUserConfigSql)) {
        QString error = query.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to create t_user_config: %1").arg(error));
        return false;
    }

    // 断点续传数据缓存表
    QString createCacheDataSql = R"(
        CREATE TABLE IF NOT EXISTS t_cache_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            reg_addr INTEGER NOT NULL,
            tag_name TEXT NOT NULL,
            raw_value INTEGER NOT NULL,
            scaled_value REAL NOT NULL,
            unit TEXT NOT NULL DEFAULT '',
            timestamp TEXT NOT NULL,
            is_alarm INTEGER NOT NULL DEFAULT 0,
            alarm_msg TEXT NOT NULL DEFAULT '',
            raw_data_json TEXT NOT NULL,
            create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (!query.exec(createCacheDataSql)) {
        QString error = query.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to create t_cache_data: %1").arg(error));
        return false;
    }

    // 创建索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_cache_create_time ON t_cache_data(create_time)");

    return true;
}

QSqlDatabase SqliteManager::getConnection()
{
    return QSqlDatabase::database(CONNECTION_NAME);
}

bool SqliteManager::saveUserConfig(const QString &username, const QJsonObject &config)
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QJsonDocument doc(config);
    QString configJson = doc.toJson(QJsonDocument::Compact);

    QString sql = R"(
        INSERT OR REPLACE INTO t_user_config (username, config_json)
        VALUES (:username, :config_json)
    )";
    query.prepare(sql);
    query.bindValue(":username", username);
    query.bindValue(":config_json", configJson);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to save user config for %1: %2").arg(username, error));
        return false;
    }

    LOG_INFO("SqliteManager", QString("User config saved for %1").arg(username));
    return true;
}

QJsonObject SqliteManager::loadUserConfig(const QString &username)
{
    QMutexLocker locker(&m_mutex);
    QJsonObject config;
    if (!m_initialized) return config;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = "SELECT config_json FROM t_user_config WHERE username = :username";
    query.prepare(sql);
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        QString configJson = query.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());
        if (doc.isObject()) {
            config = doc.object();
        }
    }

    return config;
}

bool SqliteManager::cacheDataPoint(const DataPointPtr &dataPoint, const QVector<quint16> &rawData)
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 序列化rawData
    QJsonArray rawDataArray;
    for (quint16 val : rawData) {
        rawDataArray.append(val);
    }
    QJsonDocument rawDoc(rawDataArray);
    QString rawDataJson = rawDoc.toJson(QJsonDocument::Compact);

    QString sql = R"(
        INSERT INTO t_cache_data (
            device_id, reg_addr, tag_name, raw_value, scaled_value, unit,
            timestamp, is_alarm, alarm_msg, raw_data_json
        ) VALUES (
            :device_id, :reg_addr, :tag_name, :raw_value, :scaled_value, :unit,
            :timestamp, :is_alarm, :alarm_msg, :raw_data_json
        )
    )";
    query.prepare(sql);
    query.bindValue(":device_id", dataPoint->deviceId);
    // 从tag_name解析reg_addr（格式：DeviceName.RegXXX）
    QStringList tagParts = dataPoint->tagName.split(".Reg");
    int regAddr = tagParts.size() == 2 ? tagParts[1].toInt() : 0;
    query.bindValue(":reg_addr", regAddr);
    query.bindValue(":tag_name", dataPoint->tagName);
    query.bindValue(":raw_value", 0); // 暂存0，后续优化
    query.bindValue(":scaled_value", dataPoint->value);
    query.bindValue(":unit", ""); // 暂存空，后续从设备配置获取
    query.bindValue(":timestamp", dataPoint->timestamp.toString(Qt::ISODate));
    query.bindValue(":is_alarm", dataPoint->isAlarm ? 1 : 0);
    query.bindValue(":alarm_msg", dataPoint->alarmMsg);
    query.bindValue(":raw_data_json", rawDataJson);

    if (!query.exec()) {
        QString error = query.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to cache data point: %1").arg(error));
        return false;
    }

    return true;
}

QList<QPair<DataPointPtr, QVector<quint16>>> SqliteManager::getCachedDataPoints(int limit)
{
    QMutexLocker locker(&m_mutex);
    QList<QPair<DataPointPtr, QVector<quint16>>> result;
    if (!m_initialized) return result;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString sql = QString("SELECT * FROM t_cache_data ORDER BY create_time ASC LIMIT %1").arg(limit);
    if (query.exec()) {
        while (query.next()) {
            // 解析DataPoint
            DataPointPtr dataPoint(new DataPoint());
            dataPoint->deviceId = query.value("device_id").toInt();
            dataPoint->tagName = query.value("tag_name").toString();
            dataPoint->value = query.value("scaled_value").toDouble();
            dataPoint->timestamp = QDateTime::fromString(query.value("timestamp").toString(), Qt::ISODate);
            dataPoint->isAlarm = query.value("is_alarm").toInt() == 1;
            dataPoint->alarmMsg = query.value("alarm_msg").toString();

            // 解析rawData
            QString rawDataJson = query.value("raw_data_json").toString();
            QJsonDocument rawDoc = QJsonDocument::fromJson(rawDataJson.toUtf8());
            QVector<quint16> rawData;
            if (rawDoc.isArray()) {
                QJsonArray rawArray = rawDoc.array();
                for (const QJsonValue& val : rawArray) {
                    rawData.append(val.toInt());
                }
            }

            result.append(qMakePair(dataPoint, rawData));
        }
    }

    return result;
}

bool SqliteManager::deleteCachedDataPoints(const QList<qint64> &ids)
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized || ids.isEmpty()) return false;

    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    // 构造IN子句
    QStringList idStrList;
    for (qint64 id : ids) {
        idStrList.append(QString::number(id));
    }
    QString idStr = idStrList.join(",");

    QString sql = QString("DELETE FROM t_cache_data WHERE id IN (%1)").arg(idStr);
    if (!query.exec(sql)) {
        QString error = query.lastError().text();
        LOG_ERROR("SqliteManager", QString("Failed to delete cached data points: %1").arg(error));
        return false;
    }

    LOG_INFO("SqliteManager", QString("Deleted %1 cached data points").arg(ids.size()));
    return true;
}
