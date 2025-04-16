#include "LoginWindow.h"
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
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

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
    mainLayout->addStretch();

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

    statusLabel->setText("正在登录...");
    loginButton->setEnabled(false);
    chatClient->login(username, password);
}

void LoginWindow::handleLoginSuccess(const QString& username, const QString& nickname)
{
    statusLabel->setText("登录成功");
    loginButton->setEnabled(true);
    usernameEdit->clear();
    passwordEdit->clear();
    emit loginSuccessful(username, nickname);
}

void LoginWindow::handleError(const QString& error)
{
    statusLabel->setText(error);
    loginButton->setEnabled(true);
}

void LoginWindow::showRegister()
{
    emit showRegisterWindow();
}