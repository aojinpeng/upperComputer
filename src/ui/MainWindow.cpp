#include "MainWindow.h"
#include "ui/ui_MainWindow.h"
#include "common/Logger.h"
#include "common/Config.h"
#include "logic/DeviceManager.h"
#include "logic/CollectManager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Qt5.15.2 Modbus TCP SCADA System v1.0.0");

    // 1. 初始化工具层
    Logger::instance().init("./logs", LogLevel::Debug);
    Config::instance().load("./config/config.json");

    // 2. 初始化设备管理器
    DeviceManager::instance().loadDevicesFromConfig();

    // 3. 初始化采集管理器（多线程）
    CollectManager* collectManager = new CollectManager(this);

    // 连接采集信号（跨线程，Qt::QueuedConnection自动处理）
    connect(collectManager, &CollectManager::sigDataCollected, this, [this](DataPointPtr dataPoint) {
        // UI层数据更新（示例：打印日志）
        QString alarmStr = dataPoint->isAlarm ? QString(" [ALARM: %1]").arg(dataPoint->alarmMsg) : "";
        LOG_DEBUG("MainWindow", QString("Data received: %1 = %2%3")
                                    .arg(dataPoint->tagName).arg(dataPoint->value).arg(alarmStr));
    });

    connect(collectManager, &CollectManager::sigDeviceStatusChanged, this, [this](int deviceId, DeviceStatus status) {
        DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
        if (device) {
            QString statusStr;
            switch (status) {
            case DeviceStatus::Online: statusStr = "Online"; break;
            case DeviceStatus::Offline: statusStr = "Offline"; break;
            case DeviceStatus::Connecting: statusStr = "Connecting"; break;
            case DeviceStatus::Error: statusStr = "Error"; break;
            }
            LOG_INFO("MainWindow", QString("Device %1 status changed to: %2").arg(device->deviceName, statusStr));
        }
    });

    // 4. 启动采集
    collectManager->start();

    LOG_INFO("MainWindow", "MainWindow initialized, SCADA system started");
}

MainWindow::~MainWindow()
{
    LOG_INFO("MainWindow", "MainWindow destroyed");
    delete ui;
}