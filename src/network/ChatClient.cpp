#include "ChatClient.h"
#include "utils/JsonConverter.h"
#include "utils/MessageHandler.h"
#include <QDebug>

ChatClient::ChatClient(QObject *parent) : QObject(parent) {
  socket = new QTcpSocket(this);
  heartbeatTimer = new QTimer(this);
  reconnectTimer = new QTimer(this);

  connect(socket, &QTcpSocket::connected, this,
          &ChatClient::handleSocketConnected);
  connect(socket, &QTcpSocket::disconnected, this,
          &ChatClient::handleSocketDisconnected);
  connect(socket, &QTcpSocket::readyRead, this, &ChatClient::handleSocketRead);
  connect(socket, &QAbstractSocket::errorOccurred, this,
          &ChatClient::handleSocketError);
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

void ChatClient::registerUser(const QString &username, const QString &password,
                              const QString &nickname) {
  sendJsonMessage(
      MessageHandler::createRegisterMessage(username, password, nickname));
}

void ChatClient::sendMessage(const QString &content) {
  sendJsonMessage(MessageHandler::createChatMessage(content, currentToken));
}

void ChatClient::sendPrivateMessage(const QString &receiver,
                                    const QString &content) {
  sendJsonMessage(MessageHandler::createPrivateChatMessage(receiver, content,
                                                           currentToken));
}

void ChatClient::sendGroupMessage(const QString &groupName,
                                  const QString &content) {
  sendJsonMessage(
      MessageHandler::createGroupChatMessage(groupName, content, currentToken));
}

void ChatClient::sendFile(const QString &receiver,
                          const QByteArray &fileContent) {
  const int CHUNK_SIZE = 8000; // 留余量，服务器限制10000字节
  for (int i = 0; i < fileContent.size(); i += CHUNK_SIZE) {
    QByteArray chunk = fileContent.mid(i, CHUNK_SIZE);
    QJsonObject message =
        MessageHandler::createFileMessage(receiver, chunk, currentToken);
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
  emit errorOccurred("群组列表功能未实现");
}

void ChatClient::handleSocketConnected() {
  reconnectAttempts = 0;
  reconnectTimer->stop();
  heartbeatTimer->start(HEARTBEAT_INTERVAL);
  emit connected();
}

void ChatClient::handleSocketDisconnected() {
  heartbeatTimer->stop();
  currentToken.clear();
  emit disconnected();
  tryReconnect();
}

void ChatClient::handleSocketError(QAbstractSocket::SocketError) {
  QString errorMessage = socket->errorString();
  emit errorOccurred(errorMessage);
}

void ChatClient::handleSocketRead() {
  while (socket->canReadLine()) {
    QByteArray data = socket->readLine().trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
      emit errorOccurred("无效的 JSON 格式");
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
  } else {
    reconnectTimer->stop();
    emit errorOccurred("无法重新连接到服务器，请检查网络");
  }
}

void ChatClient::sendJsonMessage(const QJsonObject &message) {
  QJsonDocument doc(message);
  QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
  socket->write(data);
  socket->flush();
}

void ChatClient::processMessage(const QJsonObject &message) {
  if (!message.contains("type") || !message["type"].isString()) {
    emit errorOccurred("消息缺少 type 字段或格式错误");
    return;
  }

  QString type = message["type"].toString();
  qint64 messageId =
      message.contains("messageId") && !message["messageId"].isNull()
          ? message["messageId"].toVariant().toLongLong()
          : 0;

  if (type == "REGISTER") {
    if (!message.contains("status") || !message["status"].isString()) {
      emit errorOccurred("注册消息缺少 status 字段");
      return;
    }
    QString status = message["status"].toString();
    if (status == "success") {
      currentToken =
          message.contains("token") ? message["token"].toString() : "";
      emit registerSuccess(currentToken);
      heartbeatTimer->start(HEARTBEAT_INTERVAL);
    } else {
      QString error =
          message.contains("errorMessage") && !message["errorMessage"].isNull()
              ? message["errorMessage"].toString()
              : "注册失败";
      emit errorOccurred(error);
    }
  } else if (type == "LOGIN") {
    if (!message.contains("status") || !message["status"].isString()) {
      emit errorOccurred("登录消息缺少 status 字段");
      return;
    }
    QString status = message["status"].toString();
    if (status == "success") {
      if (!message.contains("token") || !message.contains("nickname") ||
          message["token"].toString().isEmpty() ||
          message["nickname"].toString().isEmpty()) {
        emit errorOccurred("登录消息缺少有效的 token 或 nickname");
        return;
      }
      currentToken = message["token"].toString();
      QString nickname = message["nickname"].toString();
      qDebug() << "ChatClient: Login success, nickname =" << nickname
               << ", token =" << currentToken;
      emit loginSuccess(nickname);
      heartbeatTimer->start(HEARTBEAT_INTERVAL);
    } else {
      QString error =
          message.contains("errorMessage") && !message["errorMessage"].isNull()
              ? message["errorMessage"].toString()
              : "登录失败";
      emit errorOccurred(error);
    }
  } else if (type == "SYSTEM") {
    if (!message.contains("content") || !message["content"].isString()) {
      emit errorOccurred("系统消息缺少 content 字段");
      return;
    }
    QString content = message["content"].toString();
    if (content == "online_users") {
      if (!message.contains("onlineUsers") ||
          !message["onlineUsers"].isArray()) {
        emit errorOccurred("在线用户消息缺少 onlineUsers 数组");
        return;
      }
      QJsonArray users = message["onlineUsers"].toArray();
      int count = users.size();
      emit onlineUsersUpdated(users, count);
    } else if (content == "history_messages") {
      qDebug() << "history messages:" << message;
      if (!message.contains("messages") || !message["messages"].isArray()) {
        emit errorOccurred("历史消息缺少 messages 数组");
        return;
      }
      QJsonArray messages = message["messages"].toArray();
      emit historyMessagesReceived(messages);
    } else {
      emit errorOccurred(QString("未知系统消息内容: %1").arg(content));
    }
  } else if (type == "CHAT") {
    if (!message.contains("nickname") || !message.contains("content")) {
      emit errorOccurred("聊天消息缺少 nickname 或 content");
      return;
    }
    emit messageReceived(message["nickname"].toString(),
                         message["content"].toString(), messageId);
  } else if (type == "PRIVATE_CHAT") {
    if (!message.contains("nickname") || !message.contains("content")) {
      emit errorOccurred("私聊消息缺少 nickname 或 content");
      return;
    }
    emit privateMessageReceived(message["nickname"].toString(),
                                message["content"].toString(), messageId);
  } else if (type == "GROUP_CHAT") {
    if (!message.contains("nickname") || !message.contains("groupName") ||
        !message.contains("content")) {
      emit errorOccurred("群聊消息缺少 nickname, groupName 或 content");
      return;
    }
    emit groupMessageReceived(message["nickname"].toString(),
                              message["groupName"].toString(),
                              message["content"].toString(), messageId);
  } else if (type == "FILE") {
    if (!message.contains("nickname") || !message.contains("content")) {
      emit errorOccurred("文件消息缺少 nickname 或 content");
      return;
    }
    QByteArray fileContent =
        QByteArray::fromBase64(message["content"].toString().toLatin1());
    emit fileReceived(message["nickname"].toString(), fileContent, messageId);
  } else if (type == "ERROR") {
    QString error =
        message.contains("errorMessage") && !message["errorMessage"].isNull()
            ? message["errorMessage"].toString()
            : "服务器错误";
    emit errorOccurred(error);
  }
}