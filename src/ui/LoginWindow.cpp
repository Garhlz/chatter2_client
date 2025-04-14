#include "LoginWindow.h"
#include <QApplication>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScreen>
#include <QStyle>


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
  mainLayout->setSpacing(15);
  mainLayout->setContentsMargins(30, 30, 30, 30);

  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(10);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *usernameLabel = new QLabel("用户名:", this);
  usernameEdit = new QLineEdit(this);
  usernameEdit->setPlaceholderText("请输入用户名");
  usernameEdit->setObjectName("usernameEdit");
  formLayout->addRow(usernameLabel, usernameEdit);

  QLabel *passwordLabel = new QLabel("密码:", this);
  passwordEdit = new QLineEdit(this);
  passwordEdit->setPlaceholderText("请输入密码");
  passwordEdit->setEchoMode(QLineEdit::Password);
  passwordEdit->setObjectName("passwordEdit");
  formLayout->addRow(passwordLabel, passwordEdit);

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(20);
  loginButton = new QPushButton("登录", this);
  loginButton->setObjectName("loginButton");
  registerButton = new QPushButton("注册新账号", this);
  registerButton->setObjectName("registerButton");
  registerButton->setFlat(true);
  buttonLayout->addWidget(loginButton);
  buttonLayout->addWidget(registerButton);
  buttonLayout->addStretch();

  statusLabel = new QLabel(this);
  statusLabel->setObjectName("statusLabel");
  statusLabel->setAlignment(Qt::AlignCenter);

  mainLayout->addLayout(formLayout);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addWidget(statusLabel);
  mainLayout->addStretch();

  setFixedSize(450, 350);
  setObjectName("LoginWindow");

  // 阴影效果
  setProperty("polish", true);
  setGraphicsEffect(new QGraphicsDropShadowEffect(this));
  auto *effect = graphicsEffect();
  if (effect) {
    auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(effect);
    if (shadow) {
      shadow->setBlurRadius(12);
      shadow->setColor(QColor(0, 0, 0, 38));
      shadow->setOffset(0, 4);
    }
  }

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
  connect(registerButton, &QPushButton::clicked, this,
          &LoginWindow::showRegister);
  connect(chatClient, &ChatClient::loginSuccess, this,
          &LoginWindow::handleLoginSuccess);
  connect(chatClient, &ChatClient::errorOccurred, this,
          &LoginWindow::handleError);
  connect(passwordEdit, &QLineEdit::returnPressed, this,
          &LoginWindow::handleLogin);
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

void LoginWindow::showRegister() { emit showRegisterWindow(); }