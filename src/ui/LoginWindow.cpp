#include "LoginWindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QScreen>

LoginWindow::LoginWindow(ChatClient *client, QWidget *parent)
    : QMainWindow(parent), chatClient(client) {
    setupUi();
    connectSignals();
    setWindowTitle("聊天客户端 - 登录");
}

LoginWindow::~LoginWindow() {}

void LoginWindow::setupUi() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *usernameLabel = new QLabel("用户名:", this);
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText("请输入用户名");

    QLabel *passwordLabel = new QLabel("密码:", this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("请输入密码");
    passwordEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("登录", this);
    registerButton = new QPushButton("注册新账号", this);
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(usernameLabel);
    mainLayout->addWidget(usernameEdit);
    mainLayout->addWidget(passwordLabel);
    mainLayout->addWidget(passwordEdit);
    mainLayout->addWidget(loginButton);
    mainLayout->addWidget(registerButton);
    mainLayout->addWidget(statusLabel);

    setFixedSize(400, 300);
    setStyleSheet(
        "QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 3px; }"
        "QPushButton { padding: 8px; background-color: #4CAF50; color: white; "
        "border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QLabel { color: #333; }"
        "#statusLabel { color: red; }"
    );

    // Qt 6 居中窗口
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        QSize windowSize = size();
        int x = (screenGeometry.width() - windowSize.width()) / 2;
        int y = (screenGeometry.height() - windowSize.height()) / 2;
        move(x, y);
    }
}

void LoginWindow::connectSignals() {
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(registerButton, &QPushButton::clicked, this, &LoginWindow::showRegister);
    connect(chatClient, &ChatClient::loginSuccess, this, &LoginWindow::handleLoginSuccess);
    connect(chatClient, &ChatClient::errorOccurred, this, &LoginWindow::handleError);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin() {
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        statusLabel->setText("用户名和密码不能为空");
        return;
    }
    if (username.length() > 32 || password.length() > 32) {
        statusLabel->setText("用户名或密码不能超过32个字符");
        return;
    }
    if (!username.contains(QRegularExpression("^[a-zA-Z0-9_]+$"))) {
        statusLabel->setText("用户名只能包含字母、数字和下划线");
        return;
    }

    statusLabel->setText("正在登录...");
    loginButton->setEnabled(false);
    chatClient->login(username, password);
}

void LoginWindow::handleLoginSuccess(const QString &nickname) {
    statusLabel->setText("登录成功");
    loginButton->setEnabled(true);
    usernameEdit->clear();
    passwordEdit->clear();
    emit loginSuccessful(nickname);
}

void LoginWindow::handleError(const QString &error) {
    statusLabel->setText(error);
    loginButton->setEnabled(true);
}

void LoginWindow::showRegister() {
    emit showRegisterWindow();
}