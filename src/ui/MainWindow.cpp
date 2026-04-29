#include "MainWindow.h"
#include "ui/ui_MainWindow.h"
#include "ui/LoginDialog.h"
#include "common/Logger.h"
#include "common/Config.h"
#include "common/Utils.h"
#include "logic/DeviceManager.h"
#include "logic/CollectManager.h"
#include "logic/AlarmManager.h"
#include "db/MysqlManager.h"
#include "db/SqliteManager.h"
#include "comm/TcpServer.h"
#include "comm/HttpClient.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QSplitter>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mysqlConnected(false)
    , m_devicePanel(nullptr)
    , m_chartWidget(nullptr)
    , m_alarmDialog(nullptr)
    , m_dataQueryDialog(nullptr)
    , m_configDialog(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Scada v1.0.0");
    setMinimumSize(1400, 400);

    // 1. 显示登录对话框
    LoginDialog loginDialog(this);
    if (loginDialog.exec() != QDialog::Accepted) {
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }
    m_currentUser = loginDialog.getLoggedInUser();


    // 2. 初始化工具层
    Logger::instance().init(QString("%1/logs").arg(PROJECT_SOURCE_DIR), LogLevel::Debug);
    Config::instance().load(QString("%1/config/config.json").arg(PROJECT_SOURCE_DIR));

    // 3. 初始化数据层
    initDataLayer();

    // 4. 初始化UI
    initUi();
    initMenuBar();
    initCentralWidget();

    // 5. 初始化业务层
    initBusinessLayer();

    // 6. 初始化通信层
    initCommunicationLayer();

    // 7.初始化http上传python云端测试
    initPythonHttpTest();

    showStatusBarMessage("SCADA System started successfully");
    LOG_INFO("MainWindow", "SCADA system fully initialized and started");
    showStatusBarMessage("SCADA System started successfully");
    LOG_INFO("MainWindow", "SCADA system fully initialized and started");
}

MainWindow::~MainWindow()
{
    LOG_INFO("MainWindow", "SCADA system shutting down...");
    delete ui;
}

void MainWindow::initUi()
{
    // 初始化对话框
    m_alarmDialog = new AlarmDialog(this);
    m_dataQueryDialog = new DataQueryDialog(this);
    m_configDialog = new ConfigDialog(this);
}

void MainWindow::initMenuBar()
{
    QMenuBar* menuBar = this->menuBar();

    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu("&文件");
    QAction* actionConfig = fileMenu->addAction("&配置...");
    actionConfig->setShortcut(QKeySequence("Ctrl+Shift+C"));
    // 仅管理员可见
    if (m_currentUser.role != UserRole::Administrator) {
        actionConfig->setVisible(false);
    }
    connect(actionConfig, &QAction::triggered, this, &MainWindow::onActionConfigTriggered);

    fileMenu->addSeparator();
    QAction* actionExit = fileMenu->addAction("退出");
    actionExit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(actionExit, &QAction::triggered, this, &MainWindow::onActionExitTriggered);

    // 视图菜单
    QMenu* viewMenu = menuBar->addMenu("&视图");
    QAction* actionAlarm = viewMenu->addAction("&报警提示...");
    actionAlarm->setShortcut(QKeySequence("Ctrl+A"));
    connect(actionAlarm, &QAction::triggered, this, &MainWindow::onActionAlarmTriggered);

    QAction* actionDataQuery = viewMenu->addAction("&数据查询...");
    actionDataQuery->setShortcut(QKeySequence("Ctrl+D"));
    connect(actionDataQuery, &QAction::triggered, this, &MainWindow::onActionDataQueryTriggered);

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu("&帮助");
    QAction* actionAbout = helpMenu->addAction("&关于");
    connect(actionAbout, &QAction::triggered, this, &MainWindow::onActionAboutTriggered);
}

void MainWindow::initCentralWidget()
{
    // 使用QSplitter分割设备面板和曲线
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧：设备面板
    m_devicePanel = new DevicePanelWidget(this);
    connect(m_devicePanel, &DevicePanelWidget::sigShowChart, this, &MainWindow::onShowChart);
    splitter->addWidget(m_devicePanel);

    // 右侧：历史曲线
    m_chartWidget = new ChartWidget(this);
    m_chartWidget->setDeviceId(1); // 默认显示设备1
    splitter->addWidget(m_chartWidget);

    // 设置分割比例
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
}

void MainWindow::initDataLayer()
{
    // 初始化SQLite
    if (!SqliteManager::instance().init()) {
        QMessageBox::critical(this, "Error", "Failed to initialize local SQLite database");
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }

    // 初始化MySQL
    connect(&MysqlManager::instance(), &MysqlManager::sigConnectionLost, this, &MainWindow::onMysqlConnectionLost);
    connect(&MysqlManager::instance(), &MysqlManager::sigConnectionRestored, this, &MainWindow::onMysqlConnectionRestored);

    if (!MysqlManager::instance().init(
            Config::instance().mysqlHost(),
            Config::instance().mysqlPort(),
            Config::instance().mysqlUser(),
            Config::instance().mysqlPassword(),
            Config::instance().mysqlDatabase()
            )) {
        QMessageBox::warning(this, "Warning", "Failed to connect to remote MySQL database, using local cache only");
        m_mysqlConnected = false;
        showStatusBarMessage("MySQL connection failed, using local cache");
    } else {
        m_mysqlConnected = true;
        qDebug()<<Config::instance().deviceList();
        MysqlManager::instance().syncDeviceFromConfig(Config::instance().deviceList());
        showStatusBarMessage("MySQL connected successfully");
    }
}

void MainWindow::initBusinessLayer()
{
    // 初始化设备管理器
    DeviceManager::instance().loadDevicesFromConfig();
    m_devicePanel->refreshDevices();

    // 初始化报警管理器
    connect(&AlarmManager::instance(), &AlarmManager::sigNewAlarm, this, &MainWindow::onNewAlarm);
    connect(&AlarmManager::instance(), &AlarmManager::sigAlarmEnded, this, [this](qint64 alarmId) {
        Q_UNUSED(alarmId);
        m_alarmDialog->refreshAlarms();
    });

    // 初始化采集管理器（多线程）
    CollectManager* collectManager = new CollectManager(this);
    connect(collectManager, &CollectManager::sigDataCollected, this, &MainWindow::onDataCollected);
    connect(collectManager, &CollectManager::sigDeviceStatusChanged, this, &MainWindow::onDeviceStatusChanged);

    collectManager->start();
}

void MainWindow::initCommunicationLayer()
{
    // 初始化TCP服务端
    TcpServer* tcpServer = new TcpServer(this);
    if (tcpServer->start(12345)) {
        showStatusBarMessage("TCP Server started on port 12345");
    }

    // 连接采集信号到TCP服务端
      CollectManager* collectManager = new CollectManager(this);
    connect(collectManager, &CollectManager::sigDataCollected, tcpServer, &TcpServer::broadcastData);
}
void MainWindow::initPythonHttpTest(){
    // 【新增7. 初始化HTTP上传客户端】
    m_httpClient = new HttpClient(this); // 绑定父对象，自动管理内存
    // 绑定上传结果回调（看日志）
    connect(m_httpClient, &HttpClient::sigUploadSuccess, this, [this](const QString& response) {
        LOG_INFO("MainWindow", QString("✅ 云端上传成功：%1").arg(response));
    });
    connect(m_httpClient, &HttpClient::sigUploadFailed, this, [this](const QString& error) {
        LOG_WARNING("MainWindow", QString("❌ 云端上传失败：%1").arg(error));
    });

}
void MainWindow::showStatusBarMessage(const QString &message, int timeout)
{
    statusBar()->showMessage(message, timeout);
}

// ------------------------------ 槽函数实现 ------------------------------
void MainWindow::onActionConfigTriggered()
{
    m_configDialog->exec();
    m_devicePanel->refreshDevices();
}

void MainWindow::onActionAlarmTriggered()
{
    m_alarmDialog->refreshAlarms();
    m_alarmDialog->exec();
}

void MainWindow::onActionDataQueryTriggered()
{
    m_dataQueryDialog->exec();
}

void MainWindow::onActionExitTriggered()
{
    qApp->quit();
}

void MainWindow::onActionAboutTriggered()
{
    QMessageBox::about(this, "About",
                       "Qt5.15.2 Modbus TCP SCADA System v1.0.0\n\n"
                       "A cross-platform industrial SCADA system for Modbus TCP devices.\n"
                       "Features: Multi-device monitoring, data logging, alarm management, Qt Charts.\n\n"
                       "Developed with Qt 5.15.2, C++17, CMake, MySQL, SQLite.");
}

void MainWindow::onDataCollected(DataPointPtr dataPoint)
{
    // 1. 更新曲线
    m_chartWidget->addDataPoint(dataPoint);

    // 2. 更新设备面板数据
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(dataPoint->deviceId);
    if (device) {
        QMutexLocker locker(&device->mutex);
        m_devicePanel->updateDeviceData(dataPoint->deviceId, device->rawData, device->lastUpdateTime);
    }

    // 3. 处理报警
    if (device) {
        AlarmManager::instance().processDataPoint(dataPoint, device);
    }

    // 4. 存储数据
    if (m_mysqlConnected) {
        MysqlManager::instance().insertHistoryData(dataPoint, 0, 0.1, 0.0, "");
    } else {
        SqliteManager::instance().cacheDataPoint(dataPoint, QVector<quint16>());
    }
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastUploadTime.msecsTo(now) > 1000) { // 距离上次上传超过1秒
        QString cloudUrl = "http://127.0.0.1:8080/upload";
        m_httpClient->uploadDataPoint(cloudUrl, dataPoint);
        m_lastUploadTime = now;
    }
}

void MainWindow::onDeviceStatusChanged(int deviceId, DeviceStatus status)
{
    DeviceInfoPtr device = DeviceManager::instance().getDeviceById(deviceId);
    if (!device) return;

    m_devicePanel->updateDeviceStatus(deviceId, status);

    QString statusStr;
    switch (status) {
    case DeviceStatus::Online:
        statusStr = "Online";
        AlarmManager::instance().processDeviceOnline(deviceId);
        break;
    case DeviceStatus::Offline:
        statusStr = "Offline";
        AlarmManager::instance().processDeviceOffline(deviceId);
        break;
    case DeviceStatus::Connecting:
        statusStr = "Connecting";
        break;
    case DeviceStatus::Error:
        statusStr = "Error";
        break;
    }

    showStatusBarMessage(QString("Device %1: %2").arg(device->deviceName, statusStr));
    m_alarmDialog->refreshAlarms();
}

void MainWindow::onNewAlarm(qint64 alarmId, DataPointPtr dataPoint)
{
    Q_UNUSED(alarmId);

    // 1. 空指针保护（防止崩溃）
    if (!dataPoint) {
        return;
    }

    // 2. 【工业核心】如果弹窗已经存在，直接更新文本，不新建！（杜绝堆叠卡死）
    if (m_alarmMsgBox) {
        m_alarmMsgBox->setText(QString("新报警触发！\n设备：%1\n信息：%2")
                                   .arg(dataPoint->deviceId)
                                   .arg(dataPoint->alarmMsg));
        return;
    }

    // 3. 创建【非阻塞弹窗】（关键：用 new 创建 + show() 显示）
    m_alarmMsgBox = new QMessageBox(this);
    m_alarmMsgBox->setIcon(QMessageBox::Warning);       // 警告图标
    m_alarmMsgBox->setWindowTitle("设备报警");          // 标题
    m_alarmMsgBox->setWindowModality(Qt::NonModal);     // ✅ 非阻塞（核心）
    m_alarmMsgBox->setAttribute(Qt::WA_DeleteOnClose);  // ✅ 关闭后自动释放内存

    // 4. 设置报警内容（你的原始信息）
    QString alarmText = QString(
                            "新报警发生！\n\n"
                            "设备ID：%1\n"
                            "数据标签：%2\n"
                            "当前值：%3\n"
                            "报警信息：%4\n"
                            "时间：%5"
                            ).arg(dataPoint->deviceId)
                            .arg(dataPoint->tagName)
                            .arg(dataPoint->value)
                            .arg(dataPoint->alarmMsg)
                            .arg(dataPoint->timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"));

    m_alarmMsgBox->setText(alarmText);

    // 5. 弹窗关闭时，清空指针（防止下次无法创建）
    connect(m_alarmMsgBox, &QMessageBox::finished, this, [this]() {
        m_alarmMsgBox = nullptr;
    });

    // 6. ✅ 关键：用 show() 不是 exec() → 绝不卡死主线程
    m_alarmMsgBox->show();

    // 7. 正常刷新报警列表（无阻塞）
    if (m_alarmDialog) {
        m_alarmDialog->refreshAlarms();
    }
}

void MainWindow::onShowChart(int deviceId)
{
    m_chartWidget->setDeviceId(deviceId);
    m_chartWidget->clearChart();
    showStatusBarMessage(QString("Showing chart for device %1").arg(deviceId));
}

void MainWindow::onMysqlConnectionLost()
{
    m_mysqlConnected = false;
    QMessageBox::warning(this, "Warning", "Remote MySQL database connection lost, using local cache only");
    showStatusBarMessage("MySQL connection lost, using local cache");
    LOG_ERROR("MainWindow", "MySQL connection lost");
}

void MainWindow::onMysqlConnectionRestored()
{
    m_mysqlConnected = true;
    QMessageBox::information(this, "Info", "Remote MySQL database connection restored");
    showStatusBarMessage("MySQL connection restored");
    LOG_INFO("MainWindow", "MySQL connection restored");

    // 上传缓存数据
    QList<QPair<DataPointPtr, QVector<quint16>>> cachedData = SqliteManager::instance().getCachedDataPoints();
    if (!cachedData.isEmpty()) {
        if (MysqlManager::instance().insertHistoryDataBatch(cachedData)) {
            // 简化：暂不删除缓存
        }
    }
    QTimer::singleShot(100,this,[this](){
         MysqlManager::instance().syncDeviceFromConfig(Config::instance().deviceList());
    });


}