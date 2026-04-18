  
#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QMutex>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class Config : public QObject
{
    Q_OBJECT
public:
    // 单例模式
    static Config& instance();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 加载/重载配置文件
    bool load(const QString& configPath = "./config/config.json");
    bool reload(); // 热加载接口

    // 配置项获取接口（带默认值，避免硬编码）
    // 数据库配置
    QString mysqlHost() const;
    int mysqlPort() const;
    QString mysqlUser() const;
    QString mysqlPassword() const;
    QString mysqlDatabase() const;

    // 采集配置
    int collectIntervalMs() const;
    int heartbeatIntervalMs() const;
    int reconnectIntervalMs() const;

    // 设备配置（返回JSON数组，业务层解析）
    QJsonArray deviceList() const;

signals:
    void configChanged(); // 配置变更信号（用于热加载通知业务层）

private:
    explicit Config(QObject *parent = nullptr);
    ~Config() override = default;

    // 内部辅助函数
    bool validateConfig(const QJsonObject& root) const;
    void setDefaultValues();

private:
    mutable QMutex m_mutex;
    QString m_configPath;
    QJsonObject m_configRoot;
    bool m_loaded;
};

#endif // CONFIG_H