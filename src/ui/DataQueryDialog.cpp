#include "DataQueryDialog.h"
#include "ui/ui_DataQueryDialog.h"
#include "logic/DeviceManager.h"
#include "db/MysqlManager.h"
#include <QVBoxLayout>
#include <QMessageBox>

DataQueryDialog::DataQueryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DataQueryDialog)
    , m_chartWidget(new ChartWidget(this))
{
    ui->setupUi(this);
    setWindowTitle("History Data Query");
    setMinimumSize(1000, 600);

    initUi();
    loadDevices();

    connect(ui->btnQuery, &QPushButton::clicked, this, &DataQueryDialog::onQueryClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &DataQueryDialog::onCloseClicked);
}

DataQueryDialog::~DataQueryDialog()
{
    delete ui;
}

void DataQueryDialog::initUi()
{
    // 设置默认时间范围：最近1小时
    ui->dtEnd->setDateTime(QDateTime::currentDateTime());
    ui->dtStart->setDateTime(ui->dtEnd->dateTime().addSecs(-3600));

    // 添加ChartWidget到布局
    QVBoxLayout* chartLayout = new QVBoxLayout(ui->widgetChart);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->addWidget(m_chartWidget);
    ui->widgetChart->setLayout(chartLayout);
}

void DataQueryDialog::loadDevices()
{
    ui->cbDevice->clear();
    QList<DeviceInfoPtr> devices = DeviceManager::instance().getAllDevices();
    for (DeviceInfoPtr device : devices) {
        ui->cbDevice->addItem(QString("%1 (ID: %2)").arg(device->deviceName).arg(device->deviceId), device->deviceId);
    }
}

void DataQueryDialog::onQueryClicked()
{
    int deviceId = ui->cbDevice->currentData().toInt();
    QDateTime startTime = ui->dtStart->dateTime();
    QDateTime endTime = ui->dtEnd->dateTime();

    if (deviceId == -1) {
        QMessageBox::warning(this, "Warning", "Please select a device");
        return;
    }

    if (startTime >= endTime) {
        QMessageBox::warning(this, "Warning", "Start time must be before end time");
        return;
    }

    // 查询历史数据
    QList<DataPointPtr> dataPoints = MysqlManager::instance().queryHistoryData(deviceId, startTime, endTime, 10000);

    // 更新曲线
    m_chartWidget->setDeviceId(deviceId);
    m_chartWidget->clearChart();
    m_chartWidget->setMaxDataPoints(10000);
    for (DataPointPtr dataPoint : dataPoints) {
        m_chartWidget->addDataPoint(dataPoint);
    }

    ui->lblResultCount->setText(QString("Query result: %1 data points").arg(dataPoints.size()));
}

void DataQueryDialog::onCloseClicked()
{
    accept();
}