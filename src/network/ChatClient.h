#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "MessageProcessor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
class ChatClient : public QObject
{
    Q_OBJECT

   public:
    enum class ConnectionState
    {
        Disconnected,  // 未连接
        Connecting,    // 正在连接
        Connected,     // 已连接并可能已登录
        Reconnecting,  // 连接断开后正在重连
        Error          // 连接发生不可恢复的错误
    };
    Q_ENUM(ConnectionState)  // 关键宏
    explicit ChatClient(QObject* parent = nullptr);
    ~ChatClient();

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer(bool disconnect);
    void stopAllNetworkActivity();
    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password, const QString& nickname);
    void sendMessage(const QString& content);
    void sendPrivateMessage(const QString& receiver, const QString& content);
    // void logout(); 直接删除好了, 没用

    QString getToken() const { return currentToken; }

    ConnectionState connectionState() const { return m_connectionState; }
    bool isConnected() const { return m_connectionState == ConnectionState::Connected; }

   public slots:
    // 需要改为公共槽函数
    void sendGroupMessage(long groupId, const QString& content);
    void sendGroupTask(GroupTask* task);

   signals:
    void connected();
    void disconnected();
    void loginSuccess(const QString& username, const QString& nickname);
    void registerSuccess();
    void messageReceived(const QString& sender, const QString& content, qint64 messageId);
    void privateMessageReceived(const QString& sender, const QString& receiver,
                                const QString& content, qint64 messageId);
    void errorOccurred(const QString& error);
    void onlineUsersInit(const QJsonArray& users);
    void offlineUsersInit(const QJsonArray& users);

    void someoneLogin(const QJsonObject& loginUser);  // 信号中继到chatwindow
    void someoneLogout(const QJsonObject& logoutUser);

    void historyMessagesReceived(const QJsonArray& messages);

    void connectionStateChanged(ChatClient::ConnectionState newState);
    // 可以只根据这一个判断当前状态的变化是什么...

    void reconnecting(int reconnectAttempts);

    void connectionError(const QString& message);

   private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError error);
    void handleSocketRead();
    void sendHeartbeat();
    void tryReconnect();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);
    void handleConnectionAttemptTimeout();
   private:
    void sendJsonMessage(const QJsonObject& message);

    QTcpSocket* socket;
    QTimer* heartbeatTimer;
    QTimer* reconnectTimer;

    QTimer* serverHeartbeatTimeoutTimer;

    // 新增, 发起连接超时
    QTimer* connectionAttemptTimer;

    MessageProcessor* messageProcessor;
    QString currentToken;
    QString host;
    quint16 port;
    int reconnectAttempts = 0;
    int currentReconnectDelay = 0;

    ConnectionState m_connectionState = ConnectionState::Disconnected;

    // 新增：用户是否正在主动登出的标志，用于区分是主动断开还是异常断开
    bool m_isUserLoggingOut = false; // Add this private member

    // 辅助方法
    void handleServerHeartbeatTimeout();
    void setConnectionState(ConnectionState newState);
    void scheduleReconnect();    // 封装重连逻辑
    void resetReconnectLogic();  // 重置重连尝试次数和延迟
    void startHeartbeats();      // 启动心跳定时器和服务器心跳检测定时器
    void stopHeartbeats();       // 停止所有心跳相关定时器
};

#endif  // CHATCLIENT_H