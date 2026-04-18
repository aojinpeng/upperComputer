 
#include "Logger.h"
#include <QDir>
#include <QCoreApplication>
// ========== 修复：添加缺失的2个头文件（解决所有报错！） ==========
#include <QDebug>   // 解决 qDebug/qCritical 报错
#include <QMap>     // 解决 QMap 容器报错
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

void Logger::rotateLogFile()
{
    if (m_logFile.size() >= m_maxFileSize) {
        m_logStream.flush();
        m_logFile.close();

        // 重命名旧文件（添加时间戳）
        QString oldFileName = m_logFile.fileName();
        QString newFileName = oldFileName + "." + QDateTime::currentDateTime().toString("hhmmss");
        QFile::rename(oldFileName, newFileName);

        // 重新打开新日志文件
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_logStream.setDevice(&m_logFile);
            LOG_INFO("Logger", "Log file rotated");
        }
    }
}

void Logger::flushLog()
{
    m_logStream.flush();
    m_logFile.flush();
}