#include "LoginWindow.h"

#include "utils/ConfigManager.h"

#include <QApplication>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QScreen>
#include <QStyle>

LoginWindow::LoginWindow(ChatClient* client, QWidget* parent)
    : QMainWindow(parent), chatClient(client), isDragging(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);  // 无边框窗口
    setupUi();
    connectSignals();
    setWindowTitle("聊天客户端 - 登录");
}

LoginWindow::~LoginWindow() {}

void LoginWindow::setupUi()
{
    centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    // 标题容器
    QWidget* titleContainer = new QWidget();
    titleContainer->setObjectName("titleContainer");
    QVBoxLayout* titleLayout = new QVBoxLayout(titleContainer);
    titleLayout->setSpacing(4);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setAlignment(Qt::AlignCenter);

    QLabel* titleLabel = new QLabel("My Chatter");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("connect the world");
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(subtitleLabel);

    mainLayout->addWidget(titleContainer);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* usernameLabel = new QLabel("用户名:");
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("请输入用户名");
    usernameEdit->setObjectName("usernameEdit");
    formLayout->addRow(usernameLabel, usernameEdit);

    QLabel* passwordLabel = new QLabel("密码:");
    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("请输入密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setObjectName("passwordEdit");
    formLayout->addRow(passwordLabel, passwordEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    loginButton = new QPushButton("登录");
    loginButton->setObjectName("loginButton");
    registerButton = new QPushButton("注册");
    registerButton->setObjectName("registerButton");
    buttonLayout->addStretch();
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(registerButton);
    buttonLayout->addStretch();

    statusLabel = new QLabel();
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Fixed));

    setFixedSize(450, 350);
    setObjectName("LoginWindow");

#ifndef LOW_PERFORMANCE
    auto* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(10);
    effect->setColor(QColor(0, 0, 0, 20));
    effect->setOffset(0, 2);
    centralWidget->setGraphicsEffect(effect);
#endif
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDragging = true;
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void LoginWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}

void LoginWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDragging = false;
        event->accept();
    }
}

void LoginWindow::connectSignals()
{
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(registerButton, &QPushButton::clicked, this, &LoginWindow::showRegister);
    connect(chatClient, &ChatClient::loginSuccess, this, &LoginWindow::handleLoginSuccess);
    connect(chatClient, &ChatClient::errorOccurred, this, &LoginWindow::handleError);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);

    // 新增, 异步连接
    connect(chatClient, &ChatClient::connected, this, &LoginWindow::onChatClientConnected);
    connect(chatClient, &ChatClient::errorOccurred, this, &LoginWindow::onChatClientError);
    connect(chatClient, &ChatClient::connectionError, this, &LoginWindow::onChatClientConnectionError);

}

void LoginWindow::handleLogin()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty())
    {
        statusLabel->setText("用户名和密码不能为空");
        return;
    }
    if (username.length() > 32 || password.length() > 32)
    {
        statusLabel->setText("用户名或密码不能超过32个字符");
        return;
    }
    if (!username.contains(QRegularExpression("^[a-zA-Z0-9_]+$")))
    {
        statusLabel->setText("用户名只能包含字母、数字和下划线");
        return;
    }

    // 如果已经有登录尝试正在进行，则直接返回，避免重复操作
    if (m_isLoginAttemptActive) {
        qDebug() << "LoginWindow: 登录尝试已在进行中，忽略重复点击。";
        return;
    }

    // 设置登录尝试标志为 true
    m_isLoginAttemptActive = true;
    currentUsername = username;
    currentPassword = password;

    statusLabel->setText("正在连接服务器...");
    loginButton->setEnabled(false); // 禁用按钮，避免重复点击

    // 检查当前连接状态
    if (chatClient->isConnected())
    {
        // 如果已经连接（TCP层面），直接尝试登录
        statusLabel->setText("连接已建立，正在登录...");
        chatClient->login(currentUsername, currentPassword);
    }
    else if (chatClient->connectionState() == ChatClient::ConnectionState::Connecting ||
             chatClient->connectionState() == ChatClient::ConnectionState::Reconnecting)
    {
        // 正在连接中，等待连接成功信号
        statusLabel->setText("正在连接中，请稍候...");
        // 标记已经有尝试进行，不再次调用 connectToServer
    }
    else
    {
        // 未连接状态，发起连接请求
        chatClient->connectToServer(ConfigManager::instance().tcpHost(), ConfigManager::instance().tcpPort());
    }
}


void LoginWindow::handleLoginSuccess(const QString& username, const QString& nickname)
{
    statusLabel->setText("登录成功");
    loginButton->setEnabled(true);
    usernameEdit->clear();
    passwordEdit->clear();
    m_isLoginAttemptActive = false; // 登录成功，重置标志
    emit loginSuccessful(username, nickname);
}

void LoginWindow::handleError(const QString& error)
{
    statusLabel->setText(error);
    loginButton->setEnabled(true);
    m_isLoginAttemptActive = false; // 登录失败，重置标志
    currentUsername.clear(); // 清除暂存的凭据
    currentPassword.clear();
}

void LoginWindow::showRegister()
{
    emit showRegisterWindow();
}


// 槽函数：处理 ChatClient 连接成功信号
void LoginWindow::onChatClientConnected()
{
    if(!this->isVisible())return;
    // 只有当登录窗口是活动状态且用户意图是登录时，才发送登录请求
    // 这里的判断条件可以根据你的 LoginWindow 的具体状态管理来定
    if (!currentUsername.isEmpty() && !currentPassword.isEmpty()) // 确保是因登录而触发的连接
    {
        statusLabel->setText("服务器连接成功，正在登录...");
        chatClient->login(currentUsername, currentPassword);
    } else {
        statusLabel->setText("服务器连接成功，请重新输入凭据登录。"); // 可能是意外的连接成功
        loginButton->setEnabled(true);
        m_isLoginAttemptActive = false; // 重置标志，允许新的尝试
    }
}



// 槽函数：处理 ChatClient 业务错误信号 (如登录失败)
void LoginWindow::onChatClientError(const QString& error)
{
    statusLabel->setText("登录失败：" + error);
    loginButton->setEnabled(true); // 重新启用登录按钮
    // 清空存储的用户名和密码，避免下次连接成功后自动登录
    currentUsername.clear();
    currentPassword.clear();
}

// 槽函数：处理 ChatClient 连接错误信号 (如连接超时，服务器拒绝)
void LoginWindow::onChatClientConnectionError(const QString& message)
{
    statusLabel->setText("连接错误：" + message);
    loginButton->setEnabled(true); // 重新启用登录按钮
    // 清空存储的用户名和密码，避免下次连接成功后自动登录
    currentUsername.clear();
    currentPassword.clear();

    m_isLoginAttemptActive = false; // 连接失败，重置标志
}
