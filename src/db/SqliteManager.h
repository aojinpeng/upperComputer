  
#ifndef SQLITEMANAGER_H
#define SQLITEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include "../common/Defines.h"

class SqliteManager : public QObject
{
    Q_OBJECT
public:
    static SqliteManager& instance();
    SqliteManager(const SqliteManager&) = delete;
    SqliteManager& operator=(const SqliteManager&) = delete;

    // 初始化接口
    bool init(const QString& dbPath = "./config/scada_local.db");

    // 配置缓存接口
    bool saveUserConfig(const QString& username, const QJsonObject& config);
    QJsonObject loadUserConfig(const QString& username);

    // 断点续传数据缓存接口
    bool cacheDataPoint(const DataPointPtr& dataPoint, const QVector<quint16>& rawData);
    QList<QPair<DataPointPtr, QVector<quint16>>> getCachedDataPoints(int limit = 1000);
    bool deleteCachedDataPoints(const QList<qint64>& ids);

private:
    explicit SqliteManager(QObject *parent = nullptr);
    ~SqliteManager() override;

    // 内部辅助函数
    bool createTables();
    QSqlDatabase getConnection();

private:
    mutable QMutex m_mutex;
    QString m_dbPath;
    bool m_initialized;
    static const QString CONNECTION_NAME;
};

#endif // SQLITEMANAGER_H