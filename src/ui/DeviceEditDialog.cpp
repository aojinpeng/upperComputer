#include "DeviceEditDialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>

DeviceEditDialog::DeviceEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("编辑设备");
    initUi();
}

void DeviceEditDialog::initUi()
{
    QFormLayout* layout = new QFormLayout(this);

    m_leName = new QLineEdit(this);
    m_leIp = new QLineEdit(this);
    m_sbPort = new QSpinBox(this);
    m_sbPort->setRange(1, 65535);
    m_sbSlaveId = new QSpinBox(this);
    m_sbSlaveId->setRange(1, 255);
    m_sbStartAddr = new QSpinBox(this);
    m_sbStartAddr->setRange(0, 65535);
    m_sbRegCount = new QSpinBox(this);
    m_sbRegCount->setRange(1, 125);
    m_dbAlarmMin = new QDoubleSpinBox(this);
    m_dbAlarmMin->setRange(-1e6, 1e6);
    m_dbAlarmMax = new QDoubleSpinBox(this);
    m_dbAlarmMax->setRange(-1e6, 1e6);
    m_dbScale = new QDoubleSpinBox(this);
    m_dbScale->setRange(-1e6, 1e6);
    m_dbScale->setSingleStep(0.1);
    m_dbOffset = new QDoubleSpinBox(this);
    m_dbOffset->setRange(-1e6, 1e6);
    m_leUnit = new QLineEdit(this);

    layout->addRow("设备名称:", m_leName);
    layout->addRow("IP地址:", m_leIp);
    layout->addRow("端口:", m_sbPort);
    layout->addRow("从站ID:", m_sbSlaveId);
    layout->addRow("起始地址:", m_sbStartAddr);
    layout->addRow("寄存器数量:", m_sbRegCount);
    layout->addRow("报警下限:", m_dbAlarmMin);
    layout->addRow("报警上限:", m_dbAlarmMax);
    layout->addRow("缩放因子:", m_dbScale);
    layout->addRow("偏移量:", m_dbOffset);
    layout->addRow("单位:", m_leUnit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DeviceEditDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttonBox);
}

void DeviceEditDialog::setDevice(const QJsonObject& device)
{
    m_device = device;
    m_leName->setText(device["device_name"].toString());
    m_leIp->setText(device["ip"].toString());
    m_sbPort->setValue(device["port"].toInt(502));
    m_sbSlaveId->setValue(device["slave_id"].toInt(1));
    m_sbStartAddr->setValue(device["start_addr"].toInt(0));
    m_sbRegCount->setValue(device["reg_count"].toInt(10));
    m_dbAlarmMin->setValue(device["alarm_min"].toDouble(0.0));
    m_dbAlarmMax->setValue(device["alarm_max"].toDouble(100.0));
    m_dbScale->setValue(device["scale_factor"].toDouble(0.1));
    m_dbOffset->setValue(device["offset"].toDouble(0.0));
    m_leUnit->setText(device["unit"].toString());
}

QJsonObject DeviceEditDialog::getDevice() const
{
    QJsonObject result = m_device;  // 保留原有的 device_id
    result["device_name"] = m_leName->text();
    result["ip"] = m_leIp->text();
    result["port"] = m_sbPort->value();
    result["slave_id"] = m_sbSlaveId->value();
    result["start_addr"] = m_sbStartAddr->value();
    result["reg_count"] = m_sbRegCount->value();
    result["alarm_min"] = m_dbAlarmMin->value();
    result["alarm_max"] = m_dbAlarmMax->value();
    result["scale_factor"] = m_dbScale->value();
    result["offset"] = m_dbOffset->value();
    result["unit"] = m_leUnit->text();
    return result;
}

void DeviceEditDialog::onAccepted()
{
    // 可在此添加输入验证
    if (m_leName->text().isEmpty()) {
        // 简单验证，可根据需要扩展
        return;
    }
    accept();
}