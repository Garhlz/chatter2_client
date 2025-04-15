#include "ChatWindow.h"
#include <QDateTime>
#include <QDebug>
#include <QException>
#include <QFile>
#include <QGuiApplication>
#include <QJsonObject> // 新增包含
#include <QMessageBox>
#include <QScreen>
#include <QStatusBar>
#include <QVBoxLayout>

ChatWindow::ChatWindow(ChatClient *client, const QString &nickname,
                       QWidget *parent)
    : QMainWindow(parent), chatClient(client), nickname(nickname),
      isInitialized(false) {
  if (!chatClient || nickname.isEmpty()) {
    qDebug() << "ChatWindow: Invalid client or nickname";
    throw std::runtime_error("无效的客户端或昵称");
  }
  try {
    qDebug() << "ChatWindow: Starting setupUi";
    setupUi();
    qDebug() << "ChatWindow: Starting connectSignals";
    connectSignals();
    qDebug() << "ChatWindow: Setting window title";
    setWindowTitle("聊天客户端 - " + nickname);
    qDebug() << "ChatWindow: Initialization completed";
    isInitialized = true;
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception during initialization:" << e.what();
    QMessageBox::critical(this, "错误",
                          QString("初始化失败: %1").arg(e.what()));
    throw;
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception during initialization:" << e.what();
    QMessageBox::critical(this, "错误",
                          QString("初始化失败: %1").arg(e.what()));
    throw;
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception during initialization";
    QMessageBox::critical(this, "错误", "初始化时发生未知错误");
    throw;
  }
}

ChatWindow::~ChatWindow() { qDebug() << "ChatWindow: Destructor called"; }

void ChatWindow::setupUi() {
  try {
    qDebug() << "ChatWindow: Creating central widget";
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    qDebug() << "ChatWindow: Creating main layout";
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    qDebug() << "ChatWindow: Creating tab widget";
    chatTabs = new QTabWidget(this);
    chatTabs->setObjectName("chatTabs");
    mainLayout->addWidget(chatTabs);

    qDebug() << "ChatWindow: Setting up tabs";
    publicChatTab = new PublicChatTab(chatClient, nickname, this);
    privateChatTab = new PrivateChatTab(chatClient, nickname, this);
    groupChatTab = new GroupChatTab(chatClient, nickname, this);

    chatTabs->addTab(publicChatTab, "公共聊天");
    chatTabs->addTab(privateChatTab, "私聊");
    chatTabs->addTab(groupChatTab, "群聊");

    qDebug() << "ChatWindow: Setting up status bar";
    QStatusBar *statusBar = new QStatusBar(this);
    statusBar->setObjectName("statusBar");
    statusLabel = new QLabel("已连接");
    statusLabel->setObjectName("statusLabel");
    onlineCountLabel = new QLabel("在线人数: 0");
    onlineCountLabel->setObjectName("onlineCountLabel");
    QPushButton *logoutButton = new QPushButton("登出");
    logoutButton->setObjectName("logoutButton");
    statusBar->addWidget(statusLabel);
    statusBar->addWidget(onlineCountLabel);
    statusBar->addPermanentWidget(logoutButton);
    setStatusBar(statusBar);

    qDebug() << "ChatWindow: Setting window size and style";
    resize(1000, 750);
    setObjectName("ChatWindow");

    qDebug() << "ChatWindow: Centering window";
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
      QRect screenGeometry = screen->availableGeometry();
      QSize windowSize = size();
      int x = (screenGeometry.width() - windowSize.width()) / 2;
      int y = (screenGeometry.height() - windowSize.height()) / 2;
      move(x, y);
    }

    // Ensure stylesheet is applied
    QFile styleFile(":/styles/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
      setStyleSheet(styleFile.readAll());
      qDebug() << "ChatWindow: Applied stylesheet";
    } else {
      qWarning() << "ChatWindow: Could not open styles.qss: "
                 << styleFile.errorString();
    }

    qDebug() << "ChatWindow: setupUi completed";
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in setupUi:" << e.what();
    throw;
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in setupUi:" << e.what();
    throw;
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in setupUi";
    throw std::runtime_error("Unknown error in setupUi");
  }
}

void ChatWindow::connectSignals() {
  try {
    qDebug() << "ChatWindow: Connecting signals";
    connect(chatClient, &ChatClient::messageReceived, this,
            &ChatWindow::handleMessageReceived);
    connect(chatClient, &ChatClient::privateMessageReceived, this,
            &ChatWindow::handlePrivateMessageReceived);
    connect(chatClient, &ChatClient::groupMessageReceived, this,
            &ChatWindow::handleGroupMessageReceived);
    connect(chatClient, &ChatClient::fileReceived, this,
            &ChatWindow::handleFileReceived);
    connect(chatClient, &ChatClient::onlineUsersUpdated, this,
            &ChatWindow::handleOnlineUsersUpdated);
    connect(chatClient, &ChatClient::groupListReceived, this,
            &ChatWindow::handleGroupListReceived);
    connect(chatClient, &ChatClient::historyMessagesReceived, this,
            &ChatWindow::handleHistoryMessagesReceived);
    connect(chatClient, &ChatClient::errorOccurred, this,
            &ChatWindow::handleError);
    connect(statusBar()->findChild<QPushButton *>(), &QPushButton::clicked,
            this, &ChatWindow::handleLogout);
    qDebug() << "ChatWindow: Signals connected";
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in connectSignals:" << e.what();
    throw;
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in connectSignals:" << e.what();
    throw;
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in connectSignals";
    throw std::runtime_error("Unknown error in connectSignals");
  }
}

void ChatWindow::handleMessageReceived(const QString &sender,
                                       const QString &content,
                                       qint64 messageId) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring messageReceived before initialization";
    return;
  }
  if (messageId > 0 && displayedMessages.contains(messageId))
    return;
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  publicChatTab->appendMessage(sender, content, timestamp);
  if (messageId > 0)
    displayedMessages.insert(messageId);
}

void ChatWindow::handlePrivateMessageReceived(const QString &sender,
                                              const QString &content,
                                              qint64 messageId) {
  if (!isInitialized) {
    qDebug()
        << "ChatWindow: Ignoring privateMessageReceived before initialization";
    return;
  }
  if (messageId > 0 && displayedMessages.contains(messageId))
    return;
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  privateChatTab->appendMessage(sender, content, timestamp);
  chatTabs->setCurrentWidget(privateChatTab);
  if (messageId > 0)
    displayedMessages.insert(messageId);
}

void ChatWindow::handleGroupMessageReceived(const QString &sender,
                                            const QString &groupName,
                                            const QString &content,
                                            qint64 messageId) {
  if (!isInitialized) {
    qDebug()
        << "ChatWindow: Ignoring groupMessageReceived before initialization";
    return;
  }
  if (messageId > 0 && displayedMessages.contains(messageId))
    return;
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  groupChatTab->appendMessage(
      sender, QString("[%1] %2").arg(groupName).arg(content), timestamp);
  chatTabs->setCurrentWidget(groupChatTab);
  if (messageId > 0)
    displayedMessages.insert(messageId);
}

void ChatWindow::handleFileReceived(const QString &sender,
                                    const QByteArray &fileContent,
                                    qint64 messageId) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring fileReceived before initialization";
    return;
  }
  if (messageId > 0 && displayedMessages.contains(messageId))
    return;
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  privateChatTab->handleFileReceived(sender, fileContent, timestamp);
  if (messageId > 0)
    displayedMessages.insert(messageId);
}

void ChatWindow::handleOnlineUsersUpdated(const QJsonArray &users, int count) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring onlineUsersUpdated before initialization";
    return;
  }
  privateChatTab->updateOnlineUsers(users);
  onlineCountLabel->setText(QString("在线人数: %1").arg(count));
}

void ChatWindow::handleGroupListReceived(const QJsonArray &groups) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring groupListReceived before initialization";
    return;
  }
  groupChatTab->updateGroupList(groups);
}

void ChatWindow::handleHistoryMessagesReceived(
    const QJsonArray &messages) { // 似乎所有类型都处理了
  if (!isInitialized) {
    qDebug()
        << "ChatWindow: Ignoring historyMessagesReceived before initialization";
    return;
  }
  for (const QJsonValue &msg : messages) {
    if (!msg.isObject())
      continue;
    QJsonObject message = msg.toObject();

    if (!message.contains("type") || !message.contains("nickname") ||
        !message.contains("content"))
      continue;

    QString type = message["type"].toString();
    QString sender = message["nickname"].toString();
    QString content = message["content"].toString();
    qint64 messageId =
        message.contains("messageId") && !message["messageId"].isNull()
            ? message["messageId"].toVariant().toLongLong()
            : 0;

    if (messageId > 0 && displayedMessages.contains(messageId))
      continue;

    QString timestamp =
        message.contains("timestamp") && !message["timestamp"].isNull()
            ? QDateTime::fromString(message["timestamp"].toString(),
                                    Qt::ISODate)
                  .toString("hh:mm:ss")
            : QDateTime::currentDateTime().toString("hh:mm:ss");

    if (type == "CHAT") {
      publicChatTab->appendMessage(sender, content, timestamp);
    } else if (type == "PRIVATE_CHAT") {
      privateChatTab->appendMessage(sender, content, timestamp);
    } else if (type == "GROUP_CHAT") {
      if (!message.contains("groupName"))
        continue;
      groupChatTab->appendMessage(
          sender,
          QString("[%1] %2").arg(message["groupName"].toString()).arg(content),
          timestamp);
    } else if (type == "FILE") {
      privateChatTab->appendMessage(sender, "[接收文件] " + content, timestamp);
    }

    if (messageId > 0)
      displayedMessages.insert(messageId);
  }
}

void ChatWindow::handleLogout() {
  chatClient->logout();
  close();
}

void ChatWindow::handleError(const QString &error) {
  statusLabel->setText(error);
  QMessageBox::warning(this, "错误", error);
}

void ChatWindow::appendMessageBubble(QWidget *container, const QString &sender,
                                     const QString &content,
                                     const QString &timestamp,
                                     const QString &avatar) {
  QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(container->layout());
  if (!layout) {
    qDebug() << "ChatWindow: Invalid container layout";
    return;
  }

  if (layout->count() > 0) {
    QLayoutItem *item = layout->itemAt(layout->count() - 1);
    if (item->spacerItem()) {
      layout->removeItem(item);
      delete item;
    }
  }

  MessageBubble *bubble = new MessageBubble(avatar, sender, content, timestamp,
                                            sender == nickname, container);
  layout->addWidget(bubble);
  layout->addStretch();
}