#include "LoginDialog.h"
#include "ui/ui_LoginDialog.h"
#include "../db/SqliteManager.h"
#include "../common/Logger.h"
#include <QCryptographicHash>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>

// 默认用户：admin/admin123（管理员），operator/operator123（操作员）
const QMap<QString, UserInfo> LoginDialog::DEFAULT_USERS = {
    {"admin", {"admin", QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex(), UserRole::Administrator}},
    {"operator", {"operator", QCryptographicHash::hash("operator123", QCryptographicHash::Sha256).toHex(), UserRole::Operator}}
};

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle("SCADA登录");
    setFixedSize(400, 300);

    // 初始化默认用户到SQLite
    initDefaultUsers();

    // 加载保存的登录信息
    loadSavedLogin();

    // 连接信号槽
    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    connect(ui->lePassword, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

UserInfo LoginDialog::getLoggedInUser() const
{
    return m_loggedInUser;
}

void LoginDialog::initDefaultUsers()
{
    // 简化处理：直接使用DEFAULT_USERS，实际项目中应存储到SQLite的t_user表
    // 这里预留扩展
}

bool LoginDialog::verifyUser(const QString &username, const QString &password, UserInfo &userInfo)
{
    if (DEFAULT_USERS.contains(username)) {
        UserInfo defaultUser = DEFAULT_USERS.value(username);
        QString passwordHash = hashPassword(password);
        // ✅ 正确调试打印（你要的功能）
        qDebug() << "==================== 调试登录 ====================";
        qDebug() << "输入用户名：" << username;
        qDebug() << "输入密码明文：" << password;
        qDebug() << "数据库哈希：" << defaultUser.passwordHash;
        qDebug() << "计算哈希：" << passwordHash;
        if (defaultUser.passwordHash == passwordHash) {
            userInfo = defaultUser;
             qDebug() << "密码比对一致" << passwordHash;
            return true;
        }
    }
    return false;
}

QString LoginDialog::hashPassword(const QString &password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
}

void LoginDialog::loadSavedLogin()
{
    QSettings settings("IndustrialSCADA", "ModbusTcpScada");
    bool remember = settings.value("login/remember", false).toBool();
    bool autoLogin = settings.value("login/auto_login", false).toBool();

    ui->cbRemember->setChecked(remember);
    ui->cbAutoLogin->setChecked(autoLogin);

    if (remember) {
        QString username = settings.value("login/username", "").toString();
        QString password = settings.value("login/password", "").toString();
        ui->leUsername->setText(username);
        ui->lePassword->setText(password);
    }

    if (autoLogin && remember) {
        // 延迟自动登录，确保UI已加载
        QTimer::singleShot(100, this, [this]() {
            onLoginClicked();
        });
    }
}

void LoginDialog::saveLogin(const QString &username, const QString &password, bool remember, bool autoLogin)
{
    QSettings settings("IndustrialSCADA", "ModbusTcpScada");
    settings.setValue("login/remember", remember);
    settings.setValue("login/auto_login", autoLogin);

    if (remember) {
        settings.setValue("login/username", username);
        settings.setValue("login/password", password);
    } else {
        settings.remove("login/username");
        settings.remove("login/password");
    }

    if (!autoLogin) {
        settings.remove("login/auto_login");
    }
}

void LoginDialog::onLoginClicked()
{
    QString username = ui->leUsername->text().trimmed();
    QString password = ui->lePassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        ui->lblError->setText("Username or password cannot be empty");
        return;
    }

    UserInfo userInfo;
    if (verifyUser(username, password, userInfo)) {
        QMessageBox::information(this, "通知", "连接成功");
        m_loggedInUser = userInfo;
        saveLogin(username, password, ui->cbRemember->isChecked(), ui->cbAutoLogin->isChecked());
        LOG_INFO("LoginDialog", QString("User %1 logged in successfully (Role: %2)")
                                    .arg(username).arg(userInfo.role == UserRole::Administrator ? "Administrator" : "Operator"));
        accept();
    } else {
        ui->lblError->setText("Invalid username or password");
        LOG_WARNING("LoginDialog", QString("Failed login attempt for user %1").arg(username));
    }
}

void LoginDialog::onCancelClicked()
{
    reject();
}