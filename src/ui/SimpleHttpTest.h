#ifndef SIMPLEHTTPTEST_H
#define SIMPLEHTTPTEST_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class SimpleHttpTest : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleHttpTest(QWidget *parent = nullptr);

private slots:
    void onTestUploadClicked(); // 按钮点击槽
    void onUploadFinished(QNetworkReply* reply); // 上传完成槽

private:
    QPushButton* m_btnTest;
    QLabel* m_labelStatus;
    QNetworkAccessManager* m_networkManager;
};

#endif // SIMPLEHTTPTEST_H