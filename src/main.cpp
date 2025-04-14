#include "network/ChatClient.h"
#include "ui/ChatWindow.h"
#include "ui/LoginWindow.h"
#include "ui/RegisterWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QMessageBox>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // 解析命令行参数
  QCommandLineParser parser;
  parser.setApplicationDescription("Chat Client");
  parser.addHelpOption();
  parser.addOption({"host", "Server host", "host", "localhost"});
  parser.addOption({"port", "Server port", "port", "9999"});
  parser.process(app);

  QString host = parser.value("host");
  bool ok;
  quint16 port = parser.value("port").toUShort(&ok);
  if (!ok) {
    port = 9999;
  }

  // 创建客户端和窗口
  ChatClient *chatClient = new ChatClient(&app);
  LoginWindow *loginWindow = new LoginWindow(chatClient);
  RegisterWindow *registerWindow = new RegisterWindow(chatClient);

  // 连接信号
  QObject::connect(loginWindow, &LoginWindow::showRegisterWindow,
                   [loginWindow, registerWindow]() {
                     loginWindow->hide();
                     registerWindow->show();
                   });
  QObject::connect(registerWindow, &RegisterWindow::showLoginWindow,
                   [registerWindow, loginWindow]() {
                     registerWindow->hide();
                     loginWindow->show();
                   });
  QObject::connect(
      loginWindow, &LoginWindow::loginSuccessful,
      [loginWindow, chatClient](const QString &nickname) {
        try {
          qDebug() << "Main: Handling loginSuccessful for nickname ="
                   << nickname;
          loginWindow->hide();
          ChatWindow *newChatWindow = new ChatWindow(chatClient, nickname);
          newChatWindow->setAttribute(Qt::WA_DeleteOnClose);
          qDebug() << "Main: Showing ChatWindow";
          newChatWindow->show();
        } catch (const std::exception &e) {
          qDebug() << "Main: Exception in loginSuccessful:" << e.what();
          QMessageBox::critical(nullptr, "错误",
                                QString("创建聊天窗口失败: %1").arg(e.what()));
          loginWindow->show();
        } catch (...) {
          qDebug() << "Main: Unknown exception in loginSuccessful";
          QMessageBox::critical(nullptr, "错误", "创建聊天窗口时发生未知错误");
          loginWindow->show();
        }
      });
  QObject::connect(chatClient, &ChatClient::disconnected, [loginWindow]() {
    qDebug() << "Main: ChatClient disconnected";
    loginWindow->show();
  });

  // 加载 QSS 样式
  QString combinedStyle;
  bool styleLoaded = true;
  QStringList styleFiles = {":/styles/styles.qss", ":/styles/messagebubble.qss",
                            ":/styles/chatwindow.qss"};

  for (const QString &filePath : styleFiles) {
    QFile styleFile(filePath);
    if (!styleFile.exists()) {
      qDebug() << "Error: Style file not found:" << filePath;
      styleLoaded = false;
      continue;
    }
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
      QString styleContent = QString::fromUtf8(styleFile.readAll());
      if (!styleContent.isEmpty()) {
        combinedStyle += styleContent + "\n";
        qDebug() << "Loaded" << filePath << "successfully";
      } else {
        qDebug() << "Warning:" << filePath << "is empty";
        styleLoaded = false;
      }
      styleFile.close();
    } else {
      qDebug() << "Failed to open" << filePath << ":"
               << styleFile.errorString();
      styleLoaded = false;
    }
  }

  // 应用样式
  if (!combinedStyle.isEmpty()) {
    app.setStyleSheet(combinedStyle);
    qDebug() << "Applied QSS styles successfully";
  } else {
    qDebug() << "Error: No QSS styles applied";
    QMessageBox::warning(nullptr, "样式警告",
                         "无法加载样式文件，界面可能显示异常");
  }

  // 如果样式加载部分失败，记录警告
  if (!styleLoaded) {
    qDebug() << "Warning: One or more QSS files failed to load properly";
  }

  // 连接服务器
  qDebug() << "Main: Connecting to server at" << host << ":" << port;
  chatClient->connectToServer(host, port);
  loginWindow->show();

  return app.exec();
}