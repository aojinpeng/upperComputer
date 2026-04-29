#include "ConfigDialog.h"
#include "ui/ui_ConfigDialog.h"
#include "common/Config.h"
#include "common/Logger.h"
#include "DeviceEditDialog.h"   // 新增的设备编辑对话框
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

ConfigDialog::ConfigDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    setWindowTitle("系统配置");
    setMinimumSize(800, 600);

    initUi();
    loadConfigToUi();

    connect(ui->btnSave, &QPushButton::clicked, this, &ConfigDialog::onSaveClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &ConfigDialog::onCancelClicked);
    connect(ui->btnAddDevice, &QPushButton::clicked, this, &ConfigDialog::onAddDeviceClicked);
    connect(ui->btnEditDevice, &QPushButton::clicked, this, &ConfigDialog::onEditDeviceClicked);
    connect(ui->btnDeleteDevice, &QPushButton::clicked, this, &ConfigDialog::onDeleteDeviceClicked);
    connect(ui->tableDevices, &QTableWidget::itemSelectionChanged, this, &ConfigDialog::onDeviceSelectionChanged);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::initUi()
{
    ui->tableDevices->setColumnCount(5);
    ui->tableDevices->setHorizontalHeaderLabels({"ID", "Name", "IP", "Port", "Status"});
    ui->tableDevices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableDevices->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableDevices->horizontalHeader()->setStretchLastSection(true);
}

void ConfigDialog::loadConfigToUi()
{
    // 检查配置是否已加载，若未加载则提示用户
    if (!Config::instance().isLoaded()) {
        QMessageBox::warning(this, "警告", "配置文件未成功加载，将使用默认值。\n请检查配置文件路径或格式。");
    }

    // MySQL配置
    ui->leMysqlHost->setText(Config::instance().mysqlHost());
    ui->sbMysqlPort->setValue(Config::instance().mysqlPort());
    ui->leMysqlUser->setText(Config::instance().mysqlUser());
    ui->leMysqlPassword->setText(Config::instance().mysqlPassword());
    ui->leMysqlDatabase->setText(Config::instance().mysqlDatabase());

    // 采集配置
    ui->sbCollectInterval->setValue(Config::instance().collectIntervalMs());
    ui->sbHeartbeatInterval->setValue(Config::instance().heartbeatIntervalMs());
    ui->sbReconnectInterval->setValue(Config::instance().reconnectIntervalMs());

    // 设备列表
    m_editableDeviceArray = Config::instance().deviceList();
    refreshDeviceTable();
}

void ConfigDialog::refreshDeviceTable()
{
    ui->tableDevices->setRowCount(0);
    for (int i = 0; i < m_editableDeviceArray.size(); ++i) {
        QJsonObject obj = m_editableDeviceArray[i].toObject();
        int row = ui->tableDevices->rowCount();
        ui->tableDevices->insertRow(row);
        ui->tableDevices->setItem(row, 0, new QTableWidgetItem(QString::number(obj["device_id"].toInt())));
        ui->tableDevices->setItem(row, 1, new QTableWidgetItem(obj["device_name"].toString()));
        ui->tableDevices->setItem(row, 2, new QTableWidgetItem(obj["ip"].toString()));
        ui->tableDevices->setItem(row, 3, new QTableWidgetItem(QString::number(obj["port"].toInt())));
        // 状态列：暂时显示 "Configured"（实际状态可由采集模块动态更新）
        ui->tableDevices->setItem(row, 4, new QTableWidgetItem("Configured"));
    }
    // 刷新后更新按钮状态
    onDeviceSelectionChanged();
}

void ConfigDialog::onSaveClicked()
{
    // 获取当前配置完整副本
    QJsonObject root = Config::instance().configRoot();

    // 更新 MySQL
    QJsonObject mysql;
    mysql["host"] = ui->leMysqlHost->text();
    mysql["port"] = ui->sbMysqlPort->value();
    mysql["user"] = ui->leMysqlUser->text();
    mysql["password"] = ui->leMysqlPassword->text();
    mysql["database"] = ui->leMysqlDatabase->text();
    root["mysql"] = mysql;

    // 更新采集
    QJsonObject collect;
    collect["interval_ms"] = ui->sbCollectInterval->value();
    collect["heartbeat_ms"] = ui->sbHeartbeatInterval->value();
    collect["reconnect_ms"] = ui->sbReconnectInterval->value();
    root["collect"] = collect;

    // 更新设备列表
    root["devices"] = m_editableDeviceArray;

    // 保存配置
    if (Config::instance().saveConfig(root)) {
        QMessageBox::information(this, "成功", "配置已保存");
        // 【关键修复】延迟关闭对话框，避免在信号处理过程中销毁
        QMetaObject::invokeMethod(this, "accept", Qt::QueuedConnection);
    } else {
        QMessageBox::critical(this, "错误", "保存配置失败，请检查日志权限");
    }
}

void ConfigDialog::onCancelClicked()
{
    reject();
}

void ConfigDialog::onAddDeviceClicked()
{
    // 计算当前最大 device_id
    int maxId = 0;
    for (const auto& item : m_editableDeviceArray) {
        int id = item.toObject()["device_id"].toInt();
        if (id > maxId) maxId = id;
    }

    // 创建具有完整字段的默认设备对象
    QJsonObject newDevice;
    newDevice["device_id"] = maxId + 1;
    newDevice["device_name"] = "新设备";
    newDevice["ip"] = "192.168.1.100";
    newDevice["port"] = 502;
    newDevice["slave_id"] = 1;
    newDevice["start_addr"] = 0;
    newDevice["reg_count"] = 10;
    newDevice["alarm_min"] = 0.0;
    newDevice["alarm_max"] = 100.0;
    newDevice["scale_factor"] = 0.1;
    newDevice["offset"] = 0.0;
    newDevice["unit"] = "";

    // 打开编辑对话框
    DeviceEditDialog dlg(this);
    dlg.setDevice(newDevice);
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject editedDevice = dlg.getDevice();
        m_editableDeviceArray.append(editedDevice);
        refreshDeviceTable();
    }
}

void ConfigDialog::onEditDeviceClicked()
{
    int currentRow = ui->tableDevices->currentRow();
    if (currentRow < 0 || currentRow >= m_editableDeviceArray.size()) {
        return;
    }

    QJsonObject device = m_editableDeviceArray[currentRow].toObject();
    DeviceEditDialog dlg(this);
    dlg.setDevice(device);
    if (dlg.exec() == QDialog::Accepted) {
        QJsonObject editedDevice = dlg.getDevice();
        // 保留原有 device_id（不允许修改ID）
        editedDevice["device_id"] = device["device_id"];
        m_editableDeviceArray[currentRow] = editedDevice;
        refreshDeviceTable();
    }
}

void ConfigDialog::onDeleteDeviceClicked()
{
    int currentRow = ui->tableDevices->currentRow();
    if (currentRow >= 0 && currentRow < m_editableDeviceArray.size()) {
        if (QMessageBox::question(this, "确认删除", "确定要删除该设备吗？",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            m_editableDeviceArray.removeAt(currentRow);
            refreshDeviceTable();
        }
    }
}

void ConfigDialog::onDeviceSelectionChanged()
{
    bool hasSelection = ui->tableDevices->currentRow() >= 0;
    ui->btnEditDevice->setEnabled(hasSelection);
    ui->btnDeleteDevice->setEnabled(hasSelection);
}