 
#include "Logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QMap>
#include <QMutexLocker> // Qt自动锁工具类
// 单例实现：线程安全的局部静态变量
Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent),
    m_minLevel(LogLevel::Info),
    m_maxFileSize(10 * 1024 * 1024),
    m_initialized(false)
{
}

Logger::~Logger()
{

    QMutexLocker locker(&m_mutex);
    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
}

void Logger::init(const QString &logDir, LogLevel minLevel, qint64 maxFileSize)
{
     QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        return; // 防止重复初始化
    }

    m_logDir = logDir;
    m_minLevel = minLevel;
    m_maxFileSize = maxFileSize;

    // 创建日志目录（跨平台）
    QDir dir(m_logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 初始化日志文件（按日期命名）
    QString logFileName = QString("scada_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    m_logFile.setFileName(dir.filePath(logFileName));
     qDebug() << "这是文件路径"<< dir.filePath(logFileName);
    // 打开日志文件（Append模式）
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
        m_logStream.setCodec("UTF-8"); // 强制UTF-8编码
        m_initialized = true;
      qInfo() << "Logger: Log system initialized successfully";
    } else {
        qCritical() << "Failed to open log file:" << m_logFile.errorString();
    }
}

void Logger::log(LogLevel level, const QString &module, const QString &message)
{
    QMutexLocker locker(&m_mutex);
    if (!m_initialized || level < m_minLevel) {
        return;
    }

    // 构造日志格式：[时间] [级别] [模块] 消息
    QString logLine = QString("[%1] [%2] [%3] %4")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                          .arg(levelToString(level))
                          .arg(module)
                          .arg(message);

    // 输出到控制台（Debug模式）
#ifndef QT_NO_DEBUG
    if (level >= LogLevel::Warning) {
        qWarning() << logLine;
    } else {
        qDebug() << logLine;
    }
#endif

    // 输出到文件
    if (m_logFile.isOpen()) {
        m_logStream << logLine << endl;
        rotateLogFile(); // 检查是否需要轮转
        flushLog();      // 强制刷新（工业场景要求实时性）
    }
}

QString Logger::levelToString(LogLevel level) const
{
    static const QMap<LogLevel, QString> levelMap = {
        {LogLevel::Debug, "DEBUG"},
        {LogLevel::Info, "INFO"},
        {LogLevel::Warning, "WARNING"},
        {LogLevel::Error, "ERROR"},
        {LogLevel::Fatal, "FATAL"}
    };
    return levelMap.value(level, "UNKNOWN");
}

void Logger::rotateLogFile() {
    if (m_logFile.size() >= m_maxFileSize) {
        // 1. 关闭当前文件
        m_logStream.flush();
        m_logFile.close();

        // 2. 重命名旧文件（加序号）
        QString baseName = m_logFile.fileName().left(m_logFile.fileName().lastIndexOf('.'));
        QString extension = m_logFile.fileName().mid(m_logFile.fileName().lastIndexOf('.'));
        QString newFileName = QString("%1_%2%3").arg(baseName).arg(1).arg(extension);

        // 如果 scada_20260427_1.log 已存在，就找下一个序号（简单版）
        int i = 1;
        while (QFile::exists(newFileName)) {
            newFileName = QString("%1_%2%3").arg(baseName).arg(++i).arg(extension);
        }
        QFile::rename(m_logFile.fileName(), newFileName);

        // 3. 重新打开新文件
        m_logFile.setFileName(m_logDir + "/scada_" + QDateTime::currentDateTime().toString("yyyyMMdd") + ".log");
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_logStream.setDevice(&m_logFile);
            m_logStream.setCodec("UTF-8");
        }
    }
}

void Logger::flushLog()
{
    m_logStream.flush();
    m_logFile.flush();
}