// utils/UserManager.cpp
#include "UserManager.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

UserManager::UserManager(QObject* parent)
    : QObject(parent),
      m_onlineNumbers(0),
      m_offlineNumbers(0),
      m_busyNumbers(0),
      m_isInitialDataLoaded(false)  // 初始化标志
{
    qDebug() << "UserManager created.";
}

UserManager::~UserManager()
{
    clearUsers();
    qDebug() << "UserManager destroyed, all User objects cleared.";
}

void UserManager::clearUsers()
{
    for (User* user : m_allUsers.values())
    {
        delete user;  // 释放 User 对象
    }
    m_allUsers.clear();
    m_usernameToIdMap.clear();
    m_onlineNumbers = 0;
    m_offlineNumbers = 0;
    m_busyNumbers = 0;
    m_isInitialDataLoaded = false;  // 重置初始化标志
}

// **重新引入：初始化在线用户列表**
void UserManager::initOnlineUsers(const QJsonArray& users)
{
    qDebug() << "UserManager: Initializing online users...";
    // 不在此处 clearUsers()，因为是增量更新
    for (const QJsonValue& userValue : users)
    {
        if (userValue.isObject())
        {
            QJsonObject userObj = userValue.toObject();
            long id = userObj["userId"].toVariant().toLongLong();
            QString username = userObj["username"].toString();
            QString nickname = userObj["nickname"].toString();
            QString avatarUrl = userObj["avatarUrl"].toString();

            // 注意这里的逻辑有问题, 是直接设置状态而不是通过status项获得
            User::UserStatus status = User::UserStatus::Online;
            addOrUpdateUser(id, username, nickname, avatarUrl, status);
        }
    }
    qDebug() << "UserManager: Online users initialized. Current Online: " << m_onlineNumbers;
}

// **重新引入：初始化离线用户列表**
void UserManager::initOfflineUsers(const QJsonArray& users)
{
    qDebug() << "UserManager: Initializing offline users...";
    // 不在此处 clearUsers()，因为是增量更新
    for (const QJsonValue& userValue : users)
    {
        if (userValue.isObject())
        {
            QJsonObject userObj = userValue.toObject();
            long id = userObj["userId"].toVariant().toLongLong();
            QString username = userObj["username"].toString();
            QString nickname = userObj["nickname"].toString();
            QString avatarUrl = userObj["avatarUrl"].toString();
            // 这里也是
            User::UserStatus status = User::UserStatus::Offline;
            addOrUpdateUser(id, username, nickname, avatarUrl, status);
        }
    }
    qDebug() << "UserManager: Offline users initialized. Current Offline: " << m_offlineNumbers;
}

// **新增：标记初始数据加载完成**
void UserManager::markInitialDataLoaded()
{
    if (!m_isInitialDataLoaded)
    {
        m_isInitialDataLoaded = true;
        emit usersInitialized();  // 只有当所有初始数据加载完成后才发出此信号
        qDebug() << "UserManager: Initial data load marked as complete. Emitting usersInitialized.";
    }
}

// 内部辅助函数：添加或更新用户
void UserManager::addOrUpdateUser(long userId, const QString& username, const QString& nickname,
                                  const QString& avatarUrl, User::UserStatus newStatus)
{
    User* user = m_allUsers.value(userId);

    if (!user)
    {
        // 用户不存在，创建新用户
        user = new User(userId, username, nickname, avatarUrl, newStatus,
                        this);  // UserManager作为父对象
        m_allUsers.insert(userId, user);
        m_usernameToIdMap.insert(username, userId);

        // 更新计数
        if (newStatus == User::Online)
            m_onlineNumbers++;
        else if (newStatus == User::Offline)
            m_offlineNumbers++;
        else if (newStatus == User::Busy)
            m_busyNumbers++;

        // 如果不是在初始加载阶段，则可以发射 userAdded 信号
        if (m_isInitialDataLoaded)
        {
            emit userAdded(user);
        }
        // qDebug() << "UserManager: Added new user " << username << " (ID:" << userId
        //          << ") with status: " << newStatus;
    }
    else
    {
        // 用户已存在，更新其信息和状态
        User::UserStatus oldStatus = user->getStatus();

        // 无论状态是否改变，先更新其他信息
        user->setNickname(nickname);
        user->setAvatarUrl(avatarUrl);
        // 如果用户名可能改变，也需要更新 m_usernameToIdMap，但这通常不是常规操作
        // if (user->getUsername() != username) {
        //     m_usernameToIdMap.remove(user->getUsername());
        //     user->setUsername(username);
        //     m_usernameToIdMap.insert(username, userId);
        // }

        if (oldStatus != newStatus)
        {
            // 更新计数
            if (oldStatus == User::Online)
                m_onlineNumbers--;
            else if (oldStatus == User::Offline)
                m_offlineNumbers--;
            else if (oldStatus == User::Busy)
                m_busyNumbers--;

            if (newStatus == User::Online)
                m_onlineNumbers++;
            else if (newStatus == User::Offline)
                m_offlineNumbers++;
            else if (newStatus == User::Busy)
                m_busyNumbers++;

            user->setStatus(newStatus);

            // 信号搞太多了反而会出错, 这里就是
            emit userStatusChanged(user);

            qDebug() << "UserManager: User " << username << " (ID:" << userId
                     << ") status changed from " << oldStatus << " to " << newStatus;
        }
        else
        {
            qDebug() << "UserManager: User " << username << " (ID:" << userId << ") status already "
                     << newStatus << ". No change in status.";
        }
    }
}

User* UserManager::getUserByUsername(const QString& username) const
{
    if (m_usernameToIdMap.contains(username))
    {
        long userId = m_usernameToIdMap.value(username);
        return m_allUsers.value(userId, nullptr);
    }
    return nullptr;
}

User* UserManager::getUserById(long userId) const
{
    return m_allUsers.value(userId, nullptr);
}

// 关键方法：处理来自ChatWindow的单个用户状态变化通知
// userJson 应该包含 userId, username, nickname, avatarUrl
void UserManager::handleUserStatusChange(const QJsonObject& userJson, int status)
{
    long userId = userJson["userId"].toVariant().toLongLong();
    QString username = userJson["username"].toString();
    QString nickname = userJson["nickname"].toString();
    QString avatarUrl = userJson["avatarUrl"].toString();  // 获取头像URL
    User::UserStatus newStatus = static_cast<User::UserStatus>(status);

    addOrUpdateUser(userId, username, nickname, avatarUrl, newStatus);
}