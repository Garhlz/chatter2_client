#include "MessageProcessor.h"
#include <QDebug>
#include "utils/UserInfo.h"
#include "GlobalEventBus.h"
MessageProcessor::MessageProcessor(QObject* parent) : QObject(parent) {}

bool MessageProcessor::processMessage(const QJsonObject& message)
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
        handleRegisterMessage(message);
    }
    else if (type == "LOGIN")
    {
        handleLoginMessage(message);
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
    else if (type == "GROUP_INFO")
    {
        handleGroupInfo(message);
    }
    else if (type == "GROUP_RESPONSE")
    {
        handleGroupResponse(message);
    }
    else if (type == "HEARTBEAT")
    {
        handleHeartbeatResponse(message);
    }
    else
    {
        emit errorOccurred(QString("未知消息类型: %1").arg(type));
        return false;
    }
    return true;
}

void MessageProcessor::handleRegisterMessage(const QJsonObject& message)
{
    if (!message.contains("status") || !message["status"].isString())
    {
        emit errorOccurred("注册消息缺少 status 字段");
        return;
    }
    QString status = message["status"].toString();
    if (status == "success")
    {
        emit registerSuccess();
    }
    else
    {
        QString error = message.contains("errorMessage") && !message["errorMessage"].isNull()
                            ? message["errorMessage"].toString()
                            : "注册失败";
        emit errorOccurred(error);
    }
}

void MessageProcessor::handleLoginMessage(const QJsonObject& message)
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
            !message.contains("userId") || !message.contains("username") ||
            message["token"].toString().isEmpty() || message["nickname"].toString().isEmpty() ||
            message["username"].toString().isEmpty())
        {
            emit errorOccurred("登录消息缺少有效的 token 或 nickname 或 username");
            return;
        }
        // 这里本来也没有用到传入的token, 而是自己解析, 十分合理
        QString currentToken = message["token"].toString();
        long userId = message["userId"].toVariant().toLongLong();
        QString nickname = message["nickname"].toString();
        QString cur_username = message["username"].toString();

        UserInfo& userInfo = UserInfo::instance();
        userInfo.setUserId(userId);
        userInfo.setUsername(cur_username);
        userInfo.setNickname(nickname);
        userInfo.setToken(currentToken);
        userInfo.setOnline(true);

        qDebug()
            << QString(
                   "MessageProcessor: Login success, id: %1, username: %2, nickname: %3, token: %4")
                   .arg(userId)
                   .arg(cur_username)
                   .arg(nickname)
                   .arg(currentToken);
        // 需要百分号 + .arg 填充参数
        emit loginSuccess(cur_username, nickname, currentToken);
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
    // qDebug() << "receive online list: " << users;
    qDebug() << "Online users init, count: " << users.count();
    emit onlineUsersInit(users);
}
// 注意自己也算是在线用户的, 理论上这里应该添加的

void MessageProcessor::handleOfflineUser(const QJsonObject& message)
{
    if (!message.contains("content") || !message["content"].isArray())
    {
        emit errorOccurred("离线列表缺少 content 字段");
        return;
    }

    QJsonArray users = message["content"].toArray();
    int count = users.size();
    // qDebug() << "receive offline list: " << users;
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
    qDebug() << "user log in: " << LoginUser;
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
    qDebug() << "user logout: " << LogoutUser;
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
    // qDebug() << "history content: " << messages;
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
    if (!message.contains("nickname") || !message.contains("groupId") ||
        !message.contains("content"))
    {
        emit errorOccurred("群聊消息缺少 nickname, groupId 或 content");
        return;
    }
    // todo longlong类型都应该这样修改
    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                           ? message["messageId"].toVariant().toLongLong()
                           : 0;

    QString senderUsername = message["username"].toString();
    QString senderNickname = message["nickname"].toString();

    long groupId = message["groupId"].toVariant().toLongLong();
    QString content = message["content"].toString();

    QString timestamp = message.contains("timestamp") && !message["timestamp"].isNull()
                            ? QDateTime::fromString(message["timestamp"].toString(), Qt::ISODate)
                                  .toString("hh:mm:ss")
                            : QDateTime::currentDateTime().toString("hh:mm:ss");

    GlobalEventBus::instance()->appendGroupMessage(senderUsername, senderNickname, groupId, content,
                                                   timestamp);
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

void MessageProcessor::handleGroupInfo(const QJsonObject& message)  // 没有处理完
{
    if (!message.contains("content"))
    {
        emit errorOccurred("群组信息 消息缺少内容");
        return;
    }

    if (!message["content"].isArray())
    {
        emit errorOccurred("群组信息 消息类型错误");
        return;
    }

    // qDebug() << "receive group info: " << message["content"];
    GlobalEventBus::instance()->sendGroupInfo(message["content"]);
}

// 新增
void MessageProcessor::handleGroupResponse(const QJsonObject& message)  // 没有处理完
{
    if (!message.contains("content"))
    {
        emit errorOccurred("群组回复 消息缺少内容");
        return;
    }
    // 这里先处理不同的回复, 判断是否成功, 然后调用不同的总线信号发送回去

    QString status = message["status"].toString();
    QJsonObject content = message["content"].toObject();
    QString operationId = content["operationId"].toString();
    qDebug() << "receive group reponse: " << message["content"] << " status: " << status;
    if (status == "success")
    {
        GroupTask* task = groupTaskMap.value(operationId);
        QString taskType = task->getType();
        qDebug() << "type: " << taskType;
        if (taskType == "GROUP_CREATE")
        {
            long groupId = content["groupId"].toVariant().toLongLong();
            QString groupName = content["groupName"].toString();
            long creatorId = content["creatorId"].toVariant().toLongLong();
            GlobalEventBus::instance()->sendGroupCreate(groupId, groupName, creatorId);
        }
        else if (taskType == "GROUP_DELETE")
        {
            long groupId = content["groupId"].toVariant().toLongLong();
            GlobalEventBus::instance()->sendGroupDelete(groupId);
        }
        else if (taskType == "GROUP_ADD")
        {
            long userId = content["userId"].toVariant().toLongLong();
            long groupId = content["groupId"].toVariant().toLongLong();
            GlobalEventBus::instance()->sendGroupAdd(userId, groupId);
        }
        else if (taskType == "GROUP_REMOVE")
        {
            long userId = content["userId"].toVariant().toLongLong();
            long groupId = content["groupId"].toVariant().toLongLong();
            GlobalEventBus::instance()->sendGroupRemove(userId, groupId);
        }
    }
    else
    {
        qDebug() << "Group task reponse error, content: " << content;
    }
}

void MessageProcessor::insert(const QString& operationId, GroupTask* task)
{
    groupTaskMap.insert(operationId, task);
}

void MessageProcessor::handleHeartbeatResponse(const QJsonObject& message)
{
    QString timestamp = message["timestamp"].toString();
    qInfo()<<"heartbeat at: " << timestamp;
}