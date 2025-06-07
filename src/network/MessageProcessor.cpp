#include "MessageProcessor.h"
#include <QDebug>
#include "utils/UserInfo.h"
#include "GlobalEventBus.h"
MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent) {}

bool MessageProcessor::processMessage(const QJsonObject& message, QString& currentToken,
                                      bool& heartbeatActive)
{
    if (!message.contains("type") || !message["type"].isString())
    {
        emit errorOccurred("消息缺少 type 字段或格式错误");
        return false;
    }

    QString type = message["type"].toString();
    // cpp限制,switchcase很有限, 不能直接对于string使用,只能对于常量或者枚举使用
    // 这里转换为枚举就还要写一遍, 无语了
    if (type == "REGISTER")
    {
        handleRegisterMessage(message, currentToken, heartbeatActive);
    }
    else if (type == "LOGIN")
    {
        handleLoginMessage(message, currentToken, heartbeatActive);
    }
    else if (type == "SYSTEM")
    {
        handleSystemMessage(message);
    }
    else if (type == "ONLINE_USERS")
    {
        handleOnlineUser(message);
    }
    else if (type == "OFFLINE_USERS")
    {
        handleOfflineUser(message);
    }
    else if (type == "HISTORY_MESSAGES")
    {
        handleHistoryMessages(message);
    }
    else if (type == "CHAT")
    {
        handleChatMessage(message);
    }
    else if (type == "PRIVATE_CHAT")
    {
        handlePrivateChatMessage(message);
    }
    else if (type == "GROUP_CHAT")
    {
        handleGroupChatMessage(message);
    }
    else if (type == "FILE")
    {
        handleFileMessage(message);
    }
    else if (type == "ERROR")
    {
        handleErrorMessage(message);
    }
    else if (type == "USER_LOGIN")
    {
        handleUserLoginMessage(message);
    }
    else if (type == "USER_LOGOUT")
    {
        handleUserLogoutMessage(message);
    }
    else
    {
        emit errorOccurred(QString("未知消息类型: %1").arg(type));
        return false;
    }
    return true;
}

void MessageProcessor::handleRegisterMessage(const QJsonObject& message, QString& currentToken,
                                             bool& heartbeatActive)
{
    if (!message.contains("status") || !message["status"].isString())
    {
        emit errorOccurred("注册消息缺少 status 字段");
        return;
    }
    QString status = message["status"].toString();
    if (status == "success")
    {
        currentToken = message.contains("token") ? message["token"].toString() : "";
        emit registerSuccess(currentToken);
        heartbeatActive = true;
    }
    else
    {
        QString error = message.contains("errorMessage") && !message["errorMessage"].isNull()
                            ? message["errorMessage"].toString()
                            : "注册失败";
        emit errorOccurred(error);
    }
}

void MessageProcessor::handleLoginMessage(const QJsonObject& message, QString& currentToken,
                                          bool& heartbeatActive)
{
    if (!message.contains("status") || !message["status"].isString())
    {
        emit errorOccurred("登录消息缺少 status 字段");
        return;
    }
    QString status = message["status"].toString();
    if (status == "success")
    {
        if (!message.contains("token") || !message.contains("nickname") ||
            !message.contains("username") || message["token"].toString().isEmpty() ||
            message["nickname"].toString().isEmpty() || message["username"].toString().isEmpty())
        {
            emit errorOccurred("登录消息缺少有效的 token 或 nickname 或 username");
            return;
        }
        currentToken = message["token"].toString();
        QString nickname = message["nickname"].toString();
        QString cur_username = message["username"].toString();

        UserInfo& userInfo = UserInfo::instance();
        userInfo.setUsername(cur_username);
        userInfo.setNickname(nickname);
        userInfo.setToken(currentToken);

        qDebug() << "MessageProcessor: Login success, " << "username: " << cur_username
                 << ", nickname: " << nickname << ", token: " << currentToken;

        emit loginSuccess(cur_username, nickname);
        heartbeatActive = true;
    }
    else
    {
        QString error = message.contains("errorMessage") && !message["errorMessage"].isNull()
                            ? message["errorMessage"].toString()
                            : "登录失败";
        emit errorOccurred(error);
    }
}

void MessageProcessor::handleSystemMessage(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isString())
    {
        emit errorOccurred("系统消息缺少 content 字段");
        return;
    }
    QString content = message["content"].toString();

    emit errorOccurred(QString("未知系统消息内容: %1").arg(content));
}

void MessageProcessor::handleOnlineUser(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isArray())
    {
        emit errorOccurred("在线列表缺少 content 字段");
        return;
    }

    QJsonArray users = message["content"].toArray();
    qDebug() << "Online users init, count: " << users.count();
    emit onlineUsersInit(users);
}

void MessageProcessor::handleOfflineUser(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isArray())
    {
        emit errorOccurred("离线列表缺少 content 字段");
        return;
    }

    QJsonArray users = message["content"].toArray();
    int count = users.size();
    qDebug() << "Offline users init. count: " << users.count();
    emit offlineUsersInit(users);
}

void MessageProcessor::handleUserLoginMessage(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isObject())
    {
        emit errorOccurred("someoneLogin 缺少 content 字段");
        return;
    }
    QJsonObject LoginUser = message["content"].toObject();
    emit someoneLogin(LoginUser);
}

void MessageProcessor::handleUserLogoutMessage(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isObject())
    {
        emit errorOccurred("someoneLogout 缺少 content 字段");
        return;
    }
    QJsonObject LogoutUser = message["content"].toObject();
    emit someoneLogout(LogoutUser);
}

void MessageProcessor::handleHistoryMessages(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isArray())
    {
        emit errorOccurred("历史记录缺少 content 字段");
        return;
    }
    QJsonArray messages = message["content"].toArray();
    qDebug() << "History received successfully, number = " << messages.size();
    emit historyMessagesReceived(messages);
}

void MessageProcessor::handleChatMessage(const QJsonObject& message)
{
    if (!message.contains("nickname") || !message.contains("content"))
    {
        emit errorOccurred("聊天消息缺少 nickname 或 content");
        return;
    }
    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                           ? message["messageId"].toVariant().toLongLong()
                           : 0;
    emit messageReceived(message["nickname"].toString(), message["content"].toString(), messageId);
}

void MessageProcessor::handlePrivateChatMessage(const QJsonObject& message)
{
    if (!message.contains("username") || !message.contains("nickname") ||
        !message.contains("receiver") || !message.contains("content"))
    {
        emit errorOccurred("私聊消息缺少内容");
        return;
    }
    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                           ? message["messageId"].toVariant().toLongLong()
                           : 0;
    emit privateMessageReceived(message["username"].toString(), message["receiver"].toString(),
                                message["content"].toString(), messageId);
}

void MessageProcessor::handleGroupChatMessage(const QJsonObject& message)
{
    if (!message.contains("nickname") || !message.contains("groupName") ||
        !message.contains("content"))
    {
        emit errorOccurred("群聊消息缺少 nickname, groupName 或 content");
        return;
    }
    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                           ? message["messageId"].toVariant().toLongLong()
                           : 0;
    emit groupMessageReceived(message["nickname"].toString(), message["groupName"].toString(),
                              message["content"].toString(), messageId);
}

void MessageProcessor::handleFileMessage(const QJsonObject& message)
{
    if (!message.contains("username") || !message.contains("nickname") ||
        !message.contains("receiver") || !message.contains("content") ||
        !message.contains("timestamp"))
    {
        emit errorOccurred("文件消息缺少内容");
        return;
    }

    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                           ? message["messageId"].toVariant().toLongLong()
                           : 0;
    // qt的键值对是QString:QJsonValue, 需要调用相应的方法转化, 这里是转化为QJsonObject

    QString sender = message["username"].toString();
    QString receiver = message["receiver"].toString();
    QJsonObject fileInfo = message["content"].toObject();  // 假设文件元数据在 "fileInfo" 对象中
    QString timestamp = message["timestamp"].toString();   // 时间戳作为字符串

    // 通过事件总线发射信号，通知所有对文件消息感兴趣的组件
    GlobalEventBus::instance()->globalAppendMessage(sender, receiver, fileInfo, timestamp, true);
}

void MessageProcessor::handleErrorMessage(const QJsonObject& message)
{
    QString error = message.contains("errorMessage") && !message["errorMessage"].isNull()
                        ? message["errorMessage"].toString()
                        : "服务器错误";
    emit errorOccurred(error);
}
