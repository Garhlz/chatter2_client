#include "network/ChatClient.h"
#include "ui/ChatWindow.h"
#include "ui/LoginWindow.h"
#include "ui/RegisterWindow.h"
#include "utils/ConfigManager.h"
#include "FileTransferManager.h"
#include "GlobalEventBus.h"
#include "utils/UserInfo.h"  // 确保有这个用于测试的用户信息
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>  // 确保包含这个头文件
#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QTimer>  // 用于模拟事件

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 初始化单例
    GlobalEventBus::instance();
    FileTransferManager::instance();  // 文件传输类 (即 HttpRequestManager) 初始化

    // 加载配置文件。即使失败，ConfigManager 也会设置默认值。
    if (!ConfigManager::instance().loadConfig(":/config.json"))
    {
        qWarning() << "无法加载配置文件或配置有误，将使用默认配置。";
    }

    // 获取ConfigManager中的当前（或默认）配置值作为命令行参数的默认值
    QString defaultTcpHost = ConfigManager::instance().tcpHost();
    QString defaultTcpPortStr = QString::number(ConfigManager::instance().tcpPort());
    QString defaultHttpHost = ConfigManager::instance().httpHost();
    QString defaultHttpPortStr = QString::number(ConfigManager::instance().httpPort());
    QString defaultApiPrefix = ConfigManager::instance().apiPrefix();

    // 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("Chat Client");
    parser.addHelpOption();
    parser.addVersionOption();  // 可选：添加版本选项

    // 添加命令行选项，使用 QCommandLineOption 构造函数指定短名称、长名称、值名称和默认值
    QCommandLineOption tcpHostOption({"th", "tcp-host"}, "TCP Server host", "host", defaultTcpHost);
    QCommandLineOption tcpPortOption({"tp", "tcp-port"}, "TCP Server port", "port",
                                     defaultTcpPortStr);
    QCommandLineOption httpHostOption({"hh", "http-host"}, "HTTP Server host", "host",
                                      defaultHttpHost);
    QCommandLineOption httpPortOption({"hp", "http-port"}, "HTTP Server port", "port",
                                      defaultHttpPortStr);
    QCommandLineOption apiPrefixOption({"ap", "api-prefix"}, "API prefix", "prefix",
                                       defaultApiPrefix);

    parser.addOption(tcpHostOption);
    parser.addOption(tcpPortOption);
    parser.addOption(httpHostOption);
    parser.addOption(httpPortOption);
    parser.addOption(apiPrefixOption);

    parser.process(app);

    // 获取命令行参数的值，并将其更新到 ConfigManager 中
    // 使用 value() 方法，参数是 addOption 时指定的长名称或短名称
    QString finalTcpHost = parser.value(tcpHostOption);
    QString finalTcpPortStr = parser.value(tcpPortOption);
    QString finalHttpHost = parser.value(httpHostOption);
    QString finalHttpPortStr = parser.value(httpPortOption);
    QString finalApiPrefix = parser.value(apiPrefixOption);

    // 将端口号字符串转换为 quint16，并进行错误检查
    bool tcpPortOk;
    quint16 finalTcpPort = finalTcpPortStr.toUShort(&tcpPortOk);
    if (!tcpPortOk || finalTcpPort == 0)
    {
        qWarning() << "命令行指定的 TCP 端口无效或为0 (" << finalTcpPortStr
                   << ")，将使用默认值 9999。";
        finalTcpPort = 9999;
    }

    bool httpPortOk;
    quint16 finalHttpPort = finalHttpPortStr.toUShort(&httpPortOk);
    if (!httpPortOk || finalHttpPort == 0)
    {
        qWarning() << "命令行指定的 HTTP 端口无效或为0 (" << finalHttpPortStr
                   << ")，将使用默认值 8080。";
        finalHttpPort = 8080;
    }

    // 更新 ConfigManager 的值，确保整个应用获取的是最终配置
    ConfigManager::instance().setTcpHost(finalTcpHost);
    ConfigManager::instance().setTcpPort(finalTcpPort);
    ConfigManager::instance().setHttpHost(finalHttpHost);
    ConfigManager::instance().setHttpPort(finalHttpPort);
    ConfigManager::instance().setApiPrefix(finalApiPrefix);

    qDebug() << "最终配置：TCP Host=" << ConfigManager::instance().tcpHost()
             << ", TCP Port=" << ConfigManager::instance().tcpPort()
             << ", HTTP Host=" << ConfigManager::instance().httpHost()
             << ", HTTP Port=" << ConfigManager::instance().httpPort()
             << ", API Prefix=" << ConfigManager::instance().apiPrefix();

    // 创建客户端和窗口
    ChatClient* chatClient = new ChatClient(&app);
    LoginWindow* loginWindow = new LoginWindow(chatClient);
    RegisterWindow* registerWindow = new RegisterWindow(chatClient);
    ChatWindow* currentChatWindow = nullptr;
    // 连接信号
    QObject::connect(loginWindow, &LoginWindow::showRegisterWindow,
                     [loginWindow, registerWindow]()
                     {
                         loginWindow->hide();
                         registerWindow->show();
                     });
    QObject::connect(registerWindow, &RegisterWindow::showLoginWindow,
                     [registerWindow, loginWindow]()
                     {
                         registerWindow->hide();
                         loginWindow->show();
                     });
    // ! change
    QObject::connect(loginWindow, &LoginWindow::loginSuccessful,
                     [loginWindow, chatClient, &currentChatWindow](const QString& username,
                                                                   const QString& nickname)
                     {
                         try
                         {
                             qDebug() << "Main: Handling loginSuccessful, "
                                      << "username: " << username << ", nickname: " << nickname;
                             loginWindow->hide();

                             // 如果已有ChatWindow实例，先销毁它
                             if (currentChatWindow)
                             {
                                 delete currentChatWindow;
                                 currentChatWindow = nullptr;
                             }

                             // 创建新的ChatWindow
                             currentChatWindow = new ChatWindow(chatClient);
                             currentChatWindow->setAttribute(Qt::WA_DeleteOnClose);

                             // 监听窗口关闭信号，更新指针
                             QObject::connect(currentChatWindow, &ChatWindow::destroyed,
                                              [&currentChatWindow, &loginWindow]()
                                              {
                                                  currentChatWindow = nullptr;
                                                  loginWindow->show();
                                                  // ! change
                                                  // todo 还是有问题
                                              });

                             qDebug() << "Main: Showing ChatWindow";
                             currentChatWindow->show();
                         }
                         catch (const std::exception& e)
                         {
                             qDebug() << "Main: Exception in loginSuccessful:" << e.what();
                             QMessageBox::critical(nullptr, "错误",
                                                   QString("创建聊天窗口失败: %1").arg(e.what()));
                             loginWindow->show();
                         }
                         catch (...)
                         {
                             qDebug() << "Main: Unknown exception in loginSuccessful";
                             QMessageBox::critical(nullptr, "错误", "创建聊天窗口时发生未知错误");
                             loginWindow->show();
                         }
                     });
    QObject::connect(chatClient, &ChatClient::disconnected,
                     [loginWindow]()
                     {
                         qDebug() << "Main: ChatClient disconnected";
                         loginWindow->show();
                     });

    QLoggingCategory::setFilterRules("qt.widgets.style=true");
    // 加载样式表
    QFile styleFile(":/styles/styles.qss");

    if (styleFile.open(QFile::ReadOnly))
    {
        QString styleSheet = styleFile.readAll();
        app.setStyleSheet(styleSheet);
        qDebug() << "Successfully loaded styles.qss";
    }
    else
    {
        qWarning() << "Could not open styles.qss: " << styleFile.errorString();
    }

    // 连接服务器：使用 ConfigManager 中的最终 TCP host 和 port
    qDebug() << "Main: Connecting to TCP server at" << ConfigManager::instance().tcpHost() << ":"
             << ConfigManager::instance().tcpPort();
    chatClient->connectToServer(ConfigManager::instance().tcpHost(),
                                ConfigManager::instance().tcpPort());

    loginWindow->show();

    return app.exec();
}