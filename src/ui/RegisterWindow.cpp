#include "RegisterWindow.h"
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
    mainLayout->addStretch();

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

// 其余代码不变

void RegisterWindow::connectSignals()
{
    connect(registerButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(backToLoginButton, &QPushButton::clicked, this, &RegisterWindow::showLogin);
    connect(chatClient, &ChatClient::registerSuccess, this, &RegisterWindow::handleRegisterSuccess);
    connect(chatClient, &ChatClient::errorOccurred, this, &RegisterWindow::handleError);
}

void RegisterWindow::handleRegister()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString nickname = nicknameEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || nickname.isEmpty())
    {
        statusLabel->setText("所有字段都必须填写");
        return;
    }
    if (username.length() > 32 || password.length() > 32 || nickname.length() > 16)
    {
        statusLabel->setText("用户名/密码≤32字符，昵称≤16字符");
        return;
    }
    if (!username.contains(QRegularExpression("^[a-zA-Z0-9_]+$")))
    {
        statusLabel->setText("用户名只能包含字母、数字和下划线");
        return;
    }
    if (password.length() < 8 || !password.contains(QRegularExpression("[0-9]")) ||
        !password.contains(QRegularExpression("[a-zA-Z]")))
    {
        statusLabel->setText("密码需至少8位，包含字母和数字");
        return;
    }

    statusLabel->setText("正在注册...");
    registerButton->setEnabled(false);
    chatClient->registerUser(username, password, nickname);
}

void RegisterWindow::handleRegisterSuccess(const QString& token)
{
    statusLabel->setText("注册成功");
    QMessageBox::information(this, "注册成功", "账号注册成功，请返回登录界面进行登录。");
    registerButton->setEnabled(true);
    usernameEdit->clear();
    passwordEdit->clear();
    nicknameEdit->clear();
    emit registerSuccessful();
    showLogin();
}

void RegisterWindow::handleError(const QString& error)
{
    statusLabel->setText(error);
    registerButton->setEnabled(true);
}

void RegisterWindow::showLogin()
{
    emit showLoginWindow();
}