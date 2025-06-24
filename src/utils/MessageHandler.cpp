#include "MessageHandler.h"
#include "utils/GroupTask.h"
#include "utils/UserInfo.h"
QJsonObject MessageHandler::createLoginMessage(const QString& username, const QString& password)
{
    QJsonObject message;
    message["type"] = "LOGIN";
    message["username"] = username;
    message["password"] = password;
    return message;
}

QJsonObject MessageHandler::createRegisterMessage(const QString& username, const QString& password,
                                                  const QString& nickname)
{
    QJsonObject message;
    message["type"] = "REGISTER";
    message["username"] = username;
    message["password"] = password;
    message["nickname"] = nickname;
    return message;
}

QJsonObject MessageHandler::createChatMessage(const QString& content, const QString& token)
{
    QJsonObject message;
    message["type"] = "CHAT";
    message["content"] = content;
    message["token"] = token;
    return message;
}

QJsonObject MessageHandler::createPrivateChatMessage(const QString& receiver,
                                                     const QString& content, const QString& token)
{
    QJsonObject message;
    message["type"] = "PRIVATE_CHAT";
    message["receiver"] = receiver;  // 此处是username而不是nickname!!
    message["content"] = content;
    message["token"] = token;
    qDebug() << "send private chat: " << message;  // 在这里debug好了
    return message;
}

QJsonObject MessageHandler::createGroupChatMessage(long userId, const QString& username,
                                                   const QString& nickname, long groupId,
                                                   const QString& content, const QString& token)
{
    QJsonObject message;
    message["type"] = "GROUP_CHAT";
    message["userId"] = static_cast<qint64>(userId);  // 明确转换为 qint64
    message["username"] = username;
    message["nickname"] = nickname;
    message["groupId"] = static_cast<qint64>(groupId);  // 明确转换为 qint64
    message["content"] = content;
    message["token"] = token;
    return message;
}

QJsonObject MessageHandler::createGroupTask(const GroupTask* task)
{
    QJsonObject message;
    message["type"] = task->getType();
    QJsonObject content;
    content["operationId"] = task->getOperationId();
    content["operatorId"] = static_cast<qint64>(task->getOperatorId());  // 明确转换为 qint64
    content["groupId"] = static_cast<qint64>(task->getGroupId());        // 明确转换为 qint64
    content["userId"] = static_cast<qint64>(task->getUserId());          // 明确转换为 qint64
    content["groupName"] = task->getGroupName();

    message["content"] = content;
    message["token"] = UserInfo::instance().token();
    return message;
}

QJsonObject MessageHandler::createFileMessage(const QString& receiver,
                                              const QByteArray& fileContent, const QString& token)
{
    QJsonObject message;
    message["type"] = "FILE";
    message["receiver"] = receiver;
    message["content"] = QString::fromLatin1(fileContent.toBase64());
    message["token"] = token;
    return message;
}

QJsonObject MessageHandler::createLogoutMessage(const QString& token)
{
    QJsonObject message;
    message["type"] = "LOGOUT";
    message["token"] = token;
    return message;
}

QJsonObject MessageHandler::createHeartbeatMessage(const QString& token)
{
    QJsonObject message;
    message["type"] = "HEARTBEAT";
    message["token"] = token;
    return message;
}

QString MessageHandler::getErrorMessage(const QJsonObject& response)
{
    return response["errorMessage"].toString();
}

QString MessageHandler::getSystemMessage(const QJsonObject& response)
{
    return response["content"].toString();
}

QJsonArray MessageHandler::getOnlineUsers(const QJsonObject& response)
{
    return response["onlineUsers"].toArray();
}

int MessageHandler::getOnlineCount(const QJsonObject& response)
{
    return response["onlineCount"].toInt();
}