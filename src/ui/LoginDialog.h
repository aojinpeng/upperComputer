#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "../common/Defines.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

// 用户权限枚举
enum class UserRole {
    Operator = 0,
    Administrator = 1
};

struct UserInfo {
    QString username;
    QString passwordHash;
    UserRole role;
};

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    // 获取登录后的用户信息
    UserInfo getLoggedInUser() const;

private slots:
    void onLoginClicked();
    void onCancelClicked();

private:
    // 内部辅助函数
    void initDefaultUsers();
    bool verifyUser(const QString& username, const QString& password, UserInfo& userInfo);
    QString hashPassword(const QString& password);
    void loadSavedLogin();
    void saveLogin(const QString& username, const QString& password, bool remember, bool autoLogin);

private:
    Ui::LoginDialog *ui;
    UserInfo m_loggedInUser;
    static const QMap<QString, UserInfo> DEFAULT_USERS;
};

#endif // LOGINDIALOG_H