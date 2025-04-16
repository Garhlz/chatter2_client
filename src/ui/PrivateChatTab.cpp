#include "PrivateChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <QDebug.h>

PrivateChatTab::PrivateChatTab(ChatClient* client, const QString& username, const QString& nickname,
                               QWidget* parent)
    : QWidget(parent), chatClient(client), curUsername(username), curNickname(nickname)
{
    setupUi();
    connectSignals();
}

void PrivateChatTab::setupUi()
{
    QVBoxLayout* privateTabLayout = new QVBoxLayout(this);
    privateTabLayout->setContentsMargins(8, 8, 8, 8);
    privateTabLayout->setSpacing(10);

    QSplitter* privateSplitter = new QSplitter(Qt::Horizontal);
    privateSplitter->setObjectName("privateSplitter");

    QWidget* chatWidget = new QWidget();
    QVBoxLayout* privateChatLayout = new QVBoxLayout(chatWidget);
    privateChatLayout->setContentsMargins(0, 0, 0, 0);
    chatSessions = new QTabWidget();
    chatSessions->setObjectName("chatSessions");
    chatSessions->setTabsClosable(false);
    privateChatLayout->addWidget(chatSessions);

    QWidget* usersWidget = new QWidget();
    QVBoxLayout* usersLayout = new QVBoxLayout(usersWidget);
    usersLayout->setContentsMargins(0, 0, 0, 0);

    // 在线
    QLabel* usersLabel = new QLabel("在线用户");
    usersLabel->setObjectName("usersLabel");
    onlineUsersList = new QListWidget();
    onlineUsersList->setObjectName("onlineUsersList");
    onlineUsersList->setMaximumWidth(200);
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(onlineUsersList);

    // 离线
    QLabel* offlineUsersLabel = new QLabel("离线用户");
    offlineUsersLabel->setObjectName("offlineUsersLabel");
    offlineUsersList = new QListWidget();
    offlineUsersList->setObjectName("offlineUsersList");
    offlineUsersList->setMaximumWidth(200);
    usersLayout->addWidget(offlineUsersLabel);
    usersLayout->addWidget(offlineUsersList);

    privateSplitter->addWidget(chatWidget);
    privateSplitter->addWidget(usersWidget);
    privateSplitter->setSizes({800, 200});
    privateTabLayout->addWidget(privateSplitter);
}

void PrivateChatTab::connectSignals()
{
    // 点击就创建或者切换到目标会话
    connect(onlineUsersList, &QListWidget::itemClicked, this, &PrivateChatTab::handleUserSelected);
    connect(offlineUsersList, &QListWidget::itemClicked, this, &PrivateChatTab::handleUserSelected);
}

void PrivateChatTab::handleUserSelected(QListWidgetItem* item)
{
    if (!item) return;
    QString targetUser = item->data(Qt::UserRole).toString();
    qDebug() << "选中 user: " << targetUser;
    PrivateChatSession* session = getOrCreateSession(targetUser);
    // 此处只需要一个参数
    chatSessions->setCurrentWidget(session);
}

PrivateChatSession* PrivateChatTab::getOrCreateSession(const QString& targetUser)
{
    if (sessions.contains(targetUser))
    {
        return sessions[targetUser];
    }
    // QString targetUsername = allUsers[targetUser]["username"].toString();
    // targetUser已经表示username了
    QString targetNickname = allUsers[targetUser]["nickname"].toString();

    PrivateChatSession* session = new PrivateChatSession(chatClient, curUsername, curNickname,
                                                         targetUser, targetNickname, this);
    sessions[targetUser] = session;
    int index = chatSessions->addTab(session, targetUser);
    chatSessions->setTabToolTip(index, QString("Chat with %1").arg(targetUser));

    // Connect session signals to ChatClient
    connect(session, &PrivateChatSession::sendMessageRequested, this,
            [=](const QString& target, const QString& content)
            { chatClient->sendPrivateMessage(target, content); });

    connect(session, &PrivateChatSession::sendFileRequested, this,
            [=](const QString& target, const QByteArray& content)
            { chatClient->sendFile(target, content); });
    // qDebug() << "发送对象user: " << targetUser;

    return session;
}

PrivateChatSession* PrivateChatTab::getOrCreateSessionTwo(const QString& sender,
                                                          const QString& receiver)
{
    QString targetUser;
    if (sender == curUsername)
        targetUser = receiver;
    else
        targetUser = sender;
    return getOrCreateSession(targetUser);
}

void PrivateChatTab::appendMessage(const QString& sender, const QString& receiver,
                                   const QString& content, const QString& timestamp)
{  // sender表示username而不是nickname!!!
    qDebug() << "sender: " << sender << "receiver: " << receiver << " content: " << content;
    PrivateChatSession* session = getOrCreateSessionTwo(sender, receiver);
    session->appendMessage(sender, receiver, content, timestamp);
    // 这里也需要修改receiver
}

void PrivateChatTab::handleFileReceived(const QString& sender, const QString& receiver,
                                        const QByteArray& fileContent, const QString& timestamp)
{
    PrivateChatSession* session = getOrCreateSessionTwo(sender, receiver);
    session->handleFileReceived(sender, receiver, fileContent, timestamp);
}

// 把涉及onlinelist, offlinelist的逻辑全部提取出来, 单独处理
void PrivateChatTab::addToOnlineList(const QJsonObject& userObj)
{
    QString username = userObj["username"].toString();
    QString nickname = userObj["nickname"].toString();
    QString displayText = QString("%1 (%2)").arg(nickname).arg(username);
    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, username);
    qDebug() << "add online users: " << item << " data: " << item->data(Qt::UserRole).toString();
    onlineUsersList->addItem(item);
}

void PrivateChatTab::removeFromOnlineList(const QJsonObject& userObj)
{
    if (!userObj.contains("username"))
    {
        qDebug() << "removeFromOnlineList: Missing username in userObj";
        return;
    }

    QString username = userObj["username"].toString();
    for (int i = 0; i < onlineUsersList->count(); ++i)
    {
        QListWidgetItem* item = onlineUsersList->item(i);
        if (item->data(Qt::UserRole).toString() == username)
        {
            qDebug() << "Removing from online users: " << username;
            delete onlineUsersList->takeItem(i);
            break;
        }
    }
}

void PrivateChatTab::addToOfflineList(const QJsonObject& userObj)
{
    QString username = userObj["username"].toString();
    QString nickname = userObj["nickname"].toString();
    QString displayText = QString("%1 (%2)").arg(nickname).arg(username);
    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, username);
    qDebug() << "add offline users: " << item << " data: " << item->data(Qt::UserRole).toString();
    offlineUsersList->addItem(item);
}

void PrivateChatTab::removeFromOfflineList(const QJsonObject& userObj)
{
    if (!userObj.contains("username"))
    {
        qDebug() << "removeFromOfflineList: Missing username in userObj";
        return;
    }

    QString username = userObj["username"].toString();
    for (int i = 0; i < offlineUsersList->count(); ++i)
    {
        QListWidgetItem* item = offlineUsersList->item(i);
        if (item->data(Qt::UserRole).toString() == username)
        {
            qDebug() << "Removing from offline users: " << username;
            delete offlineUsersList->takeItem(i);
            break;
        }
    }
}

void PrivateChatTab::initOnlineUsers(const QJsonArray& users)  // 初始化在线用户列表
{
    onlineUsersList->clear();
    for (const QJsonValue& user : users)
    {
        QJsonObject userObj = user.toObject();
        addToOnlineList(userObj);
        userObj["isOnline"] = true;
        allUsers[userObj["username"].toString()] = userObj;
        onlineNumbers++;
    }
}

void PrivateChatTab::initOfflineUsers(const QJsonArray& users)  // 初始化离线用户列表
{
    offlineUsersList->clear();
    for (const QJsonValue& user : users)
    {
        QJsonObject userObj = user.toObject();
        addToOfflineList(userObj);
        userObj["isOnline"] = false;
        allUsers[userObj["username"].toString()] = userObj;
        offlineNumbers++;
    }
}
// 发现一个重大问题: 发送user类比只发送username更合适,因为username无法存储
// 已经解决, 0表示登录
void PrivateChatTab::someoneChange(int status, const QJsonObject& cur_user)
{
    if (status == 0)  // 上线
    {
        QString cur_username = cur_user["username"].toString();
        if (allUsers.contains(cur_username))
        {
            if (!allUsers[cur_username]["isOnline"].toBool())  // 取出都要写好多
            {
                allUsers[cur_username]["isOnline"] = true;
                onlineNumbers++;
                offlineNumbers--;
                qDebug() << cur_username << " 上线";
                addToOnlineList(cur_user);
                removeFromOfflineList(cur_user);
            }
            else
            {
                qDebug() << cur_username << " 重复登录";
            }
        }
        else
        {
            allUsers[cur_username] = cur_user;
            allUsers[cur_username]["isOnline"] = true;
            onlineNumbers++;
            qDebug() << cur_username << " 新增在线";
            addToOnlineList(cur_user);
        }
    }
    else  // 下线
    {
        QString cur_username = cur_user["username"].toString();
        if (allUsers.contains(cur_username))
        {
            if (allUsers[cur_username]["isOnline"].toBool())  // 当前在线
            {
                allUsers[cur_username]["isOnline"] = false;
                onlineNumbers--;
                offlineNumbers++;
                addToOfflineList(cur_user);
                removeFromOnlineList(cur_user);
            }
            else
            {
                qDebug() << cur_username << " 重复登出";
            }
        }
        else
        {
            allUsers[cur_username] = cur_user;
            allUsers[cur_username]["isOnline"] = false;
            offlineNumbers++;
            qDebug() << cur_username << " 新增离线";
            addToOfflineList(cur_user);
        }
    }
}

int PrivateChatTab::getOnlineNumber()
{
    return onlineNumbers;
}
int PrivateChatTab::getOfflineNumber()
{
    return offlineNumbers;
}