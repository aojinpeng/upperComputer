  
#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>

class Utils : public QObject
{
    Q_OBJECT
public:
    static Utils& instance();
    Utils(const Utils&) = delete;
    Utils& operator=(const Utils&) = delete;

    // 数据转换工具
    static quint16 bytesToUint16(const QByteArray& data, int offset = 0, bool bigEndian = true);
    static qint16 bytesToInt16(const QByteArray& data, int offset = 0, bool bigEndian = true);
    static QByteArray uint16ToBytes(quint16 value, bool bigEndian = true);

    // 时间工具
    static QString getCurrentTimestamp(); // ISO8601格式
    static qint64 getCurrentMsSinceEpoch();

    // 工业数据处理工具
    static double filterOutlier(double value, double min, double max, double defaultValue = 0.0);
    static double clamp(double value, double min, double max);

    // 字符串工具
    static QString padString(const QString& str, int width, QChar fillChar = QLatin1Char('0'));

private:
    explicit Utils(QObject *parent = nullptr) ;//改
    ~Utils() override = default;
};

#endif // UTILS_H