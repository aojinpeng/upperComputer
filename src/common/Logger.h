#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// 日志级别枚举（严禁硬编码数字）
// 日志分为五种情况
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

//单列模式
class Logger : public QObject
{
    Q_OBJECT
public:
    // 单例模式（线程安全懒汉式）
    static Logger& instance();
    // 禁止拷贝和赋值
    Logger(const Logger&)=delete;
    Logger& operator=(const Logger&) = delete;

    // 初始化日志系统（配置日志目录、级别、文件大小限制）
    void init(const QString& logDir = "./logs",
              LogLevel minLevel = LogLevel::Info,
              qint64 maxFileSize = 10 * 1024 * 1024); // 10MB

    // 核心日志接口
    void log(LogLevel level, const QString& module, const QString& message);


// 便捷宏定义，是为了快捷使用log
// 便捷宏定义（在代码中使用 LOG_DEBUG("Module", "Message")）
#define LOG_DEBUG(module, msg) Logger::instance().log(LogLevel::Debug, module, msg)
#define LOG_INFO(module, msg) Logger::instance().log(LogLevel::Info, module, msg)
#define LOG_WARNING(module, msg) Logger::instance().log(LogLevel::Warning, module, msg)
#define LOG_ERROR(module, msg) Logger::instance().log(LogLevel::Error, module, msg)
#define LOG_FATAL(module, msg) Logger::instance().log(LogLevel::Fatal, module, msg)

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger() override;

    // 内部辅助函数
    QString levelToString(LogLevel level) const;
    void rotateLogFile();
    void flushLog();

private:
   QRecursiveMutex m_mutex;         // 线程安全锁
    QFile m_logFile;          // 日志文件句柄
    QTextStream m_logStream;  // 日志输出流
    QString m_logDir;         // 日志目录
    LogLevel m_minLevel;      // 最低输出级别
    qint64 m_maxFileSize;     // 单日志文件最大大小
    bool m_initialized;        // 初始化标志
};

#endif // LOGGER_H