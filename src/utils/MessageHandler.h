#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "utils/GroupTask.h"
class MessageHandler
{
   public:
    static QJsonObject createLoginMessage(const QString& username, const QString& password);
    static QJsonObject createRegisterMessage(const QString& username, const QString& password,
                                             const QString& nickname);
    static QJsonObject createChatMessage(const QString& content, const QString& token);
    static QJsonObject createPrivateChatMessage(const QString& receiver, const QString& content,
                                                const QString& token);
    // change
    static QJsonObject createGroupChatMessage(long userId, const QString& username,
                                              const QString& nickname, long groupId,
                                              const QString& content, const QString& token);
    static QJsonObject createFileMessage(const QString& receiver, const QByteArray& fileContent,
                                         const QString& token);
    static QJsonObject createLogoutMessage(const QString& token);
    static QJsonObject createHeartbeatMessage(const QString& token);

    static QString getErrorMessage(const QJsonObject& response);
    static QString getSystemMessage(const QJsonObject& response);
    static QJsonArray getOnlineUsers(const QJsonObject& response);
    static int getOnlineCount(const QJsonObject& response);
    // 新增
    static QJsonObject createGroupTask(const GroupTask* task);
};

#endif  // MESSAGEHANDLER_H