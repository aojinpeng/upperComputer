#include "MainWindow.h"
#include "Logger.h"
#include "ui/ui_MainWindow.h"
#include "common/Logger.h"
#include "common/Config.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化窗口标题（带版本号）
    setWindowTitle("Qt5.15.2 Modbus TCP SCADA System v1.0.0");


}

// 窗口显示完成后，再初始化日志和配置
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // 🔥 窗口显示出来后，再初始化！
    Logger::instance().init("./logs", LogLevel::Debug);
    Config::instance().load("./config/config.json");
    LOG_INFO("MainWindow", "MainWindow initialized");
}

MainWindow::~MainWindow()
{
    // LOG_INFO("MainWindow", "MainWindow destroyed");
    delete ui;
}