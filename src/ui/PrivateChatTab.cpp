// ui/PrivateChatTab.cpp
#include "PrivateChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QStackedWidget>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

#include "PrivateChatSession.h"
#include "GlobalEventBus.h"
#include "network/ChatClient.h"

// 定义列表项数据角色
enum UserListItemDataRole
{
    UsernameRole = Qt::UserRole,       // 存储用户的username (QString)
    UserIdRole = Qt::UserRole + 1,     // 存储用户的user_id (long)
    UserStatusRole = Qt::UserRole + 2  // 存储用户的状态 (int, 对应User::UserStatus)
};

PrivateChatTab::PrivateChatTab(ChatClient* client, const QString& username, const QString& nickname,
                               UserManager* userManager_, QWidget* parent)
    : QWidget(parent),
      chatClient(client),
      curUsername(username),
      curNickname(nickname),
      userManager(userManager_)  // 初始化userManager成员
{
    setupUi();
    connectSignals();

    // 初始时不需要调用 refreshUserLists(); 等待 usersInitialized 信号
}

void PrivateChatTab::setupUi()
{
    setObjectName("PrivateChatTab");
    QVBoxLayout* privateTabLayout = new QVBoxLayout(this);
    privateTabLayout->setContentsMargins(8, 8, 8, 8);
    privateTabLayout->setSpacing(10);

    QSplitter* privateSplitter = new QSplitter(Qt::Horizontal);
    privateSplitter->setObjectName("privateSplitter");

    QWidget* chatWidget = new QWidget();
    QHBoxLayout* privateChatLayout = new QHBoxLayout(chatWidget);
    privateChatLayout->setContentsMargins(0, 0, 0, 0);

    sessionList = new QListWidget();
    sessionList->setObjectName("sessionList");
    sessionList->setMaximumWidth(200);
    sessionList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sessionList->setSelectionMode(QAbstractItemView::SingleSelection);
    privateChatLayout->addWidget(sessionList);

    sessionStack = new QStackedWidget();
    sessionStack->setObjectName("sessionStack");
    privateChatLayout->addWidget(sessionStack);

    QWidget* usersWidget = new QWidget();
    QVBoxLayout* usersLayout = new QVBoxLayout(usersWidget);
    usersLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* usersLabel = new QLabel("在线用户");
    usersLabel->setObjectName("usersLabel");
    onlineUsersList = new QListWidget();
    onlineUsersList->setObjectName("onlineUsersList");
    onlineUsersList->setSelectionMode(QAbstractItemView::SingleSelection);
    onlineUsersList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(onlineUsersList);

    QLabel* offlineUsersLabel = new QLabel("离线用户");
    offlineUsersLabel->setObjectName("usersLabel");
    offlineUsersList = new QListWidget();
    offlineUsersList->setObjectName("offlineUsersList");
    offlineUsersList->setSelectionMode(QAbstractItemView::SingleSelection);
    offlineUsersList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    usersLayout->addWidget(offlineUsersLabel);
    usersLayout->addWidget(offlineUsersList);

    privateSplitter->addWidget(chatWidget);
    privateSplitter->addWidget(usersWidget);
    privateSplitter->setSizes({800, 200});
    privateTabLayout->addWidget(privateSplitter);
}

void PrivateChatTab::connectSignals()
{
    connect(onlineUsersList, &QListWidget::itemClicked, this, &PrivateChatTab::handleUserSelected);
    connect(offlineUsersList, &QListWidget::itemClicked, this, &PrivateChatTab::handleUserSelected);

    connect(onlineUsersList, &QListWidget::itemDoubleClicked, this,
            &PrivateChatTab::handleUserSelected);
    connect(offlineUsersList, &QListWidget::itemDoubleClicked, this,
            &PrivateChatTab::handleUserSelected);

    connect(sessionList, &QListWidget::itemClicked, this, &PrivateChatTab::handleSessionSelected);
    connect(GlobalEventBus::instance(), &GlobalEventBus::globalAppendMessage, this,
            &PrivateChatTab::appendMessage);

    // **连接 UserManager 的信号**
    connect(userManager, &UserManager::usersInitialized, this, &PrivateChatTab::onUsersInitialized);
    connect(userManager, &UserManager::userStatusChanged, this,
            &PrivateChatTab::onUserStatusChanged);
    connect(userManager, &UserManager::userAdded, this, &PrivateChatTab::onUserAdded);
}

void PrivateChatTab::handleUserSelected(QListWidgetItem* item)
{
    if (!item) return;
    QString targetUsername = item->data(UsernameRole).toString();

    QListWidgetItem* currentSessionItem = sessionList->currentItem();
    if (currentSessionItem && currentSessionItem->data(UsernameRole).toString() == targetUsername)
    {
        return;
    }
    PrivateChatSession* session = getOrCreateSession(targetUsername);

    if (session)
    {
        session->scrollToBottom();
        for (int i = 0; i < sessionList->count(); ++i)
        {
            if (sessionList->item(i)->data(UsernameRole).toString() == targetUsername)
            {
                sessionList->setCurrentItem(sessionList->item(i));
                break;
            }
        }
    }
}

void PrivateChatTab::handleSessionSelected(QListWidgetItem* item)
{
    if (!item) return;
    QString targetUsername = item->data(UsernameRole).toString();

    PrivateChatSession* session = sessions.value(targetUsername, nullptr);

    if (session)
    {
        sessionStack->setCurrentWidget(session);
        session->scrollToBottom();
    }
}

PrivateChatSession* PrivateChatTab::getOrCreateSession(const QString& targetUsername)
{
    if (targetUsername == curUsername)
    {
        qDebug() << "不能与自己创建会话: " << targetUsername;
        return nullptr;
    }
    if (sessions.contains(targetUsername))
    {
        return sessions[targetUsername];
    }
    // 从UserManager获取用户信息
    User* targetUser = userManager->getUserByUsername(targetUsername);
    if (!targetUser)
    {
        qWarning() << "无法找到用户: " << targetUsername << "来创建会话。";
        return nullptr;
    }
    QString targetNickname = targetUser->getNickname();

    PrivateChatSession* session = new PrivateChatSession(chatClient, curUsername, curNickname,
                                                         targetUsername, targetNickname, this);
    sessions[targetUsername] = session;

    sessionStack->addWidget(session);

    QString displayText = targetNickname.isEmpty() ? targetUsername : targetNickname;
    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(UsernameRole, targetUsername);  // 存储username
    sessionList->addItem(item);
    sessionList->setCurrentItem(item);

    sessionStack->setCurrentWidget(session);
    session->scrollToBottom();

    connect(session, &PrivateChatSession::sendMessageRequested, this,
            [=](const QString& target, const QString& content)
            { chatClient->sendPrivateMessage(target, content); });
    return session;
}

PrivateChatSession* PrivateChatTab::getOrCreateSessionTwo(const QString& sender,
                                                          const QString& receiver)
{
    QString targetUsername;
    if (sender == curUsername)
        targetUsername = receiver;
    else
        targetUsername = sender;
    return getOrCreateSession(targetUsername);
}

void PrivateChatTab::appendMessage(const QString& sender, const QString& receiver,
                                   const QJsonValue& content, const QString& timestamp, bool isFile)
{
    PrivateChatSession* session = getOrCreateSessionTwo(sender, receiver);
    if (session)
    {
        session->appendMessage(sender, receiver, content, timestamp, isFile);
    }
}

// 辅助函数：将用户添加到在线/离线列表 UI
void PrivateChatTab::addUserToListUI(User* user)
{
    if (!user) return;

    QListWidget* targetList = nullptr;
    switch (user->getStatus())
    {
        case User::Online:
            targetList = onlineUsersList;
            break;
        case User::Offline:
            targetList = offlineUsersList;
            break;
        case User::Busy:
            targetList = onlineUsersList;  // 假设忙碌用户也显示在在线列表，可以根据需求调整
            qDebug() << "User " << user->getUsername() << " is busy.";
            break;
        default:
            return;  // 未知状态不处理
    }

    if (!targetList) return;

    // 检查是否已存在，防止重复添加
    for (int i = 0; i < targetList->count(); ++i)
    {
        if (targetList->item(i)->data(UserIdRole).toLongLong() == user->getUserId())
        {
            return;  // 已经存在，不重复添加
        }
    }

    QString displayText = QString("%1 (%2)").arg(user->getNickname()).arg(user->getUsername());
    QListWidgetItem* item = new QListWidgetItem(displayText, targetList);
    item->setData(UsernameRole, user->getUsername());                    // 存储username
    item->setData(UserIdRole, static_cast<qint64>(user->getUserId()));   // 存储user_id
    item->setData(UserStatusRole, static_cast<int>(user->getStatus()));  // 存储状态

    // 根据状态设置颜色或图标
    switch (user->getStatus())
    {
        case User::Online:
            item->setForeground(QColor(Qt::darkGreen));
            break;
        case User::Offline:
            item->setForeground(QColor(Qt::gray));
            break;
        case User::Busy:
            item->setForeground(QColor(Qt::darkYellow));
            break;  // 忙碌用户显示黄色
    }
    // qDebug() << "PrivateChatTab: UI - 将 " << user->getUsername() << " 添加到 "
    //          << (user->getStatus() == User::Online
    //                  ? "在线"
    //                  : (user->getStatus() == User::Offline ? "离线" : "忙碌"))
    //          << "列表。";
}

// 辅助函数：将用户从指定列表 UI 移除
void PrivateChatTab::removeUserFromListUI(long userId, QListWidget* targetList)
{
    if (!targetList) return;

    for (int i = 0; i < targetList->count(); ++i)
    {
        QListWidgetItem* item = targetList->item(i);
        if (item->data(UserIdRole).toLongLong() == userId)
        {
            // qDebug() << "PrivateChatTab: UI - 从 " << targetList->objectName() << " 移除 "
            //          << item->data(UsernameRole).toString();
            delete targetList->takeItem(i);  // 移除并删除项
            return;
        }
    }
}

// **UserManager 初始化完成后的槽函数**
void PrivateChatTab::onUsersInitialized()
{
    refreshUserLists();  // 收到初始化信号后，刷新整个列表
    qDebug() << "PrivateChatTab: Received usersInitialized signal. Refreshing UI.";
}

// **响应 UserManager::userStatusChanged 信号**
void PrivateChatTab::onUserStatusChanged(User* user)
{
    if (!user) return;

    // 先从两个列表中都移除，确保只存在于正确的位置
    removeUserFromListUI(user->getUserId(), onlineUsersList);
    removeUserFromListUI(user->getUserId(), offlineUsersList);

    // 再根据最新状态添加到对应的列表
    addUserToListUI(user);
    // qDebug() << "PrivateChatTab: UI - 响应用户状态变化通知: " << user->getUsername() << " -> "
    //          << user->getStatus();
}

// **响应 UserManager::userAdded 信号 (新用户首次加入)**
void PrivateChatTab::onUserAdded(User* user)
{
    if (!user) return;
    // 新用户直接添加到对应列表
    addUserToListUI(user);
    // qDebug() << "PrivateChatTab: UI - 响应新用户加入通知: " << user->getUsername();
}

// 辅助函数：刷新整个列表
void PrivateChatTab::refreshUserLists()
{
    onlineUsersList->clear();
    offlineUsersList->clear();

    QMap<long, User*> allUsers = userManager->getAllUsers();
    for (User* user : allUsers.values())
    {
        addUserToListUI(user);  // 将所有用户按其状态添加到对应列表
    }
    // qDebug() << "PrivateChatTab: UI - 用户列表已从 UserManager 刷新。";
}