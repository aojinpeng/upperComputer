#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include "../src/comm/HttpClient.h"
#include "ui/ui_LoginDialog.h"
#include "../common/Defines.h"
#include "../ui/LoginDialog.h"
#include "../ui/ChartWidget.h"
#include "../ui/DevicePanelWidget.h"
#include "../ui/AlarmDialog.h"
#include "../ui/DataQueryDialog.h"
#include "../ui/ConfigDialog.h"
#include "../logic/CollectManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 菜单槽函数
    void onActionConfigTriggered();
    void onActionAlarmTriggered();
    void onActionDataQueryTriggered();
    void onActionExitTriggered();
    void onActionAboutTriggered();

    // 业务槽函数
    void onDataCollected(DataPointPtr dataPoint);
    void onDeviceStatusChanged(int deviceId, DeviceStatus status);
    void onNewAlarm(qint64 alarmId, DataPointPtr dataPoint);
    void onShowChart(int deviceId);
    void onMysqlConnectionLost();
    void onMysqlConnectionRestored();

private:
    void initUi();
    void initMenuBar();
    void initCentralWidget();
    void initDataLayer();
    void initBusinessLayer();
    void initCommunicationLayer();
    void initPythonHttpTest();
    void showStatusBarMessage(const QString& message, int timeout = 3000);

private:
    Ui::MainWindow *ui;
    UserInfo m_currentUser;
    bool m_mysqlConnected;
    QMessageBox* m_alarmMsgBox = nullptr;
    CollectManager* m_collectManager;
    // UI组件
    DevicePanelWidget* m_devicePanel;
    ChartWidget* m_chartWidget;
    AlarmDialog* m_alarmDialog;
    DataQueryDialog* m_dataQueryDialog;
    ConfigDialog* m_configDialog;
     HttpClient* m_httpClient; // 【新增】HTTP上传客户端
    QDateTime m_lastUploadTime; // 上次上传时间
};

#endif // MAINWINDOW_H