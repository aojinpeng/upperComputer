#include "SimpleHttpTest.h"
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

SimpleHttpTest::SimpleHttpTest(QWidget *parent)
    : QWidget(parent)
{
    // 1. 初始化UI
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_btnTest = new QPushButton("点击测试HTTP上传（绝对不卡死）", this);
    m_labelStatus = new QLabel("等待测试...", this);
    layout->addWidget(m_btnTest);
    layout->addWidget(m_labelStatus);

    // 2. 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);

    // 3. 绑定按钮点击
    connect(m_btnTest, &QPushButton::clicked, this, &SimpleHttpTest::onTestUploadClicked);
}

void SimpleHttpTest::onTestUploadClicked()
{
    m_labelStatus->setText("正在上传...");
    qDebug() << "[SimpleHttpTest] 开始测试上传";

    // 1. 构造最简单的JSON数据
    QJsonObject json;
    json["device_id"] = 999;
    json["tag_name"] = "test";
    json["value"] = 25.5;
    json["timestamp"] = "2025-01-01T12:00:00";
    json["is_alarm"] = false;
    json["alarm_msg"] = "";

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    // 2. 构造HTTP请求
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/upload"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 3. 【关键！异步发送请求，绝对不阻塞UI】
    QNetworkReply* reply = m_networkManager->post(request, postData);

    // 4. 绑定完成信号（Lambda表达式，简单直接）
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onUploadFinished(reply);
    });
}

void SimpleHttpTest::onUploadFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QString result = QString::fromUtf8(response);
        m_labelStatus->setText("✅ 上传成功！响应：" + result);
        qDebug() << "[SimpleHttpTest] 上传成功，响应：" << result;
    } else {
        QString error = reply->errorString();
        m_labelStatus->setText("❌ 上传失败：" + error);
        qDebug() << "[SimpleHttpTest] 上传失败：" << error;
    }
    reply->deleteLater(); // 安全释放
}