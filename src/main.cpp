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
    QObject::connect(loginWindow, &LoginWindow::loginSuccessful,
                     [loginWindow, chatClient](const QString& username, const QString& nickname)
                     {
                         try
                         {
                             qDebug() << "Main: Handling loginSuccessful, "
                                      << "username: " << username << ", nickname: " << nickname;
                             loginWindow->hide();
                             ChatWindow* newChatWindow =
                                 new ChatWindow(chatClient, username, nickname);

                             newChatWindow->setAttribute(Qt::WA_DeleteOnClose);
                             qDebug() << "Main: Showing ChatWindow";
                             newChatWindow->show();
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

    // // --- 模拟测试代码 (如果你不需要，可以删除) ---
    // // 为了测试 FileTransferManager/HttpRequestManager，设置一个模拟用户
    // UserInfo::instance().setUsername("mock_user");
    // UserInfo::instance().setToken("mock_token_123");

    // // 模拟从服务器收到的文件消息 JSON
    // QJsonObject fileInfo;
    // fileInfo["filename"] = "test_document.txt";
    // fileInfo["size"] = 54321; // 字节
    // fileInfo["type"] = "text/plain";
    // // 注意：这里的 URL 需要是你的 HTTP 服务器实际可访问的地址
    // fileInfo["downloadUrl"] = QString("http://%1:%2/files/test_document.txt")
    //                               .arg(ConfigManager::instance().httpHost())
    //                               .arg(ConfigManager::instance().httpPort());

    // QJsonObject fileMessageObj;
    // fileMessageObj["type"] = "file"; // 这是一个文件消息
    // fileMessageObj["sender"] = "Alice";
    // fileMessageObj["receiver"] = "mock_user";
    // fileMessageObj["fileInfo"] = fileInfo; // 文件元数据
    // fileMessageObj["messageId"] = 1001;
    // fileMessageObj["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");

    // QJsonDocument fileMessageDoc(fileMessageObj);

    // // 模拟 MessageProcessor 接收到并发出通用消息信号
    // // 如果 ChatWindow 包含 PrivateChatSession，那么它应该能接收到这个信号
    // QTimer::singleShot(2000, [fileMessageDoc]() {
    //     qDebug() << "Main: Simulating incoming file JSON (from Alice)...";
    //     // 假设你有一个 MessageProcessor 实例，并连接到 GlobalEventBus
    //     // 这里只是直接发射信号，实际应该由 MessageProcessor 负责
    //     QJsonObject obj = fileMessageDoc.object();
    //     GlobalEventBus::instance()->incomingGeneralMessage(
    //         obj["sender"].toString(),
    //         obj["receiver"].toString(),
    //         obj["fileInfo"].toObject(), // 传递 QJsonObject 作为 QJsonValue
    //         obj["messageId"].toVariant().toLongLong(),
    //         obj["timestamp"].toString()
    //     );
    // });

    // // 模拟用户点击 "文件" 按钮，并进行文件上传
    // QTimer::singleShot(8000, []() {
    //     qDebug() << "Main: Simulating a file upload via HttpRequestManager.";
    //     // **请将 "C:/path/to/your/local_test_file.txt" 替换为你的电脑上实际存在的文件路径**
    //     QString mockLocalFilePath = "C:/Users/YourUser/Desktop/test_upload.txt"; //
    //     替换为真实路径 if (!QFile::exists(mockLocalFilePath)) {
    //         qWarning() << "Mock upload file does not exist:" << mockLocalFilePath << ". Skipping
    //         simulated upload."; return;
    //     }

    //     QUrl uploadUrl;
    //     uploadUrl.setScheme("http");
    //     uploadUrl.setHost(ConfigManager::instance().httpHost());
    //     uploadUrl.setPort(ConfigManager::instance().httpPort());
    //     uploadUrl.setPath(ConfigManager::instance().apiPrefix() + "/files/upload"); // 使用
    //     apiPrefix

    //     QVariantMap formData;
    //     formData["senderUsername"] = UserInfo::instance().username();
    //     formData["receiverUsername"] = "Bob"; // 模拟发送给 Bob
    //     formData["token"] = UserInfo::instance().token();

    //     HttpRequestManager::instance()->uploadFile(uploadUrl, mockLocalFilePath, formData);
    // });
    // --- 模拟测试代码结束 ---

    return app.exec();
}