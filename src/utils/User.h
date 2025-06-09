// utils/User.h (原UserInfo.h)
#ifndef USER_H
#define USER_H

#include <QObject>
#include <QString>
#include <QDebug>  // 用于调试输出

class User : public QObject
{
    Q_OBJECT
   public:
    enum UserStatus
    {
        Offline = 0,  // 0: offline
        Online = 1,   // 1: online
        Busy = 2      // 2: busy
    };
    Q_ENUM(UserStatus)  // 注册到元对象系统，可以在QVariant转换时使用

    explicit User(long id = 0, const QString& username = "", const QString& nickname = "",
                  const QString& avatarUrl = "", UserStatus status = Offline,
                  QObject* parent = nullptr)
        : QObject(parent),
          m_userId(id),
          m_username(username),
          m_nickname(nickname),
          m_avatarUrl(avatarUrl),
          m_status(status)
    {
        // qDebug() << "User created: " << username << " ID:" << id;
    }

    // Getters
    long getUserId() const { return m_userId; }
    QString getUsername() const { return m_username; }
    QString getNickname() const { return m_nickname; }
    QString getAvatarUrl() const { return m_avatarUrl; }
    UserStatus getStatus() const { return m_status; }
    bool isOnline() const { return m_status == Online; }  // 提供一个方便的isOnline方法

    // Setters (如果数据可能变化)
    void setUserId(long id) { m_userId = id; }
    void setUsername(const QString& name) { m_username = name; }
    void setNickname(const QString& name) { m_nickname = name; }
    void setAvatarUrl(const QString& url) { m_avatarUrl = url; }
    void setStatus(UserStatus status)
    {
        if (m_status != status)
        {
            m_status = status;
        }
    }

   private:
    long m_userId;
    QString m_username;
    QString m_nickname;
    QString m_avatarUrl;
    UserStatus m_status;
};

#endif  // USER_H