// utils/UserManager.h
#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include "User.h"  // 包含User类

class UserManager : public QObject
{
    Q_OBJECT
   public:
    explicit UserManager(QObject* parent = nullptr);
    ~UserManager() override;

    // **新增/恢复：分步初始化用户列表**
    void initOnlineUsers(const QJsonArray& users);
    void initOfflineUsers(const QJsonArray& users);

    // **新增：通知UserManager所有初始数据已加载**
    void markInitialDataLoaded();

    // 获取用户数据的方法 (外部只能通过这些接口访问)
    QMap<long, User*> getAllUsers() const { return m_allUsers; }  // 返回ID到User*的映射
    User* getUserByUsername(const QString& username) const;       // 根据username查找
    User* getUserById(long userId) const;                         // 根据ID查找

    int getOnlineNumber() const { return m_onlineNumbers; }
    int getOfflineNumber() const { return m_offlineNumbers; }
    int getBusyNumber() const { return m_busyNumbers; }

    // 外部唯一修改用户状态的接口
    // status: 0: offline, 1: online, 2: busy
    void handleUserStatusChange(const QJsonObject& userJson, int status);

   signals:
    // **UserManager发出的信号，通知外部UI或其他模块更新**
    void usersInitialized();  // 第一次加载所有用户数据完成 (现在由 markInitialDataLoaded 触发)
    void userStatusChanged(User* user);  // 某个用户状态发生变化（更通用）
    void userAdded(User* user);          // 新用户被添加
    void userRemoved(User* user);        // 用户被移除 (如果需要)

   private:
    QMap<long, User*> m_allUsers;
    QMap<QString, long> m_usernameToIdMap;

    int m_onlineNumbers;
    int m_offlineNumbers;
    int m_busyNumbers;

    bool m_isInitialDataLoaded;  // 标记是否已经完成了初始数据加载

    void clearUsers();  // 清理所有 User 对象
    // 内部辅助函数：添加/更新用户，包含状态变化逻辑
    void addOrUpdateUser(long userId, const QString& username, const QString& nickname,
                         const QString& avatarUrl, User::UserStatus status);
};

#endif  // USERMANAGER_H