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
    explicit ChatClient(QObject* parent = nullptr);
    ~ChatClient();

    void connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password, const QString& nickname);
    void sendMessage(const QString& content);
    void sendPrivateMessage(const QString& receiver, const QString& content);
    void logout();
    void requestGroupList();
    QString getToken() const { return currentToken; }

   public slots:
    // 需要改为公共槽函数
    void sendGroupMessage(long groupId, const QString& content);
    void sendGroupTask(GroupTask* task);

   signals:
    void connected();
    void disconnected();
    void loginSuccess(const QString& username, const QString& nickname);
    void registerSuccess(const QString& token);
    void messageReceived(const QString& sender, const QString& content, qint64 messageId);
    void privateMessageReceived(const QString& sender, const QString& receiver,
                                const QString& content, qint64 messageId);
    void errorOccurred(const QString& error);
    void onlineUsersInit(const QJsonArray& users);
    void offlineUsersInit(const QJsonArray& users);

    void someoneLogin(const QJsonObject& loginUser);  // 信号中继到chatwindow
    void someoneLogout(const QJsonObject& logoutUser);

    void historyMessagesReceived(const QJsonArray& messages);

   private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError error);
    void handleSocketRead();
    void sendHeartbeat();
    void tryReconnect();

   private:
    void sendJsonMessage(const QJsonObject& message);

    QTcpSocket* socket;
    QTimer* heartbeatTimer;
    QTimer* reconnectTimer;
    MessageProcessor* messageProcessor;
    QString currentToken;
    QString host;
    quint16 port;
    int reconnectAttempts = 0;
    static const int HEARTBEAT_INTERVAL = 30000;  // 30 seconds
    static const int MAX_RECONNECT_ATTEMPTS = 3;
    static const int RECONNECT_INTERVAL = 5000;  // 5 seconds
};

#endif  // CHATCLIENT_H