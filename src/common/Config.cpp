#include "Config.h"
#include "Logger.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config(QObject* parent)
    : QObject(parent)
    , m_loaded(false)
{
    setDefaultValues();
}

QJsonObject Config::configRoot() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot;
}

bool Config::load(const QString& configPath)
{
    QMutexLocker locker(&m_mutex);

    // 如果未传入路径或路径为空，使用默认绝对路径
    QString path = configPath;
    if (path.isEmpty()) {
        path = getDefaultConfigPath();
    }
    m_configPath = path;

    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Config", QString("Failed to open config file: %1, error: %2")
                      .arg(m_configPath, file.errorString()));
        return false;
    }

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

bool Config::validateConfig(const QJsonObject& root) const
{
    // 检查必需的顶级键是否存在
    QStringList requiredKeys = {"mysql", "collect", "devices"};
    for (const QString& key : requiredKeys) {
        if (!root.contains(key)) {
            LOG_ERROR("Config", QString("Missing required config key: %1").arg(key));
            return false;
        }
    }

    // 检查类型
    if (!root["mysql"].isObject()) {
        LOG_ERROR("Config", "Config key 'mysql' must be an object");
        return false;
    }
    if (!root["collect"].isObject()) {
        LOG_ERROR("Config", "Config key 'collect' must be an object");
        return false;
    }
    if (!root["devices"].isArray()) {
        LOG_ERROR("Config", "Config key 'devices' must be an array");
        return false;
    }

    return true;
}

bool Config::saveConfig(const QJsonObject& newConfig)
{
    QMutexLocker locker(&m_mutex);

    // 0. 确保配置路径有效（若无效则尝试调用 load 默认路径）
    if (m_configPath.isEmpty()) {
        QString defaultPath = getDefaultConfigPath();
        if (!load(defaultPath)) {
            LOG_ERROR("Config", "Cannot save: no valid config path and failed to load default");
            return false;
        }
    }

    // 1. 验证新配置
    if (!validateConfig(newConfig)) {
        LOG_ERROR("Config", "Validation failed for new config");
        return false;
    }

    // 2. 确保目录存在
    QFileInfo fileInfo(m_configPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            LOG_ERROR("Config", "Failed to create config directory: " + dir.absolutePath());
            return false;
        }
    }

    // 3. 临时文件 & 备份文件
    QString tempPath = m_configPath + ".tmp";
    QString backupPath = m_configPath + ".bak";

    // 4. 写入临时文件
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Config", "Failed to open temp file: " + tempPath);
        return false;
    }
    QJsonDocument doc(newConfig);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.flush()) {
        file.close();
        QFile::remove(tempPath);
        LOG_ERROR("Config", "Flush failed");
        return false;
    }
    file.close();

    // 5. 原子保存：备份 -> 替换 -> 删除备份
    if (QFile::exists(m_configPath)) {
        if (!QFile::rename(m_configPath, backupPath)) {
            LOG_ERROR("Config", "Failed to rename to backup");
            QFile::remove(tempPath);
            return false;
        }
    }
    if (!QFile::rename(tempPath, m_configPath)) {
        LOG_ERROR("Config", "Failed to rename temp to config");
        if (QFile::exists(backupPath))
            QFile::rename(backupPath, m_configPath);
        QFile::remove(tempPath);
        return false;
    }
    QFile::remove(backupPath);

    // 6. 更新内存
    m_configRoot = newConfig;
    m_loaded = true;

    // 【关键修复】在锁内发射信号（递归锁允许，避免竞态）
    // 并确保使用 Qt::QueuedConnection 的连接不会立即销毁发送者
    emit configChanged();

    LOG_INFO("Config", "Config saved successfully");
    return true;
}

void Config::setDefaultValues()
{
    m_configRoot["mysql"] = QJsonObject{
        {"host", "127.0.0.1"},
        {"port", 3306},
        {"user", "root"},
        {"password", "1234"},
        {"database", "modbus_scada"}
    };

    m_configRoot["collect"] = QJsonObject{
        {"interval_ms", 5000},
        {"heartbeat_ms", 5000},
        {"reconnect_ms", 3000}
    };

    m_configRoot["devices"] = QJsonArray();
}

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
    return m_configRoot["collect"].toObject()["interval_ms"].toInt(5000);
}

int Config::heartbeatIntervalMs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["collect"].toObject()["heartbeat_ms"].toInt(5000);
}

int Config::reconnectIntervalMs() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["collect"].toObject()["reconnect_ms"].toInt(5000);
}

QJsonArray Config::deviceList() const
{
    QMutexLocker locker(&m_mutex);
    return m_configRoot["devices"].toArray();
}

QString Config::getDefaultConfigPath()
{
    return  QString("%1/config/config.json").arg(PROJECT_SOURCE_DIR);
}