#include "RegisterWindow.h"
#include <QMessageBox>
#include <QApplication>
#include <QStyle>
#include <QRegularExpression>
#include <QGuiApplication>
#include <QScreen>

RegisterWindow::RegisterWindow(ChatClient *client, QWidget *parent)
    : QMainWindow(parent), chatClient(client) {
    setupUi();
    connectSignals();
    setWindowTitle("聊天客户端 - 注册");
}

RegisterWindow::~RegisterWindow() {}

void RegisterWindow::setupUi() {
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

    QLabel *nicknameLabel = new QLabel("昵称:", this);
    nicknameEdit = new QLineEdit(this);
    nicknameEdit->setPlaceholderText("请输入昵称");

    registerButton = new QPushButton("注册", this);
    backToLoginButton = new QPushButton("返回登录", this);
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(usernameLabel);
    mainLayout->addWidget(usernameEdit);
    mainLayout->addWidget(passwordLabel);
    mainLayout->addWidget(passwordEdit);
    mainLayout->addWidget(nicknameLabel);
    mainLayout->addWidget(nicknameEdit);
    mainLayout->addWidget(registerButton);
    mainLayout->addWidget(backToLoginButton);
    mainLayout->addWidget(statusLabel);

    setFixedSize(400, 350);
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

void RegisterWindow::connectSignals() {
    connect(registerButton, &QPushButton::clicked, this, &RegisterWindow::handleRegister);
    connect(backToLoginButton, &QPushButton::clicked, this, &RegisterWindow::showLogin);
    connect(chatClient, &ChatClient::registerSuccess, this, &RegisterWindow::handleRegisterSuccess);
    connect(chatClient, &ChatClient::errorOccurred, this, &RegisterWindow::handleError);
}

void RegisterWindow::handleRegister() {
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString nickname = nicknameEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || nickname.isEmpty()) {
        statusLabel->setText("所有字段都必须填写");
        return;
    }
    if (username.length() > 32 || password.length() > 32 || nickname.length() > 16) {
        statusLabel->setText("用户名/密码≤32字符，昵称≤16字符");
        return;
    }
    if (!username.contains(QRegularExpression("^[a-zA-Z0-9_]+$"))) {
        statusLabel->setText("用户名只能包含字母、数字和下划线");
        return;
    }
    if (password.length() < 8 || !password.contains(QRegularExpression("[0-9]")) ||
        !password.contains(QRegularExpression("[a-zA-Z]"))) {
        statusLabel->setText("密码需至少8位，包含字母和数字");
        return;
    }

    statusLabel->setText("正在注册...");
    registerButton->setEnabled(false);
    chatClient->registerUser(username, password, nickname);
}

void RegisterWindow::handleRegisterSuccess(const QString &token) {
    statusLabel->setText("注册成功");
    QMessageBox::information(this, "注册成功", "账号注册成功，请返回登录界面进行登录。");
    registerButton->setEnabled(true);
    usernameEdit->clear();
    passwordEdit->clear();
    nicknameEdit->clear();
    emit registerSuccessful();
    showLogin();
}

void RegisterWindow::handleError(const QString &error) {
    statusLabel->setText(error);
    registerButton->setEnabled(true);
}

void RegisterWindow::showLogin() {
    emit showLoginWindow();
}