#include "HttpClient.h"
#include "../common/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>

HttpClient::HttpClient(QObject *parent)
    : QObject(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_uploadTimer(new QTimer(this))
{
    m_uploadTimer->setSingleShot(true);
}

HttpClient::~HttpClient()
{
}

void HttpClient::uploadDataPoint(const QString& url, const DataPointPtr& dataPoint)
{
    // 构造JSON数据
    QJsonObject json;
    json["device_id"] = dataPoint->deviceId;
    json["tag_name"] = dataPoint->tagName;
    json["value"] = dataPoint->value;
    json["timestamp"] = dataPoint->timestamp.toString(Qt::ISODate);
    json["is_alarm"] = dataPoint->isAlarm;
    json["alarm_msg"] = dataPoint->alarmMsg;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    // 构造HTTP请求
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送POST请求
    QNetworkReply* reply = m_networkManager->post(request, postData);

    // 超时处理
    m_uploadTimer->start(UPLOAD_TIMEOUT_MS);
    connect(m_uploadTimer, &QTimer::timeout, this, [this, reply]() {
        if (reply->isRunning()) {
            reply->abort();
            QString error = "Upload timeout";
            LOG_ERROR("HttpClient", error);
            emit sigUploadFailed(error);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_uploadTimer->stop();
        onUploadFinished(reply);
    });
}

void HttpClient::onUploadFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        LOG_INFO("HttpClient", QString("Upload success, response: %1").arg(QString(response)));
        emit sigUploadSuccess(QString(response));
    } else {
        QString error = reply->errorString();
        LOG_ERROR("HttpClient", QString("Upload failed: %1").arg(error));
        emit sigUploadFailed(error);
    }
    reply->deleteLater();
}
void HttpClient::onUploadTimeout(){};
