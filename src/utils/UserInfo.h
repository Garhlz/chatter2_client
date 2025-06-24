// src/utils/UserInfo.h
#ifndef USERINFO_H
#define USERINFO_H

#include <QString>

class UserInfo
{
   public:
    // 删除拷贝构造函数和赋值操作符，确保单例
    UserInfo(const UserInfo&) = delete;
    UserInfo& operator=(const UserInfo&) = delete;

    // 获取单例实例
    static UserInfo& instance();

    // 用户信息设置
    void setUserId(long id) { m_userId = id; }
    void setUsername(const QString& username) { m_username = username; }
    void setNickname(const QString& nickname) { m_nickname = nickname; }
    void setToken(const QString& token) { m_token = token; }
    void setOnline(bool flag){isOnline = flag;}

    // 用户信息获取
    long userId() const { return m_userId; }
    QString username() const { return m_username; }
    QString nickname() const { return m_nickname; }
    QString token() const { return m_token; }
    bool online() const {return isOnline;}
    // 检查是否已登录
    bool isLoggedIn() const { return isOnline;}

    // 清空用户信息（登出时使用）
    void clear()
    {
        m_userId = -1;
        m_username.clear();
        m_nickname.clear();
        m_token.clear();
        isOnline = false;
    }

   private:
    UserInfo() = default;   // 私有构造函数
    ~UserInfo() = default;  // 私有析构函数

    long m_userId = -1;
    QString m_username;
    QString m_nickname;
    QString m_token;
    bool isOnline;
};

#endif  // USERINFO_H