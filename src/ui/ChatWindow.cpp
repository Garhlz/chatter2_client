#include "ChatWindow.h"
#include <QDateTime>
#include <QDebug>
#include <QException>
#include <QFile>
#include <QGuiApplication>
#include <QJsonObject>
#include <QMessageBox>
#include <QScreen>
#include <QStatusBar>
#include <QVBoxLayout>
#include "GlobalEventBus.h"
#include <QJsonDocument>
#include "utils/UserInfo.h"
ChatWindow::ChatWindow(ChatClient* client, QWidget* parent)
    : QMainWindow(parent),
      chatClient(client),
      onlineNumbers(0),
      offlineNumbers(0),
      curUsername(UserInfo::instance().username()),
      nickname(UserInfo::instance().nickname()),
      isInitialized(false)
{
    if (!chatClient || nickname.isEmpty())
    {
        qDebug() << "ChatWindow: Invalid client or nickname";
        throw std::runtime_error("无效的客户端或昵称");
    }
    try
    {
        qDebug() << "ChatWindow: Starting setupUi";
        setupUi();
        qDebug() << "ChatWindow: Starting connectSignals";
        connectSignals();
        qDebug() << "ChatWindow: Setting window title";
        setWindowTitle("聊天客户端 - " + nickname);
        qDebug() << "ChatWindow: Initialization completed";
        isInitialized = true;
    }
    catch (const QException& e)
    {
        qDebug() << "ChatWindow: Qt exception during initialization:" << e.what();
        QMessageBox::critical(this, "错误", QString("初始化失败: %1").arg(e.what()));
        throw;
    }
    catch (const std::exception& e)
    {
        qDebug() << "ChatWindow: Exception during initialization:" << e.what();
        QMessageBox::critical(this, "错误", QString("初始化失败: %1").arg(e.what()));
        throw;
    }
    catch (...)
    {
        qDebug() << "ChatWindow: Unknown exception during initialization";
        QMessageBox::critical(this, "错误", "初始化时发生未知错误");
        throw;
    }
}

ChatWindow::~ChatWindow()
{
    qDebug() << "ChatWindow: Destructor called";
}

void ChatWindow::setupUi()
{
    try
    {
        qDebug() << "ChatWindow: Creating central widget";
        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        qDebug() << "ChatWindow: Creating main layout";
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(15, 15, 15, 15);
        mainLayout->setSpacing(12);

        qDebug() << "ChatWindow: Creating tab widget";
        chatTabs = new QTabWidget(this);
        chatTabs->setObjectName("chatTabs");
        mainLayout->addWidget(chatTabs);

        qDebug() << "ChatWindow: Init user manager";
        userManager = new UserManager(this);

        qDebug() << "ChatWindow: Setting up tabs";
        publicChatTab = new PublicChatTab(chatClient, nickname, this);
        privateChatTab = new PrivateChatTab(chatClient, curUsername, nickname, userManager, this);
        groupChatTab = new GroupChatTab(chatClient, nickname, userManager, this);

        chatTabs->addTab(publicChatTab, "公共聊天");
        chatTabs->addTab(privateChatTab, "私聊");
        chatTabs->addTab(groupChatTab, "群聊");

        qDebug() << "ChatWindow: Setting up status bar";
        QStatusBar* statusBar = new QStatusBar(this);
        statusBar->setObjectName("statusBar");
        statusLabel = new QLabel("已连接");
        statusLabel->setObjectName("statusLabel");
        onlineCountLabel = new QLabel("在线人数: 0");
        onlineCountLabel->setObjectName("onlineCountLabel");
        QPushButton* logoutButton = new QPushButton("登出");
        logoutButton->setObjectName("logoutButton");
        statusBar->addWidget(statusLabel);
        statusBar->addWidget(onlineCountLabel);
        statusBar->addPermanentWidget(logoutButton);
        setStatusBar(statusBar);

        qDebug() << "ChatWindow: Setting window size and style";
        resize(1000, 750);
        setObjectName("ChatWindow");

        qDebug() << "ChatWindow: Centering window";
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen)
        {
            QRect screenGeometry = screen->availableGeometry();
            QSize windowSize = size();
            int x = (screenGeometry.width() - windowSize.width()) / 2;
            int y = (screenGeometry.height() - windowSize.height()) / 2;
            move(x, y);
        }

        // Ensure stylesheet is applied
        QFile styleFile(":/styles/styles.qss");
        if (styleFile.open(QFile::ReadOnly))
        {
            setStyleSheet(styleFile.readAll());
            qDebug() << "ChatWindow: Applied stylesheet";
        }
        else
        {
            qWarning() << "ChatWindow: Could not open styles.qss: " << styleFile.errorString();
        }

        qDebug() << "ChatWindow: setupUi completed";
    }
    catch (const QException& e)
    {
        qDebug() << "ChatWindow: Qt exception in setupUi:" << e.what();
        throw;
    }
    catch (const std::exception& e)
    {
        qDebug() << "ChatWindow: Exception in setupUi:" << e.what();
        throw;
    }
    catch (...)
    {
        qDebug() << "ChatWindow: Unknown exception in setupUi";
        throw std::runtime_error("Unknown error in setupUi");
    }
}

void ChatWindow::connectSignals()
{
    try
    {
        qDebug() << "ChatWindow: Connecting signals";
        connect(chatClient, &ChatClient::messageReceived, this, &ChatWindow::handleMessageReceived);
        connect(chatClient, &ChatClient::privateMessageReceived, this,
                &ChatWindow::handlePrivateMessageReceived);
        // 用户状态相关
        connect(chatClient, &ChatClient::onlineUsersInit, this, &ChatWindow::handleOnlineUsersInit);
        connect(chatClient, &ChatClient::offlineUsersInit, this,
                &ChatWindow::handleOfflineUsersInit);

        connect(chatClient, &ChatClient::someoneLogin, this, &ChatWindow::handleSomeoneLogin);
        connect(chatClient, &ChatClient::someoneLogout, this, &ChatWindow::handleSomeoneLogout);

        connect(chatClient, &ChatClient::historyMessagesReceived, this,
                &ChatWindow::handleHistoryMessagesReceived);
        connect(chatClient, &ChatClient::errorOccurred, this, &ChatWindow::handleError);
        connect(statusBar()->findChild<QPushButton*>(), &QPushButton::clicked, this,
                &ChatWindow::handleLogout);

        // 连接UserManager的信号到ChatWindow的UI更新槽
        connect(userManager, &UserManager::usersInitialized, this,
                &ChatWindow::updateUserCountsDisplay);
        connect(userManager, &UserManager::userStatusChanged, this,
                &ChatWindow::updateUserCountsDisplay);
        connect(userManager, &UserManager::userAdded, this, &ChatWindow::updateUserCountsDisplay);
        connect(userManager, &UserManager::userRemoved, this, &ChatWindow::updateUserCountsDisplay);
        isInitialized = true;  // 假设UI和Manager都已准备好
        qDebug() << "ChatWindow: Signals connected";
    }
    catch (const QException& e)
    {
        qDebug() << "ChatWindow: Qt exception in connectSignals:" << e.what();
        throw;
    }
    catch (const std::exception& e)
    {
        qDebug() << "ChatWindow: Exception in connectSignals:" << e.what();
        throw;
    }
    catch (...)
    {
        qDebug() << "ChatWindow: Unknown exception in connectSignals";
        throw std::runtime_error("Unknown error in connectSignals");
    }
}

void ChatWindow::handleMessageReceived(const QString& sender, const QString& content,
                                       qint64 messageId)
// 此处sender一定表示username
{
    if (!isInitialized)
    {
        qDebug() << "ChatWindow: Ignoring messageReceived before initialization";
        return;
    }
    if (messageId > 0 && displayedMessages.contains(messageId)) return;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    publicChatTab->appendMessage(sender, content, timestamp);
    if (messageId > 0) displayedMessages.insert(messageId);
}

void ChatWindow::handlePrivateMessageReceived(const QString& sender, const QString& receiver,
                                              const QString& content, qint64 messageId)
// 注意sender一定表示username而不是nickname
{
    if (!isInitialized)
    {
        qDebug() << "ChatWindow: Ignoring privateMessageReceived before initialization";
        return;
    }
    if (messageId > 0 && displayedMessages.contains(messageId)) return;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    privateChatTab->appendMessage(sender, receiver, content, timestamp, false);
    chatTabs->setCurrentWidget(privateChatTab);
    if (messageId > 0) displayedMessages.insert(messageId);
}

void ChatWindow::handleHistoryMessagesReceived(const QJsonArray& messages)
{
    if (!isInitialized)
    {
        qDebug() << "ChatWindow: Ignoring historyMessagesReceived before initialization";
        return;
    }

    for (const QJsonValue& msg : messages)
    {
        if (!msg.isObject()) continue;
        QJsonObject message = msg.toObject();

        if (!message.contains("type") || !message.contains("nickname") ||
            !message.contains("content"))
            continue;

        QString type = message["type"].toString();
        QString sender = message["nickname"].toString();
        QString senderUsername = message["username"].toString();

        // 如果是文件信息直接放置文件的具体内容
        QString content = "";
        if (message["content"].isString() && type != "FILE")
        {
            content = message["content"].toString();
        }

        qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                               ? message["messageId"].toVariant().toLongLong()
                               : 0;

        if (messageId > 0 && displayedMessages.contains(messageId)) continue;

        QString timestamp =
            message.contains("timestamp") && !message["timestamp"].isNull()
                ? QDateTime::fromString(message["timestamp"].toString(), Qt::ISODate)
                      .toString("hh:mm:ss")
                : QDateTime::currentDateTime().toString("hh:mm:ss");

        if (type == "CHAT")
        {
            publicChatTab->appendMessage(sender, content, timestamp);
        }
        else if (type == "PRIVATE_CHAT")
        {
            QString receiver = message["receiver"].toString();
            privateChatTab->appendMessage(senderUsername, receiver, content, timestamp, false);
        }
        else if (type == "GROUP_CHAT")  // 处理历史消息中的群组消息
        {
            if (!message.contains("groupId")) continue;
            QString senderUsername = message["username"].toString();
            QString senderNickname = message["nickname"].toString();
            long groupId = message["groupId"].toVariant().toLongLong();
            QString content = message["content"].toString();

            QString timestamp =
                message.contains("timestamp") && !message["timestamp"].isNull()
                    ? QDateTime::fromString(message["timestamp"].toString(), Qt::ISODate)
                          .toString("hh:mm:ss")
                    : QDateTime::currentDateTime().toString("hh:mm:ss");

            groupChatTab->appendMessage(senderUsername, senderNickname, groupId, content,
                                        timestamp);
        }
        else if (type == "FILE")
        {
            QString receiver = message["receiver"].toString();

            // 这里的message本身就是后端的FileAttachment
            QJsonDocument doc = QJsonDocument::fromJson(message["content"].toString().toUtf8());
            QJsonObject fileInfo = doc.object();
            privateChatTab->appendMessage(senderUsername, receiver, fileInfo, timestamp, true);
        }
        // 其实最好还是不要用总线, 这里逻辑已经写好了...

        if (messageId > 0) displayedMessages.insert(messageId);
    }
}

void ChatWindow::handleLogout()
{
    chatClient->logout();
    close();
    // todo 显示登录窗口
}

void ChatWindow::handleError(const QString& error)
{
    statusLabel->setText(error);
    QMessageBox::warning(this, "错误", error);
}

void ChatWindow::appendMessageBubble(QWidget* container, const QString& sender,
                                     const QString& content, const QString& timestamp,
                                     const QString& avatar)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout)
    {
        qDebug() << "ChatWindow: Invalid container layout";
        return;
    }

    if (layout->count() > 0)
    {
        QLayoutItem* item = layout->itemAt(layout->count() - 1);
        if (item->spacerItem())
        {
            layout->removeItem(item);
            delete item;
        }
    }

    MessageBubble* bubble =
        new MessageBubble(avatar, sender, content, timestamp, sender == nickname, container);
    layout->addWidget(bubble);
    layout->addStretch();
}

// --用户状态相关--

// 处理初始在线用户列表
void ChatWindow::handleOnlineUsersInit(const QJsonArray& users)
{
    if (!isInitialized)
    {
        qDebug() << "ChatWindow: Ignoring onlineUsersInit before initialization";
        return;
    }
    userManager->initOnlineUsers(users);  // 传递给 UserManager 处理
    m_initialOnlineLoaded = true;         // 标记在线列表已加载

    // 检查是否所有初始数据都已加载
    if (m_initialOnlineLoaded && m_initialOfflineLoaded)
    {
        userManager->markInitialDataLoaded();  // 通知 UserManager 初始数据加载完成
        // 可选：重置标志，如果你的应用支持多次完整的初始化流程
        // m_initialOnlineLoaded = false;
        // m_initialOfflineLoaded = false;
    }
    qDebug() << "ChatWindow: 初始在线用户列表已处理。";
}

// 处理初始离线用户列表
void ChatWindow::handleOfflineUsersInit(const QJsonArray& users)
{
    if (!isInitialized)
    {
        qDebug() << "ChatWindow: Ignoring offlineUsersInit before initialization";
        return;
    }
    userManager->initOfflineUsers(users);  // 传递给 UserManager 处理
    m_initialOfflineLoaded = true;         // 标记离线列表已加载

    // 检查是否所有初始数据都已加载
    if (m_initialOnlineLoaded && m_initialOfflineLoaded)
    {
        userManager->markInitialDataLoaded();  // 通知 UserManager 初始数据加载完成
        // 可选：重置标志
        // m_initialOnlineLoaded = false;
        // m_initialOfflineLoaded = false;
    }
    qDebug() << "ChatWindow: 初始离线用户列表已处理。";
}

// 处理用户登录事件 (连接到 ChatClient 发出的信号)
void ChatWindow::handleSomeoneLogin(const QJsonObject& loginUser)
{
    // 传递给 UserManager 处理，UserManager 会更新内部数据并发射信号
    userManager->handleUserStatusChange(loginUser, User::Online);  // 登录状态对应 User::Online (1)
    // ChatWindow 的 UI 统计会通过连接 userManager 信号的 updateUserCountsDisplay 槽自动更新
    qDebug() << "ChatWindow: 收到用户登录通知，已转发给 UserManager。";
}

// 处理用户登出事件 (连接到 ChatClient 发出的信号)
void ChatWindow::handleSomeoneLogout(const QJsonObject& logoutUser)
{
    // 传递给 UserManager 处理，UserManager 会更新内部数据并发射信号
    userManager->handleUserStatusChange(logoutUser,
                                        User::Offline);  // 登出状态对应 User::Offline (0)
    // ChatWindow 的 UI 统计会通过连接 userManager 信号的 updateUserCountsDisplay 槽自动更新
    qDebug() << "ChatWindow: 收到用户登出通知，已转发给 UserManager。";
}

// 更新在线/离线人数显示 (连接到UserManager的信号)
void ChatWindow::updateUserCountsDisplay()
{
    onlineNumbers = userManager->getOnlineNumber();
    offlineNumbers = userManager->getOfflineNumber();
    // busyNumbers = userManager->getBusyNumber(); // 如果有忙碌人数标签
    onlineCountLabel->setText(QString("在线人数: %1").arg(onlineNumbers));
    // 如果有 offlineCountLabel 或 busyCountLabel，也在这里更新
    qDebug() << "ChatWindow: 人数统计更新 -> 在线: " << onlineNumbers
             << ", 离线: " << offlineNumbers << ", 忙碌: " << userManager->getBusyNumber();
}