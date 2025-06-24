#include "ChatClient.h"
#include "utils/JsonConverter.h"
#include "utils/MessageHandler.h"
#include "utils/UserInfo.h"
#include <QDebug>
#include "GlobalEventBus.h"
#include <QDateTime>         // 用于随机抖动
#include <QRandomGenerator>  // 用于随机抖动
#include <QMetaEnum>
// 常量定义
const int HEARTBEAT_INTERVAL = 15000;        // 15 秒
const int SERVER_HEARTBEAT_TIMEOUT = 45000;  // 服务器3个心跳周期未响应，45秒
const int INITIAL_RECONNECT_DELAY = 1000;    // 首次重连延迟 1 秒
const int MAX_RECONNECT_DELAY = 60000;       // 最大重连延迟 60 秒
const int MAX_RECONNECT_ATTEMPTS = 10;       // 最大重连尝试次数
const int CONNECTION_ATTEMPT_TIMEOUT = 10000; // 10秒连接超时

ChatClient::ChatClient(QObject* parent) : QObject(parent),
    socket(new QTcpSocket(this)),
    heartbeatTimer(new QTimer(this)),
    reconnectTimer(new QTimer(this)),
    messageProcessor(new MessageProcessor(this)),
    serverHeartbeatTimeoutTimer(new QTimer(this)),
    reconnectAttempts(0),
    currentReconnectDelay(INITIAL_RECONNECT_DELAY),
    m_connectionState(ConnectionState::Disconnected), // 确保初始状态正确
    m_isUserLoggingOut(false), // 初始化标志位
    connectionAttemptTimer(new QTimer(this))
{
    // 在这里连接 MessageProcessor 的信号到 ChatClient 的信号

    // 注册成功
    connect(messageProcessor, &MessageProcessor::registerSuccess, this,
            [this]()
            {
                // 注册成功不代表业务连接完成。
                // 此时不应启动心跳，心跳应在登录成功后启动
                emit registerSuccess();  // 转发信号给外部
            });

    // 登录成功
    connect(
        messageProcessor, &MessageProcessor::loginSuccess, this,
        [this](const QString& username, const QString& nickname, const QString& token)
        {
            this->currentToken = token;

            startHeartbeats();                               // 启动心跳和服务器心跳超时检测
            setConnectionState(ConnectionState::Connected);  // 登录成功才认为是真正“连接”并可交互
            emit loginSuccess(username, nickname);           // 转发信号给外部
        });

    // 业务逻辑信号转发
    connect(messageProcessor, &MessageProcessor::messageReceived, this,
            &ChatClient::messageReceived);
    connect(messageProcessor, &MessageProcessor::privateMessageReceived, this,
            &ChatClient::privateMessageReceived);
    connect(messageProcessor, &MessageProcessor::onlineUsersInit, this,
            &ChatClient::onlineUsersInit);
    connect(messageProcessor, &MessageProcessor::offlineUsersInit, this,
            &ChatClient::offlineUsersInit);
    connect(messageProcessor, &MessageProcessor::historyMessagesReceived, this,
            &ChatClient::historyMessagesReceived);
    connect(messageProcessor, &MessageProcessor::someoneLogin, this, &ChatClient::someoneLogin);
    connect(messageProcessor, &MessageProcessor::someoneLogout, this, &ChatClient::someoneLogout);
    connect(messageProcessor, &MessageProcessor::errorOccurred, this, &ChatClient::errorOccurred); // 业务层错误

    // QTcpSocket 信号连接
    connect(socket, &QTcpSocket::connected, this, &ChatClient::handleSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &ChatClient::handleSocketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::handleSocketRead);
    connect(socket, &QAbstractSocket::errorOccurred, this, &ChatClient::handleSocketError);
    connect(socket, &QTcpSocket::stateChanged, this, &ChatClient::onSocketStateChanged);

    // 事件总线信号连接
    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupMessage, this,
            &ChatClient::sendGroupMessage);
    connect(GlobalEventBus::instance(), &GlobalEventBus::taskSubmitted, this,
            &ChatClient::sendGroupTask);

    // 定时器信号连接
    connect(heartbeatTimer, &QTimer::timeout, this, &ChatClient::sendHeartbeat);
    connect(reconnectTimer, &QTimer::timeout, this, &ChatClient::tryReconnect);
    connect(serverHeartbeatTimeoutTimer, &QTimer::timeout, this,
            &ChatClient::handleServerHeartbeatTimeout);

    // 新增连接超时处理槽函数
    connect(connectionAttemptTimer, &QTimer::timeout, this, &ChatClient::handleConnectionAttemptTimeout);
}

// 析构函数：确保所有资源被释放和定时器停止
ChatClient::~ChatClient()
{
    stopAllNetworkActivity(); // 停止所有定时器和 socket 活动
    // QObject 的父子关系会处理子对象的释放，但显式停止定时器是良好实践
    qDebug() << "ChatClient destroyed.";
}

// 修改 setConnectionState，添加 message 参数以提供更详细的日志和信号信息
void ChatClient::setConnectionState(ConnectionState newState) // 接口不变，内部可以添加一个 message 参数
{
    // 如果实际接口不允许 message 参数，则不修改
    // 假设内部使用 string msg 来方便调试
    QString msg = QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(newState));

    if (m_connectionState == newState) return;

    m_connectionState = newState;
    qDebug() << "ChatClient State Changed: " << msg;
    emit connectionStateChanged(m_connectionState);

    // 兼容旧的信号，可以逐步移除这些兼容性信号
    if (m_connectionState == ConnectionState::Connected)
    {
        emit connected();  // 兼容旧的信号
    }
    else if (m_connectionState == ConnectionState::Disconnected)
    {
        emit disconnected();  // 兼容旧的信号
    }
    else if (m_connectionState == ConnectionState::Reconnecting)
    {
        emit reconnecting(reconnectAttempts);  // 兼容旧的信号
    }
    else if (m_connectionState == ConnectionState::Error)
    {
        // 这里的 connectionError 专用于网络错误导致的状态改变
        // 业务逻辑错误由 messageProcessor::errorOccurred 转发
        emit connectionError("网络连接出现问题"); // 提供一个默认的错误消息
    }
}

void ChatClient::connectToServer(const QString& host, quint16 port)
{
    this->host = host;
    this->port = port;

    // 在尝试连接之前，停止所有之前的活动，确保 socket 状态干净
    stopAllNetworkActivity();
    resetReconnectLogic(); // 确保重连参数在首次连接时是初始值

    // 只有当 socket 处于 UnconnectedState 时才发起连接
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        setConnectionState(ConnectionState::Connecting);
        socket->connectToHost(host, port);
        connectionAttemptTimer->start(CONNECTION_ATTEMPT_TIMEOUT);
    }
    else
    {
        qDebug() << "connectToServer: Socket is not in UnconnectedState. Current state:"
                 << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socket->state());
        // 如果 socket 已经处于其他状态（如 ConnectingState），则不重复调用 connectToHost()
        // 确保 m_connectionState 正确反映了 socket 的意图状态
        if (socket->state() == QAbstractSocket::ConnectingState) {
            setConnectionState(ConnectionState::Connecting);
        }
    }
}

void ChatClient::disconnectFromServer(bool disconnect)
{
    // 如果 disconnect 为 true，表示用户主动登出或需要彻底断开
    if(disconnect)
    {
        m_isUserLoggingOut = true; // 设置用户主动登出标志
        stopAllNetworkActivity(); // 停止所有活动，包括强制关闭 socket

        // 清理业务相关数据
        currentToken.clear();
        UserInfo::instance().clear();
        // 即使 socket 已经 Unconnected，也确保设置状态
        setConnectionState(ConnectionState::Disconnected);
        qDebug() << "ChatClient: Explicit disconnect triggered.";
    }
    else
    {
        // 如果 disconnect 为 false，表示只是清空持久化内容，不主动断开连接
        // 这种情况下，socket 保持连接，心跳和重连机制也保持活跃
        // 仅仅清空 Token 和 UserInfo
        currentToken.clear();
        UserInfo::instance().clear();
        qDebug() << "ChatClient: Persistent content cleared, connection not actively severed.";
    }
}

// 新增私有辅助方法：集中停止所有网络相关活动（定时器和socket）
void ChatClient::stopAllNetworkActivity()
{
    heartbeatTimer->stop();
    serverHeartbeatTimeoutTimer->stop();
    reconnectTimer->stop();
    connectionAttemptTimer->stop();
    reconnectAttempts = 0; // 重置重连尝试次数
    currentReconnectDelay = INITIAL_RECONNECT_DELAY; // 重置延迟

    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort(); // 强制中断任何挂起的连接或发送操作，立即让 socket 进入 UnconnectedState
        qDebug() << "ChatClient: Socket aborted to stop all activity.";
    }
    // 不在这里设置 Disconnected 状态，让 socket 的 disconnected 信号处理或外部调用来设置最终状态
}


void ChatClient::login(const QString& username, const QString& password)
{
    // 只有当 TCP 连接已建立（socket 处于 ConnectedState）时才发送登录请求
    if (socket->state() == QAbstractSocket::ConnectedState) {
        sendJsonMessage(MessageHandler::createLoginMessage(username, password));
    } else {
        qWarning() << "Login failed: Socket not connected. Current state:"
                   << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socket->state());
        emit errorOccurred("无法登录，请先连接服务器。");
    }
}

void ChatClient::registerUser(const QString& username, const QString& password,
                              const QString& nickname)
{
    // 只有当 TCP 连接已建立时才发送注册请求
    if (socket->state() == QAbstractSocket::ConnectedState) {
        sendJsonMessage(MessageHandler::createRegisterMessage(username, password, nickname));
    } else {
        qWarning() << "Register failed: Socket not connected. Current state:"
                   << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socket->state());
        emit errorOccurred("无法注册，请先连接服务器。");
    }
}

void ChatClient::sendMessage(const QString& content)
{
    // 只有当业务层状态为 Connected（已登录）且 Token 有效时才发送消息
    if (m_connectionState == ConnectionState::Connected && !currentToken.isEmpty())
    {
        sendJsonMessage(MessageHandler::createChatMessage(content, currentToken));
    }
    else
    {
        qWarning() << "Message not sent: Not connected or not logged in. Current state:"
                   << QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(m_connectionState));
        emit errorOccurred("消息发送失败：您可能已断开连接或未登录。");
    }
}

void ChatClient::sendPrivateMessage(const QString& receiver, const QString& content)
{
    // 只有当业务层状态为 Connected（已登录）且 Token 有效时才发送消息
    if (m_connectionState == ConnectionState::Connected && !currentToken.isEmpty())
    {
        sendJsonMessage(MessageHandler::createPrivateChatMessage(receiver, content, currentToken));
    }
    else
    {
        qWarning() << "Private message not sent: Not connected or not logged in. Current state:"
                   << QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(m_connectionState));
        emit errorOccurred("私聊消息发送失败：您可能已断开连接或未登录。");
    }
}

void ChatClient::sendGroupMessage(long groupId, const QString& content)
{
    // 只有当业务层状态为 Connected（已登录）且 Token 有效时才发送消息
    if (m_connectionState == ConnectionState::Connected && !currentToken.isEmpty())
    {
        sendJsonMessage(MessageHandler::createGroupChatMessage(
            UserInfo::instance().userId(), UserInfo::instance().username(),
            UserInfo::instance().nickname(), groupId, content, currentToken));
    }
    else
    {
        qWarning() << "Group message not sent: Not connected or not logged in. Current state:"
                   << QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(m_connectionState));
        emit errorOccurred("群聊消息发送失败：您可能已断开连接或未登录。");
    }
}

void ChatClient::sendGroupTask(GroupTask* task)
{
    // 只有当业务层状态为 Connected（已登录）且 Token 有效时才发送任务
    if (m_connectionState == ConnectionState::Connected && !currentToken.isEmpty())
    {
        messageProcessor->insert(task->getOperationId(), task);
        sendJsonMessage(MessageHandler::createGroupTask(task));
    }
    else
    {
        qWarning() << "Group task not sent: Not connected or not logged in. Current state:"
                   << QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(m_connectionState));
        emit errorOccurred("任务发送失败：您可能已断开连接或未登录。");
    }
}

void ChatClient::handleSocketConnected()
{
    resetReconnectLogic(); // TCP 连接成功，重置重连计数和延迟
    connectionAttemptTimer->stop();
    // 这里, 在状态变化的时候, 就会发出信号了
    setConnectionState(ConnectionState::Connected);
    qDebug() << "Socket connected (TCP level).";
}

void ChatClient::handleSocketDisconnected()
{
    qDebug() << "Socket disconnected.";
    stopHeartbeats(); // 连接断开，停止心跳
    connectionAttemptTimer->stop(); // 断开连接，停止连接尝试超时定时器
    // 如果是用户主动登出，不触发重连，并重置标志位
    if (m_isUserLoggingOut) {
        setConnectionState(ConnectionState::Disconnected);
        m_isUserLoggingOut = false; // 重置标志
        qDebug() << "Disconnected due to user logout, no reconnect initiated.";
        return;
    }

    // 如果不是用户主动登出，且之前是 Connected 或正在尝试连接/重连，则调度重连
    if (m_connectionState == ConnectionState::Connected ||    // 之前是 Connected 但断开
        m_connectionState == ConnectionState::Connecting ||   // 正在 Connecting 但断开
        m_connectionState == ConnectionState::Reconnecting || // 正在 Reconnecting 但又断开
        m_connectionState == ConnectionState::Error)          // 之前是 Error 状态也尝试重连
    {
        setConnectionState(ConnectionState::Disconnected); // 临时设置为 Disconnected
        scheduleReconnect(); // 尝试重连
    }
    else
    {
        // 如果已经是 Disconnected 或其他非活跃状态，可能不需要额外动作
        setConnectionState(ConnectionState::Disconnected);
    }

    // 清理 Token 和 UserInfo，无论是否重连，这些都应该被清除
    currentToken.clear();
    UserInfo::instance().clear();
}

void ChatClient::handleSocketError(QAbstractSocket::SocketError socketError)
{

    connectionAttemptTimer->stop(); // 断开连接，停止连接尝试超时定时器
    QString errorMessage = socket->errorString();
    qWarning() << "Socket Error: " << errorMessage << " (" << socketError << ")";

    // 如果是用户主动登出导致，不触发重连和错误状态，因为断开是预期行为
    if (m_isUserLoggingOut) {
        qDebug() << "Socket error during user logout, ignoring automatic reconnect.";
        return;
    }

    // 某些错误（如连接拒绝）可能不会触发 disconnected 信号，所以这里也调度重连
    if (socketError == QAbstractSocket::HostNotFoundError ||
        socketError == QAbstractSocket::ConnectionRefusedError ||
        socketError == QAbstractSocket::RemoteHostClosedError ||
        socketError == QAbstractSocket::SocketTimeoutError ||
        socketError == QAbstractSocket::NetworkError)
    {
        // 如果 socket 已经处于 Closing 或 Unconnected，则等待 disconnected 信号或下一个 tryReconnect 周期
        if (socket->state() != QAbstractSocket::ClosingState && socket->state() != QAbstractSocket::UnconnectedState) {
            socket->abort(); // 强制 abort，以确保 socket 进入 UnconnectedState
        }
        setConnectionState(ConnectionState::Error); // 标记为错误状态
        scheduleReconnect(); // 尝试重连
    }
    else
    {
        // 其他类型的错误，可能需要用户干预，不触发重连
        setConnectionState(ConnectionState::Error);
        emit errorOccurred("连接错误：" + errorMessage); // 报告给UI业务层错误
        emit connectionError("网络连接出现非预期错误：" + errorMessage); // 报告给UI连接层错误
        resetReconnectLogic(); // 停止重连尝试
    }
}

void ChatClient::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    // 这个槽用于更精确地管理 m_connectionState
    switch (socketState)
    {
        case QAbstractSocket::UnconnectedState:
            // 只有当 m_connectionState 不是 Disconnected 且不是用户主动登出时，才设置为 Disconnected
            if (m_connectionState != ConnectionState::Disconnected && !m_isUserLoggingOut) {
                 setConnectionState(ConnectionState::Disconnected);
            }
            break;
        case QAbstractSocket::HostLookupState:
        case QAbstractSocket::ConnectingState:
            // 如果当前不是 Reconnecting 状态，则设置为 Connecting
            // 避免 Reconnecting 状态被 Connecting 覆盖（例如，socket 内部转换状态）
            if (m_connectionState != ConnectionState::Reconnecting)
            {
                setConnectionState(ConnectionState::Connecting);
            }
            break;
        case QAbstractSocket::ConnectedState:
            // TCP 连接建立，但业务层尚未“连接”（未登录），所以 m_connectionState 保持不变
            // 只有登录成功后，loginSuccess 信号的槽函数才会将 m_connectionState 设置为 Connected
            break;
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ClosingState:
            // ClosingState 表示 socket 正在关闭，此时不应尝试连接
            // 确保 m_connectionState 不会停留在 Connected 或 Connecting
            if (m_connectionState == ConnectionState::Connected || m_connectionState == ConnectionState::Connecting || m_connectionState == ConnectionState::Reconnecting) {
                setConnectionState(ConnectionState::Disconnected);
            }
            break;
        case QAbstractSocket::ListeningState:
            break;
    }
    qDebug() << "Socket State Changed: " << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socketState);
}

void ChatClient::handleSocketRead()
{
    while (socket->canReadLine())
    {
        QByteArray data = socket->readLine().trimmed();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isNull() || !doc.isObject())
        {
            emit errorOccurred("无效的 JSON 格式或非对象消息");
            continue;
        }

        // 收到任何消息都表示服务器活跃，重置服务器心跳超时定时器
        // serverHeartbeatTimeoutTimer->start(SERVER_HEARTBEAT_TIMEOUT);
        startHeartbeats(); // 包含了上面那条
        messageProcessor->processMessage(doc.object());
    }
}
// todo 这里的设计需要深思熟虑... 如果非登录状态也要保持连接, 可能不需要发送token?
// 如果取消登录也取消了连接, 需要在登录/注册按钮中加入socket连接的逻辑...
// 还是不要token好了...
void ChatClient::sendHeartbeat()
{
    // 只有当业务层状态为 Connected 时才发送心跳
    // ! change 修改为只有在登陆状态的时候才发送心跳
    if (m_connectionState == ConnectionState::Connected && UserInfo::instance().online())
    {
        sendJsonMessage(MessageHandler::createHeartbeatMessage(currentToken));
        qDebug() << "Sent Heartbeat.";
    }
    else
    {
        qDebug() << "Heartbeat not sent: not in Connected state. Current state:"
                 << QMetaEnum::fromType<ConnectionState>().valueToKey(static_cast<int>(m_connectionState));
        // 如果状态不符合，并且心跳定时器还在运行，停止它. 因为断开连接也要停止心跳
        if (heartbeatTimer->isActive()) {
            stopHeartbeats();
        }
    }
}

void ChatClient::handleServerHeartbeatTimeout()
{
    qWarning() << "Server heartbeat timed out. Forcing disconnect to trigger reconnect.";
    // 服务器长时间无响应，主动断开连接，这将触发 handleSocketDisconnected，进而启动重连
    socket->abort(); // 使用 abort() 强制关闭，立即触发 disconnected 信号
    // setConnectionState 的更新将在 handleSocketDisconnected 中完成
}

void ChatClient::tryReconnect()
{
    // 如果用户正在主动登出，停止重连
    if (m_isUserLoggingOut) {
        qDebug() << "tryReconnect: User is logging out, stopping reconnect attempts.";
        resetReconnectLogic();
        return;
    }

    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS)
    {
        reconnectAttempts++;
        int delay = currentReconnectDelay;
        // 添加随机抖动，避免“惊群效应”
        delay += QRandomGenerator::global()->bounded(delay / 2); // 随机增加 0 到 delay/2 的时间

        qDebug() << QString("Attempting to reconnect... Attempt %1, next delay %2ms")
                        .arg(reconnectAttempts)
                        .arg(delay);

        currentReconnectDelay = qMin(currentReconnectDelay * 2, MAX_RECONNECT_DELAY); // 指数退避

        // 确保 socket 处于 UnconnectedState 才进行连接尝试
        QAbstractSocket::SocketState currentSocketState = socket->state();
        if (currentSocketState == QAbstractSocket::UnconnectedState)
        {
            setConnectionState(ConnectionState::Reconnecting); // 设置为重连状态
            socket->connectToHost(host, port);
        }
        else if (currentSocketState == QAbstractSocket::ClosingState ||
                 currentSocketState == QAbstractSocket::ConnectingState)
        {
            // 如果 socket 正在关闭或连接中，等待其状态变化，不立即尝试连接
            qDebug() << "tryReconnect: Socket is in" << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(currentSocketState) << "state, waiting for next retry.";
            // 在这种情况下，定时器会继续运行，并在下一个超时周期再次调用 tryReconnect
        }
        else
        {
            // 对于 ConnectedState, BoundState, ListeningState（不应在重连时出现）
            // 强制 abort() 以清理状态，使其回到 UnconnectedState
            qDebug() << "tryReconnect: Socket in unexpected state (" << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(currentSocketState) << "), aborting to clear.";
            socket->abort(); // 强制清理，希望下次 tryReconnect 时能变为 UnconnectedState
            // 此时不设置状态，等待 onSocketStateChanged 或 handleSocketDisconnected 来更新
        }

        // 重新启动定时器，等待下一次尝试
        reconnectTimer->start(delay);
    }
    else
    {
        qWarning() << "Max reconnect attempts reached. Unable to reconnect.";
        reconnectTimer->stop();
        setConnectionState(ConnectionState::Error); // 最终状态：错误，无法重连
        emit errorOccurred("无法重新连接到服务器，请检查网络或稍后重试。"); // 业务层错误
        emit connectionError("无法重新连接到服务器，请检查网络或稍后重试。"); // 连接层错误
    }
}

void ChatClient::scheduleReconnect()
{
    // 只有当重连定时器未激活，并且不是用户正在主动登出时才调度重连
    if (!reconnectTimer->isActive() && !m_isUserLoggingOut)
    {
        // 确保 socket 状态适合调度重连，例如，不是处于 Connected 状态
        if (socket->state() != QAbstractSocket::ConnectedState) {
            resetReconnectLogic(); // 首次重连时重置参数
            tryReconnect();        // 立即尝试第一次重连
        } else {
            qDebug() << "scheduleReconnect: Socket is already Connected, not scheduling reconnect.";
        }
    }
    else if (reconnectTimer->isActive()) {
        qDebug() << "scheduleReconnect: Reconnect already scheduled/active.";
    } else if (m_isUserLoggingOut) {
        qDebug() << "scheduleReconnect: User is logging out, not scheduling reconnect.";
    }
}

void ChatClient::resetReconnectLogic()
{
    reconnectAttempts = 0;
    currentReconnectDelay = INITIAL_RECONNECT_DELAY;
    reconnectTimer->stop(); // 确保停止定时器
    qDebug() << "Reconnect logic reset and timer stopped.";
}

void ChatClient::startHeartbeats()
{
    if (!heartbeatTimer->isActive())
    {
        heartbeatTimer->start(HEARTBEAT_INTERVAL);
        qDebug() << "Heartbeat timer started.";
    }
    // 每次启动心跳或收到消息时，重置服务器心跳超时计时
    serverHeartbeatTimeoutTimer->stop(); // 先停止确保重置
    serverHeartbeatTimeoutTimer->start(SERVER_HEARTBEAT_TIMEOUT);
    qDebug() << "Server heartbeat timeout timer reset.";
}

void ChatClient::stopHeartbeats()
{
    heartbeatTimer->stop();
    serverHeartbeatTimeoutTimer->stop();
    qDebug() << "Heartbeats stopped.";
}

void ChatClient::sendJsonMessage(const QJsonObject& message)
{
    // 在发送消息前，再次检查 socket 状态。
    // 这里判断 ConnectedState 更为准确，因为只有建立了 TCP 连接才能发送。
    if (socket->state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(message);
        QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
        qint64 bytesWritten = socket->write(data);
        if (bytesWritten == -1) {
            qWarning() << "Failed to write to socket:" << socket->errorString();
            emit errorOccurred("发送数据失败：" + socket->errorString());
        }
        // socket->flush(); // QTcpSocket通常会自动刷新
    } else {
        qWarning() << "Attempted to send message while socket is not connected. Message type:"
                   << message["type"].toString() << ", Current socket state:"
                   << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socket->state());
        // 发送一个连接层错误信号
        emit connectionError("无法发送消息：网络未连接或状态异常。");
    }
}


// private slots:
void ChatClient::handleConnectionAttemptTimeout()
{
    qWarning() << "连接服务器超时。";
    connectionAttemptTimer->stop(); // 停止定时器

    // 强制断开socket，这会触发 handleSocketDisconnected 和 handleSocketError
    // 从而进入重连逻辑（如果不是用户主动断开）或错误状态
    if (socket->state() == QAbstractSocket::ConnectingState) {
        socket->abort(); // 立即中止连接尝试
        setConnectionState(ConnectionState::Error); // 设置为错误状态
        emit connectionError("连接服务器超时，请检查网络或重试。"); // 发出更具体的错误信号
        emit errorOccurred("连接服务器超时，请手动重连。"); // 报告给UI业务层错误
        resetReconnectLogic(); // 停止重连尝试，因为是单次手动重连的超时
    }
}