 
#include "Config.h"
#include "Logger.h"
#include <QFile>

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config(QObject *parent)
    : QObject(parent),
    m_loaded(false)
{
    setDefaultValues(); // 先设置默认值，防止加载失败导致崩溃
}

bool Config::load(const QString &configPath)
{
    QMutexLocker locker(&m_mutex);
    m_configPath = configPath;

    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Config", QString("Failed to open config file: %1, error: %2")
                      .arg(m_configPath, file.errorString()));
        return false;
    }

    // 解析JSON
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        LOG_ERROR("Config", QString("Failed to parse config file: %1, error: %2")
                      .arg(m_configPath, parseError.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        LOG_ERROR("Config", "Config file root must be a JSON object");
        return false;
    }

    QJsonObject root = doc.object();
    if (!validateConfig(root)) {
        LOG_ERROR("Config", "Config file validation failed");
        return false;
    }

    m_configRoot = root;
    m_loaded = true;
    LOG_INFO("Config", "配置文件加载成功");
    emit configChanged();
    return true;
}

bool Config::reload()
{
    LOG_INFO("Config", "Reloading config file...");
    return load(m_configPath);
}

bool Config::validateConfig(const QJsonObject &root) const
{
    // 简单校验：检查必需的顶层键
    QStringList requiredKeys = {"mysql", "collect", "devices"};
    for (const QString& key : requiredKeys) {
        if (!root.contains(key)) {
            LOG_ERROR("Config", QString("Missing required config key: %1").arg(key));
            return false;
        }
    }
    return true;
}

void Config::setDefaultValues()
{
    // MySQL默认配置
    m_configRoot["mysql"] = QJsonObject{
        {"host", "127.0.0.1"},
        {"port", 3306},
        {"user", "root"},
        {"password", "1234"},
        {"database", "modbus_scada"}
    };

    // 采集默认配置
    m_configRoot["collect"] = QJsonObject{
        {"interval_ms", 1000},
        {"heartbeat_ms", 5000},
        {"reconnect_ms", 3000}
    };

    // 空设备列表
    m_configRoot["devices"] = QJsonArray();
}

// 配置项Get实现（带默认值兜底）
QString Config::mysqlHost() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["mysql"].toObject()["host"].toString("127.0.0.1");
}

int Config::mysqlPort() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["mysql"].toObject()["port"].toInt(3306);
}

QString Config::mysqlUser() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["mysql"].toObject()["user"].toString("root");
}

QString Config::mysqlPassword() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["mysql"].toObject()["password"].toString("");
}

QString Config::mysqlDatabase() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["mysql"].toObject()["database"].toString("modbus_scada");
}

int Config::collectIntervalMs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["collect"].toObject()["interval_ms"].toInt(1000);
}

int Config::heartbeatIntervalMs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["collect"].toObject()["heartbeat_ms"].toInt(5000);
}

int Config::reconnectIntervalMs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["collect"].toObject()["reconnect_ms"].toInt(3000);
}

QJsonArray Config::deviceList() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["devices"].toArray();
}