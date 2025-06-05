#include "ChatClient.h"
#include "utils/JsonConverter.h"
#include "utils/MessageHandler.h"
#include <QDebug>

ChatClient::ChatClient(QObject* parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    heartbeatTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);
    messageProcessor = new MessageProcessor(this);

    // 在这里连接 MessageProcessor 的信号到 ChatClient 的信号
    // 信号似乎是可以全局连接的, 只要在同一个项目中即可
    connect(messageProcessor, &MessageProcessor::registerSuccess, this,
            &ChatClient::registerSuccess);
    connect(messageProcessor, &MessageProcessor::loginSuccess, this, &ChatClient::loginSuccess);
    connect(messageProcessor, &MessageProcessor::messageReceived, this,
            &ChatClient::messageReceived);
    connect(messageProcessor, &MessageProcessor::privateMessageReceived, this,
            &ChatClient::privateMessageReceived);
    // 实际上后者也是一个信号, 相当于发出了信号的连锁反应
    // 这样改动松耦合, 不改变chatclient和外界的接口
    connect(messageProcessor, &MessageProcessor::groupMessageReceived, this,
            &ChatClient::groupMessageReceived);
    connect(messageProcessor, &MessageProcessor::fileReceived, this, &ChatClient::fileReceived);

    connect(messageProcessor, &MessageProcessor::onlineUsersInit, this,
            &ChatClient::onlineUsersInit);
    connect(messageProcessor, &MessageProcessor::offlineUsersInit, this,
            &ChatClient::offlineUsersInit);

    connect(messageProcessor, &MessageProcessor::historyMessagesReceived, this,
            &ChatClient::historyMessagesReceived);

    connect(messageProcessor, &MessageProcessor::someoneLogin, this, &ChatClient::someoneLogin);
    connect(messageProcessor, &MessageProcessor::someoneLogout, this, &ChatClient::someoneLogout);

    connect(messageProcessor, &MessageProcessor::errorOccurred, this, &ChatClient::errorOccurred);
    connect(socket, &QTcpSocket::connected, this, &ChatClient::handleSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &ChatClient::handleSocketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::handleSocketRead);
    connect(socket, &QAbstractSocket::errorOccurred, this, &ChatClient::handleSocketError);
    connect(heartbeatTimer, &QTimer::timeout, this, &ChatClient::sendHeartbeat);
    connect(reconnectTimer, &QTimer::timeout, this, &ChatClient::tryReconnect);
}

ChatClient::~ChatClient()
{
    disconnectFromServer();
    delete socket;
    delete heartbeatTimer;
    delete reconnectTimer;
    delete messageProcessor;
}

void ChatClient::connectToServer(const QString& host, quint16 port)
{
    this->host = host;
    this->port = port;
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        socket->connectToHost(host, port);
    }
}

void ChatClient::disconnectFromServer()
{
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->disconnectFromHost();
    }
    heartbeatTimer->stop();
    reconnectTimer->stop();
    currentToken.clear();
}

void ChatClient::login(const QString& username, const QString& password)
{
    sendJsonMessage(MessageHandler::createLoginMessage(username, password));
}

void ChatClient::registerUser(const QString& username, const QString& password,
                              const QString& nickname)
{
    sendJsonMessage(MessageHandler::createRegisterMessage(username, password, nickname));
}

void ChatClient::sendMessage(const QString& content)
{
    sendJsonMessage(MessageHandler::createChatMessage(content, currentToken));
}

void ChatClient::sendPrivateMessage(const QString& receiver, const QString& content)
{
    sendJsonMessage(MessageHandler::createPrivateChatMessage(receiver, content, currentToken));
}

void ChatClient::sendGroupMessage(const QString& groupName, const QString& content)
{
    sendJsonMessage(MessageHandler::createGroupChatMessage(groupName, content, currentToken));
}

void ChatClient::sendFile(const QString& receiver, const QByteArray& fileContent)
{
    const int CHUNK_SIZE = 8000;  // 留余量，服务器限制10000字节
    for (int i = 0; i < fileContent.size(); i += CHUNK_SIZE)
    {
        QByteArray chunk = fileContent.mid(i, CHUNK_SIZE);
        QJsonObject message = MessageHandler::createFileMessage(receiver, chunk, currentToken);
        message["part"] = i / CHUNK_SIZE + 1;
        message["totalParts"] = (fileContent.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;
        sendJsonMessage(message);
    }
}

void ChatClient::logout()
{
    if (!currentToken.isEmpty())
    {
        sendJsonMessage(MessageHandler::createLogoutMessage(currentToken));
        currentToken.clear();
        heartbeatTimer->stop();
    }
}

void ChatClient::requestGroupList()
{
    emit errorOccurred("群组列表功能未实现");
}

void ChatClient::handleSocketConnected()
{
    reconnectAttempts = 0;
    reconnectTimer->stop();
    heartbeatTimer->start(HEARTBEAT_INTERVAL);
    emit connected();
}

void ChatClient::handleSocketDisconnected()
{
    heartbeatTimer->stop();
    currentToken.clear();
    emit disconnected();
    tryReconnect();
}

void ChatClient::handleSocketError(QAbstractSocket::SocketError)
{
    QString errorMessage = socket->errorString();
    emit errorOccurred(errorMessage);
}

void ChatClient::handleSocketRead()
{
    while (socket->canReadLine())
    {
        QByteArray data = socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull())
        {
            emit errorOccurred("无效的 JSON 格式");
            continue;
        }
        bool heartbeatActive = heartbeatTimer->isActive();
        messageProcessor->processMessage(doc.object(), currentToken, heartbeatActive);
        if (heartbeatActive && !heartbeatTimer->isActive())
        {
            heartbeatTimer->start(HEARTBEAT_INTERVAL);
        }
    }
}

void ChatClient::sendHeartbeat()
{
    if (!currentToken.isEmpty())
    {
        sendJsonMessage(MessageHandler::createHeartbeatMessage(currentToken));
    }
}

void ChatClient::tryReconnect()
{
    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS)
    {
        reconnectAttempts++;
        socket->connectToHost(host, port);
    }
    else
    {
        reconnectTimer->stop();
        emit errorOccurred("无法重新连接到服务器，请检查网络");
    }
}

void ChatClient::sendJsonMessage(const QJsonObject& message)
{
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    socket->write(data);
    socket->flush();
}