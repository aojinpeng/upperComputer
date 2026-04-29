#ifndef MYSQLMANAGER_H
#define MYSQLMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include "../common/Defines.h"

class MysqlManager : public QObject
{
    Q_OBJECT
public:
    static MysqlManager& instance();
    MysqlManager(const MysqlManager&) = delete;
    MysqlManager& operator=(const MysqlManager&) = delete;

    // 初始化接口
    bool init(const QString& host, int port, const QString& user, const QString& password, const QString& database);
    bool isConnected() const;
    void reconnect();

    // 设备数据接口
    bool syncDeviceFromConfig(const QJsonArray& deviceArray);
    QList<DeviceInfoPtr> loadDevicesFromDb();

    // 历史数据接口
    bool insertHistoryData(const DataPointPtr& dataPoint, quint16 rawValue, double scaleFactor, double offset, const QString& unit);
    bool insertHistoryDataBatch(const QList<QPair<DataPointPtr, QVector<quint16>>>& dataList);
    QList<DataPointPtr> queryHistoryData(int deviceId, const QDateTime& startTime, const QDateTime& endTime, int limit = 10000);

    // 报警数据接口
    qint64 insertAlarm(const DataPointPtr& dataPoint, int alarmType, int alarmLevel);
    bool updateAlarmEndTime(qint64 alarmId, const QDateTime& endTime);
    bool confirmAlarm(qint64 alarmId, const QString& confirmedBy, const QString& confirmRemark = "");
    QList<QPair<qint64, DataPointPtr>> queryUnconfirmedAlarms(int limit = 100);

signals:
    void sigConnectionLost();
    void sigConnectionRestored();

private:
    explicit MysqlManager(QObject *parent = nullptr);
    ~MysqlManager() override;

    // 内部辅助函数
    QSqlDatabase getConnection() const;
    bool testConnection() const;

private:
    mutable QMutex m_mutex;
    QString m_host;
    int m_port;
    QString m_user;
    QString m_password;
    QString m_database;
    bool m_initialized;
    bool m_connected;
    static const QString CONNECTION_NAME;
};

#endif // MYSQLMANAGER_H
