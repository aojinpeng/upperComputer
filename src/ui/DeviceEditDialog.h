#ifndef DEVICEEDITDIALOG_H
#define DEVICEEDITDIALOG_H

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;

class DeviceEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceEditDialog(QWidget* parent = nullptr);
    void setDevice(const QJsonObject& device);
    QJsonObject getDevice() const;

private slots:
    void onAccepted();

private:
    void initUi();

    QLineEdit* m_leName;
    QLineEdit* m_leIp;
    QSpinBox* m_sbPort;
    QSpinBox* m_sbSlaveId;
    QSpinBox* m_sbStartAddr;
    QSpinBox* m_sbRegCount;
    QDoubleSpinBox* m_dbAlarmMin;
    QDoubleSpinBox* m_dbAlarmMax;
    QDoubleSpinBox* m_dbScale;
    QDoubleSpinBox* m_dbOffset;
    QLineEdit* m_leUnit;

    QJsonObject m_device;
};

#endif // DEVICEEDITDIALOG_H