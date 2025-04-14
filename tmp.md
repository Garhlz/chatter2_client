这是我当前的chatclient,我感觉暂时不用在本地存储消息记录, 用服务器存储之后请求查找即可.先帮我修改client的逻辑
#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>

class ChatClient : public QObject {
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient();

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password, const QString &nickname);
    void sendMessage(const QString &content);
    void sendPrivateMessage(const QString &receiver, const QString &content);
    void sendGroupMessage(const QString &groupName, const QString &content);
    void sendFile(const QString &receiver, const QByteArray &fileContent);
    void logout();
    void requestGroupList();
    QString getToken() const { return currentToken; }

signals:
    void connected();
    void disconnected();
    void loginSuccess(const QString &nickname);
    void registerSuccess(const QString &token);
    // 例如这就是一个信号函数, 当设置emit registerSuccess(token)的时候, 就会触发这个信号函数, connect绑定的关键字处于RegisterWindow中
    // connect(chatClient, &ChatClient::registerSuccess, this, &RegisterWindow::handleRegisterSuccess);
    // 会在RegisterWindow中触发对应的槽函数
    /*
    void RegisterWindow::handleRegisterSuccess(const QString &token) {
        statusLabel->setText("注册成功");
        QMessageBox::information(this, "注册成功", "账号注册成功，请返回登录界面进行登录。");
        registerButton->setEnabled(true);
        usernameEdit->clear();
        passwordEdit->clear();
        nicknameEdit->clear();
        emit registerSuccessful();
        showLogin();
    }
    */
    void messageReceived(const QString &sender, const QString &content, qint64 messageId);
    void privateMessageReceived(const QString &sender, const QString &content, qint64 messageId);
    void groupMessageReceived(const QString &sender, const QString &groupName, const QString &content, qint64 messageId);
    void fileReceived(const QString &sender, const QByteArray &fileContent, qint64 messageId);
    void errorOccurred(const QString &error);
    void onlineUsersUpdated(const QJsonArray &users, int count);
    void groupListReceived(const QJsonArray &groups);
    void historyMessagesReceived(const QJsonArray &messages); // 历史消息接收信号

private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError error);
    void handleSocketRead();
    void sendHeartbeat();
    void tryReconnect();

private:
    void sendJsonMessage(const QJsonObject &message);
    void processMessage(const QJsonObject &message);
    void logMessage(const QString &prefix, const QByteArray &data);

    QTcpSocket *socket;
    QTimer *heartbeatTimer;
    QTimer *reconnectTimer;
    QString currentToken;
    QString host;
    quint16 port;
    int reconnectAttempts = 0;
    static const int HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const int MAX_RECONNECT_ATTEMPTS = 3;
    static const int RECONNECT_INTERVAL = 5000; // 5 seconds
};

#endif // CHATCLIENT_H

#include "ChatClient.h"
#include "utils/MessageHandler.h"
#include "utils/JsonConverter.h"
#include <QDebug>
#include <QFile>

ChatClient::ChatClient(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    heartbeatTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);

    connect(socket, &QTcpSocket::connected, this, &ChatClient::handleSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &ChatClient::handleSocketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::handleSocketRead);
    connect(socket, &QAbstractSocket::errorOccurred, this, &ChatClient::handleSocketError);
    connect(heartbeatTimer, &QTimer::timeout, this, &ChatClient::sendHeartbeat);
    connect(reconnectTimer, &QTimer::timeout, this, &ChatClient::tryReconnect);
}

ChatClient::~ChatClient() {
    disconnectFromServer();
    delete socket;
    delete heartbeatTimer;
    delete reconnectTimer;
}

void ChatClient::connectToServer(const QString &host, quint16 port) {
    this->host = host;
    this->port = port;
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->connectToHost(host, port);
        logMessage("Connecting to", QString("%1:%2").arg(host).arg(port).toUtf8());
    }
}

void ChatClient::disconnectFromServer() {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    heartbeatTimer->stop();
    reconnectTimer->stop();
    currentToken.clear();
}

void ChatClient::login(const QString &username, const QString &password) {
    sendJsonMessage(MessageHandler::createLoginMessage(username, password));
}

void ChatClient::registerUser(const QString &username, const QString &password, const QString &nickname) {
    sendJsonMessage(MessageHandler::createRegisterMessage(username, password, nickname));
}

void ChatClient::sendMessage(const QString &content) {
    sendJsonMessage(MessageHandler::createChatMessage(content, currentToken));
}

void ChatClient::sendPrivateMessage(const QString &receiver, const QString &content) {
    sendJsonMessage(MessageHandler::createPrivateChatMessage(receiver, content, currentToken));
}

void ChatClient::sendGroupMessage(const QString &groupName, const QString &content) {
    sendJsonMessage(MessageHandler::createGroupChatMessage(groupName, content, currentToken));
}

void ChatClient::sendFile(const QString &receiver, const QByteArray &fileContent) {
    const int CHUNK_SIZE = 8000; // 留余量，服务器限制10000字节
    for (int i = 0; i < fileContent.size(); i += CHUNK_SIZE) {
        QByteArray chunk = fileContent.mid(i, CHUNK_SIZE);
        QJsonObject message = MessageHandler::createFileMessage(receiver, chunk, currentToken);
        message["part"] = i / CHUNK_SIZE + 1;
        message["totalParts"] = (fileContent.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;
        sendJsonMessage(message);
    }
}

void ChatClient::logout() {
    if (!currentToken.isEmpty()) {
        sendJsonMessage(MessageHandler::createLogoutMessage(currentToken));
        currentToken.clear();
        heartbeatTimer->stop();
    }
}

void ChatClient::requestGroupList() {
    // 暂时禁用：服务器不支持 GET_GROUPS 类型
    // QJsonObject message{{"type", "GET_GROUPS"}, {"token", currentToken}};
    // sendJsonMessage(message);
    logMessage("Warning", "requestGroupList disabled: server does not support GET_GROUPS");
}

void ChatClient::handleSocketConnected() {
    reconnectAttempts = 0;
    reconnectTimer->stop();
    heartbeatTimer->start(HEARTBEAT_INTERVAL);
    emit connected();
    logMessage("Connected to", QString("%1:%2").arg(host).arg(port).toUtf8());
}

void ChatClient::handleSocketDisconnected() {
    heartbeatTimer->stop();
    currentToken.clear();
    emit disconnected();
    tryReconnect();
    logMessage("Disconnected from", QString("%1:%2").arg(host).arg(port).toUtf8());
}

void ChatClient::handleSocketError(QAbstractSocket::SocketError) {
    QString errorMessage = socket->errorString();
    emit errorOccurred(errorMessage);
    logMessage("Socket error", errorMessage.toUtf8());
}

void ChatClient::handleSocketRead() {
    while (socket->canReadLine()) {
        QByteArray data = socket->readLine().trimmed();
        logMessage("Received", data);
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            emit errorOccurred("Invalid JSON format");
            logMessage("Error", "Invalid JSON format");
            continue;
        }
        processMessage(doc.object());
    }
}

void ChatClient::sendHeartbeat() {
    if (!currentToken.isEmpty()) {
        sendJsonMessage(MessageHandler::createHeartbeatMessage(currentToken));
    }
}

void ChatClient::tryReconnect() {
    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        reconnectAttempts++;
        socket->connectToHost(host, port);
        logMessage("Reconnecting", QString("Attempt %1/%2").arg(reconnectAttempts).arg(MAX_RECONNECT_ATTEMPTS).toUtf8());
    } else {
        reconnectTimer->stop();
        emit errorOccurred("无法重新连接到服务器，请检查网络");
        logMessage("Error", "Reconnect failed");
    }
}

void ChatClient::sendJsonMessage(const QJsonObject &message) {
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    socket->write(data);
    socket->flush();
    logMessage("Sent", data);
}

void ChatClient::processMessage(const QJsonObject &message) {
    QString type = message["type"].toString();
    qint64 messageId = message.contains("messageId") && !message["messageId"].isNull()
                       ? message["messageId"].toVariant().toLongLong()
                       : 0;

    if (type == "REGISTER") {
        QString status = message["status"].toString();
        if (status == "success") {
            currentToken = message.contains("token") ? message["token"].toString() : "";
            QString nickname = message["nickname"].toString();
            emit registerSuccess(currentToken);
            heartbeatTimer->start(HEARTBEAT_INTERVAL);
            logMessage("Register success", QString("Nickname: %1").arg(nickname).toUtf8());
        } else {
            QString error = message["errorMessage"].toString();
            emit errorOccurred(error.isEmpty() ? "Register failed" : error);
            logMessage("Register error", error.toUtf8());
        }
    } else if (type == "LOGIN") {
        QString status = message["status"].toString();
        if (status == "success") {
            currentToken = message["token"].toString();
            QString nickname = message["nickname"].toString();
            emit loginSuccess(nickname);
            heartbeatTimer->start(HEARTBEAT_INTERVAL);
            logMessage("Login success", QString("Nickname: %1").arg(nickname).toUtf8());
        } else {
            QString error = message["errorMessage"].toString();
            emit errorOccurred(error.isEmpty() ? "Login failed" : error);
            logMessage("Login error", error.toUtf8());
        }
    } else if (type == "SYSTEM") {
        QString content = message["content"].toString();
        if (content == "online_users") {
            QJsonArray users = message["onlineUsers"].toArray();
            int count = users.size();
            emit onlineUsersUpdated(users, count);
            logMessage("Online users", QString("Count: %1").arg(count).toUtf8());
        } else if (content == "history_messages") {
            QJsonArray messages = message["messages"].toArray();
            logMessage("History messages", QString("Count: %1").arg(messages.size()).toUtf8());
        } else {
            logMessage("System message", QString("Content: %1").arg(content).toUtf8());
        }
    } else if (type == "CHAT") {
        emit messageReceived(message["nickname"].toString(), message["content"].toString(), messageId);
    } else if (type == "PRIVATE_CHAT") {
        emit privateMessageReceived(message["nickname"].toString(), message["content"].toString(), messageId);
    } else if (type == "GROUP_CHAT") {
        emit groupMessageReceived(message["nickname"].toString(), message["groupName"].toString(),
                                  message["content"].toString(), messageId);
    } else if (type == "FILE") {
        QByteArray fileContent = QByteArray::fromBase64(message["content"].toString().toLatin1());
        emit fileReceived(message["nickname"].toString(), fileContent, messageId);
    } else if (type == "ERROR") {
        QString error = message["errorMessage"].toString();
        emit errorOccurred(error);
        logMessage("Server error", error.toUtf8());
    } /* else if (type == "GET_GROUPS") {
        // 暂时禁用：服务器不支持 GET_GROUPS
        // emit groupListReceived(message["groups"].toArray());
        // logMessage("Group list", QString("Received groups"));
    } */
}

void ChatClient::logMessage(const QString &prefix, const QByteArray &data) {
    QFile logFile("client.log");
    if (logFile.open(QIODevice::Append)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " [" << prefix << "] "
            << QString::fromUtf8(data).trimmed() << "\n";
        logFile.close();
    }
}