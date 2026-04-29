#include "MainWindow.h"
#include <QApplication>
#include "common/Logger.h"

int main(int argc, char *argv[])
{
    // 高DPI支持（工业场景常用高分屏）
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    // 设置应用程序信息（用于QSettings等）
    QApplication::setOrganizationName("IndustrialSCADA");
    QApplication::setApplicationName("ModbusTcpScada");
    if (!Config::instance().load()) {
        qWarning() << "Failed to load config, using defaults";
    }
    MainWindow w;
    w.show();
    int ret = a.exec();

    // 退出前清理日志
    LOG_INFO("Main", "Application exiting with code:" + QString::number(ret));
    return ret;
}