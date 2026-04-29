#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QRecursiveMutex>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class Config : public QObject
{
    Q_OBJECT
public:
    static Config& instance();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 如果 configPath 为空，则使用默认绝对路径（应用程序目录下的 config/config.json）
    bool load(const QString& configPath = "./config/config.json");
    bool reload();
    bool isLoaded() const { QMutexLocker locker(&m_mutex); return m_loaded; }

    QJsonObject configRoot() const;
    bool saveConfig(const QJsonObject& newConfig);

    // 配置项获取接口
    QString mysqlHost() const;
    int mysqlPort() const;
    QString mysqlUser() const;
    QString mysqlPassword() const;
    QString mysqlDatabase() const;

    int collectIntervalMs() const;
    int heartbeatIntervalMs() const;
    int reconnectIntervalMs() const;

    QJsonArray deviceList() const;

signals:
    void configChanged();

private:
    explicit Config(QObject* parent = nullptr);
    ~Config() override = default;

    bool validateConfig(const QJsonObject& root) const;
    void setDefaultValues();
    static QString getDefaultConfigPath();   // 获取应用程序目录下的默认配置路径

private:
    mutable QRecursiveMutex m_mutex;
    QString m_configPath;
    QJsonObject m_configRoot;
    bool m_loaded;
};

#endif // CONFIG_H