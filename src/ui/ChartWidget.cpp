#include "ChartWidget.h"
#include <QVBoxLayout>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent),
    m_chart(new QChart()),
    m_chartView(new QChartView(m_chart)),
    m_series(new QLineSeries()),
    m_axisX(new QDateTimeAxis()),
    m_axisY(new QValueAxis()),
    m_deviceId(-1),
    m_maxDataPoints(100)
{
    initChart();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartView);
    setLayout(layout);
}

ChartWidget::~ChartWidget()
{
}

void ChartWidget::initChart()
{
    m_chart->legend()->hide();
    m_chart->addSeries(m_series);
    m_chart->setTitle("实时数据曲线");

    // 配置X轴（时间）
    m_axisX->setTitleText("时间");
    m_axisX->setFormat("hh:mm:ss");
    m_axisX->setTickCount(10);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    // 配置Y轴（数值）
    m_axisY->setTitleText("数值");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    // 抗锯齿
    m_chartView->setRenderHint(QPainter::Antialiasing);
}

void ChartWidget::setDeviceId(int deviceId)
{
    m_deviceId = deviceId;
    clearChart();
}

void ChartWidget::setMaxDataPoints(int count)
{
    m_maxDataPoints = count;
}

void ChartWidget::addDataPoint(const DataPointPtr &dataPoint)
{
    if (m_deviceId != -1 && dataPoint->deviceId != m_deviceId) {
        return;
    }

    QDateTime timestamp = dataPoint->timestamp;
    double value = dataPoint->value;

    // 添加到缓冲区
    m_dataBuffer.append(qMakePair(timestamp, value));

    // 限制缓冲区大小
    while (m_dataBuffer.size() > m_maxDataPoints) {
        m_dataBuffer.removeFirst();
    }

    // 更新曲线
    m_series->clear();
    for (const auto& pair : m_dataBuffer) {
        m_series->append(pair.first.toMSecsSinceEpoch(), pair.second);
    }

    updateAxis();
}

void ChartWidget::clearChart()
{
    m_dataBuffer.clear();
    m_series->clear();
    m_chart->setTitle(QString("实时数据曲线 - 设备 %1").arg(m_deviceId));
}

void ChartWidget::updateAxis()
{
    if (m_dataBuffer.isEmpty()) return;

    // 更新X轴范围
    QDateTime minTime = m_dataBuffer.first().first;
    QDateTime maxTime = m_dataBuffer.last().first;
    m_axisX->setRange(minTime, maxTime);

    // 更新Y轴范围
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::min();
    for (const auto& pair : m_dataBuffer) {
        minY = qMin(minY, pair.second);
        maxY = qMax(maxY, pair.second);
    }
    // 增加10%边距
    double margin = (maxY - minY) * 0.1;
    m_axisY->setRange(minY - margin, maxY + margin);
}