#include "network/ChatClient.h"
#include "ui/LoginWindow.h"
#include "ui/RegisterWindow.h"
#include "ui/ChatWindow.h"  // 仍然需要包含，因为 ChatWindow 在 WindowManager 中使用
#include "utils/ConfigManager.h"
#include "FileTransferManager.h"
#include "GlobalEventBus.h"
// #include "utils/UserInfo.h" // 如果不再直接在 main 中使用，可以移除
#include "WindowManager.h"  // 新增

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QFile>
#include <QLoggingCategory>
#include <QMessageBox>
// #include <QTimer> // 如果不再直接在 main 中使用，可以移除

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 1. 初始化单例
    GlobalEventBus::instance();
    FileTransferManager::instance();  // 文件传输类 (即 HttpRequestManager) 初始化

    // 2. 加载配置文件。即使失败，ConfigManager 也会设置默认值。
    if (!ConfigManager::instance().loadConfig(":/config.json"))
    {
        qWarning() << "无法加载配置文件或配置有误，将使用默认配置。";
    }

    // 3. 获取ConfigManager中的当前（或默认）配置值作为命令行参数的默认值
    QString defaultTcpHost = ConfigManager::instance().tcpHost();
    QString defaultTcpPortStr = QString::number(ConfigManager::instance().tcpPort());
    QString defaultHttpHost = ConfigManager::instance().httpHost();
    QString defaultHttpPortStr = QString::number(ConfigManager::instance().httpPort());
    QString defaultApiPrefix = ConfigManager::instance().apiPrefix();

    // 4. 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("Chat Client");
    parser.addHelpOption();
    parser.addVersionOption();

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

    // 5. 获取命令行参数的值，并将其更新到 ConfigManager 中
    QString finalTcpHost = parser.value(tcpHostOption);
    QString finalTcpPortStr = parser.value(tcpPortOption);
    QString finalHttpHost = parser.value(httpHostOption);
    QString finalHttpPortStr = parser.value(httpPortOption);
    QString finalApiPrefix = parser.value(apiPrefixOption);

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

    // 6. 更新 ConfigManager 的值，确保整个应用获取的是最终配置
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

    // 7. 创建 ChatClient 实例
    ChatClient* chatClient = new ChatClient(&app);  // 将 app 作为父对象，确保其生命周期受控

    // 8. 创建并启动 WindowManager
    WindowManager windowManager(chatClient);
    windowManager.startApplication();  // 由 WindowManager 管理初始窗口显示和连接尝试

    // 9. 加载样式表
    QLoggingCategory::setFilterRules("qt.widgets.style=true");
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

    // 10. 进入事件循环
    return app.exec();
}