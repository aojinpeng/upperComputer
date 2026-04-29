#include "AlarmDialog.h"
#include "ui/ui_AlarmDialog.h"
#include "logic/AlarmManager.h"
#include "db/MysqlManager.h"
#include <QMessageBox>

AlarmDialog::AlarmDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AlarmDialog)
{
    ui->setupUi(this);
    setWindowTitle("Alarm Management");
    setMinimumSize(800, 500);

    initUi();
    refreshAlarms();

    connect(ui->btnConfirm, &QPushButton::clicked, this, &AlarmDialog::onConfirmClicked);
    connect(ui->btnConfirmAll, &QPushButton::clicked, this, &AlarmDialog::onConfirmAllClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &AlarmDialog::onCloseClicked);
}

AlarmDialog::~AlarmDialog()
{
    delete ui;
}

void AlarmDialog::initUi()
{
    ui->tableAlarms->setColumnCount(6);
    ui->tableAlarms->setHorizontalHeaderLabels({"ID", "Device", "Tag", "Value", "Message", "Time"});
    ui->tableAlarms->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableAlarms->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableAlarms->horizontalHeader()->setStretchLastSection(true);
}

void AlarmDialog::refreshAlarms()
{
    m_currentAlarms = MysqlManager::instance().queryUnconfirmedAlarms(100);
    ui->tableAlarms->setRowCount(0);

    for (const auto& pair : m_currentAlarms) {
        qint64 alarmId = pair.first;
        DataPointPtr dataPoint = pair.second;
        int row = ui->tableAlarms->rowCount();
        ui->tableAlarms->insertRow(row);
        ui->tableAlarms->setItem(row, 0, new QTableWidgetItem(QString::number(alarmId)));
        ui->tableAlarms->setItem(row, 1, new QTableWidgetItem(QString::number(dataPoint->deviceId)));
        ui->tableAlarms->setItem(row, 2, new QTableWidgetItem(dataPoint->tagName));
        ui->tableAlarms->setItem(row, 3, new QTableWidgetItem(QString::number(dataPoint->value)));
        ui->tableAlarms->setItem(row, 4, new QTableWidgetItem(dataPoint->alarmMsg));
        ui->tableAlarms->setItem(row, 5, new QTableWidgetItem(dataPoint->timestamp.toString("yyyy-MM-dd hh:mm:ss")));
    }

    ui->lblAlarmCount->setText(QString("Unconfirmed Alarms: %1").arg(m_currentAlarms.size()));
}

void AlarmDialog::onConfirmClicked()
{
    int currentRow = ui->tableAlarms->currentRow();
    if (currentRow >= 0 && currentRow < m_currentAlarms.size()) {
        qint64 alarmId = m_currentAlarms[currentRow].first;
        if (AlarmManager::instance().confirmAlarm(alarmId, "Admin", "Confirmed via UI")) {
            QMessageBox::information(this, "Success", "Alarm confirmed");
            refreshAlarms();
        }
    }
}

void AlarmDialog::onConfirmAllClicked()
{
    for (const auto& pair : m_currentAlarms) {
        AlarmManager::instance().confirmAlarm(pair.first, "Admin", "Batch confirmed");
    }
    QMessageBox::information(this, "Success", "All alarms confirmed");
    refreshAlarms();
}

void AlarmDialog::onCloseClicked()
{
    accept();
}