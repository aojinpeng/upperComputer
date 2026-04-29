#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QTimer>
#include "../common/Defines.h"

QT_CHARTS_USE_NAMESPACE

    class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

    // 配置接口
    void setDeviceId(int deviceId);
    void setMaxDataPoints(int count); // 最大显示数据点数

public slots:
    void addDataPoint(const DataPointPtr& dataPoint);
    void clearChart();

private:
    void initChart();
    void updateAxis();

private:
    QChart* m_chart;
    QChartView* m_chartView;
    QLineSeries* m_series;
    QDateTimeAxis* m_axisX;
    QValueAxis* m_axisY;
    int m_deviceId;
    int m_maxDataPoints;
    QList<QPair<QDateTime, double>> m_dataBuffer;
};

#endif // CHARTWIDGET_H
