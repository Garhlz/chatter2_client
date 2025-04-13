#include <QApplication>
#include <QCommandLineParser>
#include "network/ChatClient.h"
#include "ui/LoginWindow.h"
#include "ui/RegisterWindow.h"
#include "ui/ChatWindow.h"

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
    LoginWindow *loginWindow = new LoginWindow(chatClient, &app);
    RegisterWindow *registerWindow = new RegisterWindow(chatClient, &app);
    ChatWindow *chatWindow = nullptr;

    // 连接信号
    QObject::connect(loginWindow, &LoginWindow::showRegisterWindow, [=]() {
        loginWindow->hide();
        registerWindow->show();
    });
    QObject::connect(registerWindow, &RegisterWindow::showLoginWindow, [=]() {
        registerWindow->hide();
        loginWindow->show();
    });
    QObject::connect(loginWindow, &LoginWindow::loginSuccessful, [=](const QString &nickname) {
        loginWindow->hide();
        chatWindow = new ChatWindow(chatClient, nickname, &app);
        chatWindow->show();
    });
    QObject::connect(chatClient, &ChatClient::disconnected, [=]() {
        if (chatWindow) {
            chatWindow->hide();
            delete chatWindow;
            chatWindow = nullptr;
        }
        loginWindow->show();
    });

    // 连接服务器
    chatClient->connectToServer(host, port);
    loginWindow->show();

    return app.exec();
}