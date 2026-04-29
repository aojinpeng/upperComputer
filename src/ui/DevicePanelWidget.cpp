#include "DevicePanelWidget.h"
#include "logic/DeviceManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

// ------------------------------ DeviceCardWidget ------------------------------
DeviceCardWidget::DeviceCardWidget(DeviceInfoPtr device, QWidget *parent)
    : QWidget(parent),
    m_device(device)
{
    initUi();
    updateStatus(m_device->status);
}

void DeviceCardWidget::initUi()
{
    setFixedSize(160, 180);
    setStyleSheet("QWidget { background-color: #f0f0f0; border-radius: 8px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // 顶部：状态 + 名称
    QHBoxLayout* topLayout = new QHBoxLayout();
    m_lblStatus = new QLabel("●");
    m_lblStatus->setStyleSheet("font-size: 24px;");
    m_lblName = new QLabel(m_device->deviceName);
    m_lblName->setStyleSheet("font-size: 16px; font-weight: bold;");
    topLayout->addWidget(m_lblStatus);
    topLayout->addWidget(m_lblName, 1);
    mainLayout->addLayout(topLayout);

    // 分隔线
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #cccccc;");
    mainLayout->addWidget(line);

    // 中间：数据
    m_lblData = new QLabel("Data: --");
    m_lblData->setStyleSheet("font-size: 14px;");
    mainLayout->addWidget(m_lblData);

    // 底部：时间 + 按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    m_lblTime = new QLabel("Last: --");
    m_lblTime->setStyleSheet("font-size: 12px; color: #666666;");
    m_btnChart = new QPushButton("切换chars");
    m_btnChart->setFixedSize(65, 30);
    connect(m_btnChart, &QPushButton::clicked, this, &DeviceCardWidget::onShowChartClicked);
    bottomLayout->addWidget(m_lblTime, 1);
    bottomLayout->addWidget(m_btnChart);
    mainLayout->addLayout(bottomLayout);
}

void DeviceCardWidget::updateStatus(DeviceStatus status)
{
    QString color;
    QString statusText;
    switch (status) {
    case DeviceStatus::Online:
        color = "#00cc00"; // 绿色
        statusText = "Online";
        break;
    case DeviceStatus::Offline:
        color = "#cc0000"; // 红色
        statusText = "Offline";
        break;
    case DeviceStatus::Connecting:
        color = "#ffcc00"; // 黄色
        statusText = "Connecting";
        break;
    case DeviceStatus::Error:
        color = "#ff6600"; // 橙色
        statusText = "Error";
        break;
    }

    m_lblStatus->setStyleSheet(QString("font-size: 24px; color: %1;").arg(color));
    m_lblStatus->setToolTip(statusText);
}

void DeviceCardWidget::updateData(const QVector<quint16>& rawData, const QDateTime& timestamp)
{
    if (!rawData.isEmpty()) {
        // 简单显示第一个寄存器的值
        m_lblData->setText(QString("Reg0: %1").arg(rawData[0]));
    }
    m_lblTime->setText(QString("Last: %1").arg(timestamp.toString("hh:mm:ss")));
}

void DeviceCardWidget::onShowChartClicked()
{
    emit sigShowChart(m_device->deviceId);
}

// ------------------------------ DevicePanelWidget ------------------------------
DevicePanelWidget::DevicePanelWidget(QWidget *parent)
    : QWidget(parent),
    m_scrollArea(new QScrollArea(this)),
    m_contentWidget(new QWidget()),
    m_gridLayout(new QGridLayout(m_contentWidget))
{
    initUi();
    refreshDevices();
}

DevicePanelWidget::~DevicePanelWidget()
{
    clearCards();
}

void DevicePanelWidget::initUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);


    m_scrollArea->setWidget(m_contentWidget);
    m_scrollArea->setWidgetResizable(true);
    // 水平滚动条关闭（因为卡片不会超出宽度了）
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 垂直滚动条按需显示
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainLayout->addWidget(m_scrollArea);

    m_gridLayout->setSpacing(10);
    m_gridLayout->setContentsMargins(10, 10, 10, 10);
    m_contentWidget->setLayout(m_gridLayout);
}
void DevicePanelWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 窗口大小变化时，重新计算列数并刷新卡片
    refreshDevices();
}

void DevicePanelWidget::refreshDevices()
{
    clearCards();

    QList<DeviceInfoPtr> devices = DeviceManager::instance().getAllDevices();
    if (devices.isEmpty()) return;

    // ========== 新增：根据面板宽度自动计算列数 ==========
    // 1. 获取面板可用宽度（减去边距）
    int availableWidth = m_scrollArea->viewport()->width() - 20;
    // 2. 单个卡片需要的宽度（卡片180px + 布局间距10px）
    int cardTotalWidth = 160 + 10;
    // 3. 计算最大列数，最少1列，避免除0
    int maxCols = qMax(1, availableWidth / cardTotalWidth);
    // =================================================

    int row = 0, col = 0;

    for (DeviceInfoPtr device : devices) {
        DeviceCardWidget* card = new DeviceCardWidget(device, this);
        connect(card, &DeviceCardWidget::sigShowChart, this, &DevicePanelWidget::sigShowChart);

        m_gridLayout->addWidget(card, row, col);
        m_deviceCards[device->deviceId] = card;

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // 底部添加弹性空间
    m_gridLayout->setRowStretch(row + 1, 1);
}

void DevicePanelWidget::updateDeviceStatus(int deviceId, DeviceStatus status)
{
    if (m_deviceCards.contains(deviceId)) {
        m_deviceCards[deviceId]->updateStatus(status);
    }
}

void DevicePanelWidget::updateDeviceData(int deviceId, const QVector<quint16>& rawData, const QDateTime& timestamp)
{
    if (m_deviceCards.contains(deviceId)) {
        m_deviceCards[deviceId]->updateData(rawData, timestamp);
    }
}

void DevicePanelWidget::clearCards()
{
    for (auto it = m_deviceCards.begin(); it != m_deviceCards.end(); ++it) {
        it.value()->deleteLater();
    }
    m_deviceCards.clear();

    // 清空布局
    QLayoutItem* item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}
