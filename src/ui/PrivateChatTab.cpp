#include "PrivateChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QStackedWidget>
#include <QtConcurrent/QtConcurrent>
#include <QDebug.h>
#include "PrivateChatSession.h"
#include "utils/UserInfo.h"
#include "GlobalEventBus.h"

PrivateChatTab::PrivateChatTab(ChatClient* client, const QString& username, const QString& nickname,
                               QWidget* parent)
    : QWidget(parent), chatClient(client), curUsername(username), curNickname(nickname)
{
    setupUi();
    connectSignals();
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
}

void PrivateChatTab::handleUserSelected(QListWidgetItem* item)
{
    if (!item) return;
    QString targetUser = item->data(Qt::UserRole).toString();
    qDebug() << "选中 user: " << targetUser;
    PrivateChatSession* session = getOrCreateSession(targetUser);
    if (session)
    {  // 检查 session 非空
        sessionStack->setCurrentWidget(session);
        for (int i = 0; i < sessionList->count(); ++i)
        {
            if (sessionList->item(i)->data(Qt::UserRole).toString() == targetUser)
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
    QString targetUser = item->data(Qt::UserRole).toString();
    PrivateChatSession* session = sessions.value(targetUser, nullptr);
    if (session)
    {
        sessionStack->setCurrentWidget(session);
    }
}

PrivateChatSession* PrivateChatTab::getOrCreateSession(const QString& targetUser)
{
    if (targetUser == curUsername)
    {  // 过滤当前用户
        qDebug() << "不能与自己创建会话: " << targetUser;
        return nullptr;
    }
    if (sessions.contains(targetUser))
    {
        return sessions[targetUser];
    }
    QString targetNickname = allUsers[targetUser]["nickname"].toString();

    PrivateChatSession* session = new PrivateChatSession(chatClient, curUsername, curNickname,
                                                         targetUser, targetNickname, this);
    sessions[targetUser] = session;

    sessionStack->addWidget(session);

    QString displayText = targetNickname.isEmpty() ? targetUser : targetNickname;
    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, targetUser);
    sessionList->addItem(item);
    sessionList->setCurrentItem(item);
    sessionStack->setCurrentWidget(session);

    connect(session, &PrivateChatSession::sendMessageRequested, this,
            [=](const QString& target, const QString& content)
            { chatClient->sendPrivateMessage(target, content); });
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
                                   const QJsonValue& content, const QString& timestamp, bool isFile)
{
    qDebug() << " PrivateChatTab::appendMessage sender: " << sender << "receiver: " << receiver
             << " content: " << content;
    PrivateChatSession* session = getOrCreateSessionTwo(sender, receiver);
    if (session)
    {  // 检查 session 非空
        session->appendMessage(sender, receiver, content, timestamp, isFile);
    }
}

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

void PrivateChatTab::initOnlineUsers(const QJsonArray& users)
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

void PrivateChatTab::initOfflineUsers(const QJsonArray& users)
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

void PrivateChatTab::someoneChange(int status, const QJsonObject& cur_user)
{
    if (status == 0)  // 上线
    {
        QString cur_username = cur_user["username"].toString();
        if (allUsers.contains(cur_username))
        {
            if (!allUsers[cur_username]["isOnline"].toBool())
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
            if (allUsers[cur_username]["isOnline"].toBool())
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