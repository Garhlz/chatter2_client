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
    QJsonObject message{{"type", "GET_GROUPS"}, {"token", currentToken}};
    sendJsonMessage(message);
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
    qint64 messageId = message["messageId"].toVariant().toLongLong();

    if (type == "SYSTEM") {
        QString content = message["content"].toString();
        if (content == "登录成功" || content == "注册成功") {
            currentToken = message["token"].toString();
            QString nickname = message["nickname"].toString();
            if (content == "登录成功") {
                emit loginSuccess(nickname);
            } else {
                emit registerSuccess(currentToken);
            }
            heartbeatTimer->start(HEARTBEAT_INTERVAL);
        }
        if (message.contains("onlineUsers") && message.contains("onlineCount")) {
            emit onlineUsersUpdated(message["onlineUsers"].toArray(), message["onlineCount"].toInt());
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
        emit errorOccurred(message["errorMessage"].toString());
    } else if (type == "GET_GROUPS") {
        emit groupListReceived(message["groups"].toArray());
    }
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