 
#include "Utils.h"
#include <QDataStream>
#include <cmath>

Utils::Utils(QObject *parent) : QObject(parent){

}

Utils& Utils::instance()
{
    static Utils instance;
    return instance;
}

quint16 Utils::bytesToUint16(const QByteArray &data, int offset, bool bigEndian)
{
    if (offset + 1 >= data.size()) {
        return 0;
    }
    quint16 value = 0;
    QDataStream stream(data.mid(offset, 2));
    stream.setByteOrder(bigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
    stream >> value;
    return value;
}

qint16 Utils::bytesToInt16(const QByteArray &data, int offset, bool bigEndian)
{
    return static_cast<qint16>(bytesToUint16(data, offset, bigEndian));
}

QByteArray Utils::uint16ToBytes(quint16 value, bool bigEndian)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(bigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
    stream << value;
    return data;
}

QString Utils::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

qint64 Utils::getCurrentMsSinceEpoch()
{
    return QDateTime::currentMSecsSinceEpoch();
}

double Utils::filterOutlier(double value, double min, double max, double defaultValue)
{
    if (value < min || value > max || std::isnan(value) || std::isinf(value)) {
        return defaultValue;
    }
    return value;
}

double Utils::clamp(double value, double min, double max)
{
    return qBound(min, value, max);
}

QString Utils::padString(const QString &str, int width, QChar fillChar)
{
    return str.rightJustified(width, fillChar);
}