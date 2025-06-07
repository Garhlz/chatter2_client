#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <QJsonObject>
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
class MessageProcessor : public QObject
{
    Q_OBJECT

   public:
    explicit MessageProcessor(QObject* parent = nullptr);

    // 处理消息，返回是否成功，更新 token 和心跳状态
    bool processMessage(const QJsonObject& message, QString& currentToken, bool& heartbeatActive);

   signals:
    // 注册和登录信号
    void registerSuccess(const QString& token);
    void loginSuccess(const QString& username, QString& nickname);
    // 消息接收信号
    void messageReceived(const QString& sender, const QString& content, qint64 messageId);
    void privateMessageReceived(const QString& sender, const QString& receiver,
                                const QString& content, qint64 messageId);
    void groupMessageReceived(const QString& sender, const QString& groupName,
                              const QString& content, qint64 messageId);
    // void fileReceived(const QString& sender, const QString& receiver, const QJsonObject& fileInfo,
    //                   qint64 messageId, QString timestamp);
    // 这里修改了, 使用事件总线进行发送

    // 系统信息信号
    void onlineUsersInit(const QJsonArray& users);
    void offlineUsersInit(const QJsonArray& users);
    void historyMessagesReceived(const QJsonArray& messages);
    void errorOccurred(const QString& error);

    void someoneLogin(const QJsonObject& loginUsername);
    void someoneLogout(const QJsonObject& logoutUsername);

   private:
    void handleRegisterMessage(const QJsonObject& message, QString& currentToken,
                               bool& heartbeatActive);
    void handleLoginMessage(const QJsonObject& message, QString& currentToken,
                            bool& heartbeatActive);
    void handleSystemMessage(const QJsonObject& message);
    void handleChatMessage(const QJsonObject& message);
    void handlePrivateChatMessage(const QJsonObject& message);
    void handleGroupChatMessage(const QJsonObject& message);
    void handleFileMessage(const QJsonObject& message);
    void handleErrorMessage(const QJsonObject& message);
    void handleOnlineUser(const QJsonObject& message);
    void handleOfflineUser(const QJsonObject& message);
    void handleHistoryMessages(const QJsonObject& message);
    void handleUserLoginMessage(const QJsonObject& message);
    void handleUserLogoutMessage(const QJsonObject& message);
};

#endif  // MESSAGEPROCESSOR_H