#include "RegisterWindow.h"

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


RegisterWindow::RegisterWindow(ChatClient* client, QWidget* parent)
    : QMainWindow(parent), chatClient(client), isDragging(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
    setupUi();
    connectSignals();
    setWindowTitle("聊天客户端 - 注册");
}

RegisterWindow::~RegisterWindow() {}

void RegisterWindow::setupUi()
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

    QLabel* nicknameLabel = new QLabel("昵称:");
    nicknameEdit = new QLineEdit();
    nicknameEdit->setPlaceholderText("请输入昵称");
    nicknameEdit->setObjectName("nicknameEdit");
    formLayout->addRow(nicknameLabel, nicknameEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    registerButton = new QPushButton("注册");
    registerButton->setObjectName("registerButton");
    backToLoginButton = new QPushButton("返回登录");
    backToLoginButton->setObjectName("backToLoginButton");
    buttonLayout->addStretch();
    buttonLayout->addWidget(registerButton);
    buttonLayout->addWidget(backToLoginButton);
    buttonLayout->addStretch();

    statusLabel = new QLabel();
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addSpacerItem(new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Fixed));

    setFixedSize(450, 400);
    setObjectName("RegisterWindow");

#ifndef LOW_PERFORMANCE
    auto* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(10);
    effect->setColor(QColor(0, 0, 0, 20));
    effect->setOffset(0, 2);
    centralWidget->setGraphicsEffect(effect);
#endif
}

void RegisterWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDragging = true;
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void RegisterWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
}

void RegisterWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDragging = false;
        event->accept();
    }
}

void RegisterWindow::connectSignals()
{
    connect(registerButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(backToLoginButton, &QPushButton::clicked, this, &RegisterWindow::showLogin);
    connect(chatClient, &ChatClient::registerSuccess, this, &RegisterWindow::handleRegisterSuccess);
    connect(chatClient, &ChatClient::errorOccurred, this, &RegisterWindow::handleError);

    // 新增, 异步连接
    connect(chatClient, &ChatClient::connected, this, &RegisterWindow::onChatClientConnected);
    connect(chatClient, &ChatClient::errorOccurred, this, &RegisterWindow::onChatClientError);
    connect(chatClient, &ChatClient::connectionError, this, &RegisterWindow::onChatClientConnectionError);
}

void RegisterWindow::handleRegister()
{
    currentUsername = usernameEdit->text().trimmed();
    currentPassword = passwordEdit->text();
    currentNickname = nicknameEdit->text().trimmed();

    if (currentUsername.isEmpty() || currentPassword.isEmpty() || currentNickname.isEmpty())
    {
        statusLabel->setText("所有字段都必须填写");
        return;
    }
    if (currentUsername.length() > 32 || currentPassword.length() > 32 || currentNickname.length() > 16)
    {
        statusLabel->setText("用户名/密码≤32字符，昵称≤16字符");
        return;
    }
    if (!currentUsername.contains(QRegularExpression("^[a-zA-Z0-9_]+$")))
    {
        statusLabel->setText("用户名只能包含字母、数字和下划线");
        return;
    }
    if (currentPassword.length() < 8 || !currentPassword.contains(QRegularExpression("[0-9]")) ||
        !currentPassword.contains(QRegularExpression("[a-zA-Z]")))
    {
        statusLabel->setText("密码需至少8位，包含字母和数字");
        return;
    }

    // --- 核心修改：注册尝试状态管理 ---
    if (m_isRegisterAttemptActive) {
        qDebug() << "RegisterWindow: 注册尝试已在进行中，忽略重复点击。";
        return;
    }

    m_isRegisterAttemptActive = true; // 标记有注册尝试正在进行

    statusLabel->setText("正在连接服务器...");
    setUiEnabled(false); // 禁用UI，防止重复点击，并给出用户反馈

    // 根据ChatClient的连接状态决定下一步操作
    if (chatClient->isConnected())
    {
        // 如果ChatClient已连接，直接发送注册请求
        statusLabel->setText("连接已建立，正在注册...");
        chatClient->registerUser(currentUsername, currentPassword, currentNickname);
    }
    else if (chatClient->connectionState() == ChatClient::ConnectionState::Connecting ||
             chatClient->connectionState() == ChatClient::ConnectionState::Reconnecting)
    {
        // 如果正在连接或重连中，等待连接成功信号，不重复发起连接
        statusLabel->setText("正在连接中，请稍候...");
    }
    else
    {
        // 如果未连接，发起连接请求。连接成功后会自动触发 onChatClientConnected
        chatClient->connectToServer(ConfigManager::instance().tcpHost(), ConfigManager::instance().tcpPort());
    }

}

// 槽函数：处理 ChatClient 注册成功信号 (业务层面)
void RegisterWindow::handleRegisterSuccess()
{
    statusLabel->setText("注册成功");
    QMessageBox::information(this, "注册成功", "账号注册成功，请返回登录界面进行登录。");
    setUiEnabled(true); // 重新启用UI
    usernameEdit->clear();
    passwordEdit->clear();
    nicknameEdit->clear();

    // 清空暂存的注册凭据
    currentUsername.clear();
    currentPassword.clear();
    currentNickname.clear();
    m_isRegisterAttemptActive = false; // 注册成功，重置标志

    emit registerSuccessful(); // 通知上层管理器注册成功
    emit showLoginWindow();    // 导航回登录窗口 (使用统一的信号名称)
}

// 槽函数：处理 ChatClient 业务错误信号 (如用户名已存在，服务器拒绝注册)
void RegisterWindow::handleError(const QString& error)
{
    statusLabel->setText("注册失败：" + error);
    setUiEnabled(true); // 重新启用UI
    // 清空暂存的凭据，避免下次连接成功后自动尝试注册
    currentUsername.clear();
    currentPassword.clear();
    currentNickname.clear();
    m_isRegisterAttemptActive = false; // 注册失败，重置标志
}
void RegisterWindow::showLogin()
{
    emit showLoginWindow();
}


// 槽函数：处理 ChatClient 连接成功信号 (TCP 层面)
void RegisterWindow::onChatClientConnected()
{
    if(!this->isVisible())return;
    // 只有当有活跃的注册尝试时，才发送注册请求
    // 并且确保临时凭据非空，避免意外触发
    if (m_isRegisterAttemptActive && !currentUsername.isEmpty() && !currentPassword.isEmpty() && !currentNickname.isEmpty())
    {
        statusLabel->setText("服务器连接成功，正在注册...");
        // --- 修正：调用 registerUser 而不是 login ---
        chatClient->registerUser(currentUsername, currentPassword, currentNickname);
    } else {
        // 如果没有活跃的注册尝试（例如，可能是ChatClient内部重连成功，但用户并未点击注册），
        // 则更新状态，并允许新的尝试。
        statusLabel->setText("服务器连接成功，请填写注册信息。");
        setUiEnabled(true); // 重新启用UI
        m_isRegisterAttemptActive = false; // 重置标志
    }
}

// 槽函数：处理 ChatClient 业务错误信号
void RegisterWindow::onChatClientError(const QString& error)
{
    statusLabel->setText("注册失败：" + error);
    registerButton->setEnabled(true); // 重新启用登录按钮
    // 清空存储的用户名和密码，避免下次连接成功后自动登录
    currentUsername.clear();
    currentPassword.clear();
    currentNickname.clear();
}


// 槽函数：处理 ChatClient 连接错误信号 (如连接超时，服务器拒绝连接)
void RegisterWindow::onChatClientConnectionError(const QString& message)
{
    statusLabel->setText("连接错误：" + message);
    setUiEnabled(true); // 重新启用UI
    // 清空暂存的凭据
    currentUsername.clear();
    currentPassword.clear();
    currentNickname.clear();
    m_isRegisterAttemptActive = false; // 连接失败，重置标志
}

void RegisterWindow::setUiEnabled(bool enabled)
{
    usernameEdit->setEnabled(enabled);
    passwordEdit->setEnabled(enabled);
    nicknameEdit->setEnabled(enabled);
    registerButton->setEnabled(enabled);
    backToLoginButton->setEnabled(enabled);
    // 确保在禁用时，焦点不会停留在禁用控件上，可以移开
    if (!enabled) {
        setFocus(); // 或者将焦点设置到其他可用的地方
    }
}