#ifndef DEVICEPANELWIDGET_H
#define DEVICEPANELWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include "../common/Defines.h"
#include <QMap>

class DeviceCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceCardWidget(DeviceInfoPtr device, QWidget *parent = nullptr);
    void updateStatus(DeviceStatus status);
    void updateData(const QVector<quint16>& rawData, const QDateTime& timestamp);

signals:
    void sigShowChart(int deviceId);

private slots:
    void onShowChartClicked();

private:
    void initUi();


private:
    DeviceInfoPtr m_device;
    QLabel* m_lblStatus;
    QLabel* m_lblName;
    QLabel* m_lblData;
    QLabel* m_lblTime;
    QPushButton* m_btnChart;
};

class DevicePanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DevicePanelWidget(QWidget *parent = nullptr);
    ~DevicePanelWidget();

public slots:
    void refreshDevices();
    void updateDeviceStatus(int deviceId, DeviceStatus status);
    void updateDeviceData(int deviceId, const QVector<quint16>& rawData, const QDateTime& timestamp);

signals:
    void sigShowChart(int deviceId);

private:
    void initUi();
    void clearCards();
    void resizeEvent(QResizeEvent* event) override;

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QGridLayout* m_gridLayout;
    QMap<int, DeviceCardWidget*> m_deviceCards;
};

#endif // DEVICEPANELWIDGET_H
