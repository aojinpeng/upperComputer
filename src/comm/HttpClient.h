#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include "../common/Defines.h"

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject *parent = nullptr);
    ~HttpClient() override;

    void uploadDataPoint(const QString& url, const DataPointPtr& dataPoint);

signals:
    void sigUploadSuccess(const QString& response);
    void sigUploadFailed(const QString& error);

private slots:
    void onUploadFinished(QNetworkReply* reply);
    void onUploadTimeout();

private:
    QNetworkAccessManager* m_networkManager;
    QTimer* m_uploadTimer;
    static const int UPLOAD_TIMEOUT_MS = 10000;
};

#endif // HTTPCLIENT_H
