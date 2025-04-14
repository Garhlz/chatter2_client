#include "ChatWindow.h"
#include <QDateTime>
#include <QDebug>
#include <QException>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QtConcurrent/QtConcurrent>

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
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    qDebug() << "ChatWindow: Creating tab widget";
    chatTabs = new QTabWidget(this);
    chatTabs->setObjectName("chatTabs");
    mainLayout->addWidget(chatTabs);

    // 公共聊天页
    qDebug() << "ChatWindow: Setting up public chat tab";
    publicChatTab = new QWidget();
    publicChatTab->setObjectName("publicChatTab");
    QVBoxLayout *publicLayout = new QVBoxLayout(publicChatTab);
    publicLayout->setContentsMargins(5, 5, 5, 5);

    publicChatDisplay = new QScrollArea();
    publicChatDisplay->setObjectName("publicChatDisplay");
    publicChatContainer = new QWidget();
    publicChatContainer->setObjectName("publicChatContainer");
    publicChatLayout = new QVBoxLayout(publicChatContainer);
    publicChatLayout->setAlignment(Qt::AlignTop);
    publicChatLayout->addStretch();
    publicChatDisplay->setWidget(publicChatContainer);
    publicChatDisplay->setWidgetResizable(true);

    qDebug() << "ChatWindow: Creating public input layout";
    QHBoxLayout *publicInputLayout = new QHBoxLayout();
    publicMessageInput = new QLineEdit();
    publicMessageInput->setObjectName("publicMessageInput");
    publicSendButton = new QPushButton();
    publicSendButton->setObjectName("publicSendButton");
    // publicSendButton->setIcon(QIcon(":/icons/send.png")); // 需添加图标资源
    publicInputLayout->addWidget(publicMessageInput);
    publicInputLayout->addWidget(publicSendButton);
    publicLayout->addWidget(publicChatDisplay);
    publicLayout->addLayout(publicInputLayout);

    // 私聊页
    qDebug() << "ChatWindow: Setting up private chat tab";
    privateChatTab = new QWidget();
    privateChatTab->setObjectName("privateChatTab");
    QHBoxLayout *privateLayout = new QHBoxLayout(privateChatTab);
    privateLayout->setContentsMargins(5, 5, 5, 5);

    QSplitter *privateSplitter = new QSplitter(Qt::Horizontal);
    privateSplitter->setObjectName("privateSplitter");

    QWidget *chatWidget = new QWidget();
    QVBoxLayout *privateChatLayout = new QVBoxLayout(chatWidget);
    privateChatDisplay = new QScrollArea();
    privateChatDisplay->setObjectName("privateChatDisplay");
    privateChatContainer = new QWidget();
    privateChatContainer->setObjectName("privateChatContainer");
    QVBoxLayout *privateMessagesLayout = new QVBoxLayout(privateChatContainer);
    privateMessagesLayout->setAlignment(Qt::AlignTop);
    privateMessagesLayout->addStretch();
    privateChatDisplay->setWidget(privateChatContainer);
    privateChatDisplay->setWidgetResizable(true);

    QHBoxLayout *privateInputLayout = new QHBoxLayout();
    privateMessageInput = new QLineEdit();
    privateMessageInput->setObjectName("privateMessageInput");
    privateSendButton = new QPushButton();
    privateSendButton->setObjectName("privateSendButton");
    // privateSendButton->setIcon(QIcon(":/icons/send.png"));
    sendFileButton = new QPushButton();
    sendFileButton->setObjectName("sendFileButton");
    // sendFileButton->setIcon(QIcon(":/icons/file.png")); // 需添加图标资源
    privateInputLayout->addWidget(privateMessageInput);
    privateInputLayout->addWidget(privateSendButton);
    privateInputLayout->addWidget(sendFileButton);
    privateChatLayout->addWidget(privateChatDisplay);
    privateChatLayout->addLayout(privateInputLayout);

    QWidget *usersWidget = new QWidget();
    QVBoxLayout *usersLayout = new QVBoxLayout(usersWidget);
    QLabel *usersLabel = new QLabel("在线用户");
    usersLabel->setObjectName("usersLabel");
    onlineUsersList = new QListWidget();
    onlineUsersList->setObjectName("onlineUsersList");
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(onlineUsersList);

    privateSplitter->addWidget(chatWidget);
    privateSplitter->addWidget(usersWidget);
    privateSplitter->setSizes({400, 200}); // 初始比例
    privateLayout->addWidget(privateSplitter);

    // 群聊页
    qDebug() << "ChatWindow: Setting up group chat tab";
    groupChatTab = new QWidget();
    groupChatTab->setObjectName("groupChatTab");
    QVBoxLayout *groupLayout = new QVBoxLayout(groupChatTab);
    groupLayout->setContentsMargins(5, 5, 5, 5);

    groupChatDisplay = new QScrollArea();
    groupChatDisplay->setObjectName("groupChatDisplay");
    groupChatContainer = new QWidget();
    groupChatContainer->setObjectName("groupChatContainer");
    groupChatLayout = new QVBoxLayout(groupChatContainer);
    groupChatLayout->setAlignment(Qt::AlignTop);
    groupChatLayout->addStretch();
    groupChatDisplay->setWidget(groupChatContainer);
    groupChatDisplay->setWidgetResizable(true);

    QHBoxLayout *groupInputLayout = new QHBoxLayout();
    groupCombo = new QComboBox();
    groupCombo->setObjectName("groupCombo");
    groupCombo->setPlaceholderText("选择群组");
    groupMessageInput = new QLineEdit();
    groupMessageInput->setObjectName("groupMessageInput");
    groupMessageInput->setPlaceholderText("消息内容");
    groupSendButton = new QPushButton();
    groupSendButton->setObjectName("groupSendButton");
    // groupSendButton->setIcon(QIcon(":/icons/send.png"));
    groupInputLayout->addWidget(groupCombo);
    groupInputLayout->addWidget(groupMessageInput);
    groupInputLayout->addWidget(groupSendButton);
    groupLayout->addWidget(groupChatDisplay);
    groupLayout->addLayout(groupInputLayout);

    qDebug() << "ChatWindow: Adding tabs";
    chatTabs->addTab(publicChatTab, "公共聊天");
    chatTabs->addTab(privateChatTab, "私聊");
    chatTabs->addTab(groupChatTab, "群聊");

    // 状态栏
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
    resize(900, 700);
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
    connect(publicSendButton, &QPushButton::clicked, this,
            &ChatWindow::sendMessage);
    connect(privateSendButton, &QPushButton::clicked, this,
            &ChatWindow::sendPrivateMessage);
    connect(groupSendButton, &QPushButton::clicked, this,
            &ChatWindow::sendGroupMessage);
    connect(sendFileButton, &QPushButton::clicked, this, &ChatWindow::sendFile);

    connect(publicMessageInput, &QLineEdit::returnPressed, this,
            &ChatWindow::sendMessage);
    connect(privateMessageInput, &QLineEdit::returnPressed, this,
            &ChatWindow::sendPrivateMessage);
    connect(groupMessageInput, &QLineEdit::returnPressed, this,
            &ChatWindow::sendGroupMessage);

    connect(onlineUsersList, &QListWidget::itemClicked, this,
            &ChatWindow::handleUserSelected);

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

void ChatWindow::sendMessage() {
  QString content = publicMessageInput->text().trimmed();
  if (content.isEmpty())
    return;
  if (content.toUtf8().size() > 1000) {
    QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
    return;
  }
  chatClient->sendMessage(content);
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  appendMessageBubble(publicChatContainer, nickname, content, timestamp);
  publicMessageInput->clear();
}

void ChatWindow::sendPrivateMessage() {
  QString content = privateMessageInput->text().trimmed();
  if (content.isEmpty())
    return;
  if (content.toUtf8().size() > 1000) {
    QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
    return;
  }
  if (!selectedUser.isEmpty()) {
    chatClient->sendPrivateMessage(selectedUser, content);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    appendMessageBubble(privateChatContainer, nickname, content, timestamp);
    privateMessageInput->clear();
  } else {
    QMessageBox::warning(this, "错误", "请先选择一个在线用户");
  }
}

void ChatWindow::sendGroupMessage() {
  QString groupName = groupCombo->currentText().trimmed();
  QString content = groupMessageInput->text().trimmed();
  if (content.isEmpty() || groupName.isEmpty()) {
    QMessageBox::warning(this, "错误", "群组和消息内容不能为空");
    return;
  }
  if (content.toUtf8().size() > 1000) {
    QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
    return;
  }
  chatClient->sendGroupMessage(groupName, content);
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  appendMessageBubble(groupChatContainer, nickname,
                      QString("[%1] %2").arg(groupName).arg(content),
                      timestamp);
  groupMessageInput->clear();
}

void ChatWindow::sendFile() {
  if (selectedUser.isEmpty()) {
    QMessageBox::warning(this, "错误", "请先选择一个在线用户");
    return;
  }
  QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
  if (filePath.isEmpty())
    return;
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "错误", "无法打开文件");
    return;
  }
  qint64 size = file.size();
  if (size > 10 * 1024 * 1024) {
    QMessageBox::warning(this, "错误", "文件不能超过10MB");
    file.close();
    return;
  }
  QByteArray fileContent = file.readAll();
  chatClient->sendFile(selectedUser, fileContent);
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  appendMessageBubble(privateChatContainer, nickname,
                      "[发送文件] " + QFileInfo(filePath).fileName(),
                      timestamp);
  file.close();
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
  appendMessageBubble(publicChatContainer, sender, content, timestamp);
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
  appendMessageBubble(privateChatContainer, sender, content, timestamp);
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
  QString message = QString("[%1] %2").arg(groupName).arg(content);
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  appendMessageBubble(groupChatContainer, sender, message, timestamp);
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
  QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件");
  if (saveFilePath.isEmpty())
    return;
  QFuture<void> future = QtConcurrent::run([=]() {
    QFile file(saveFilePath);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(fileContent);
      file.close();
      QMetaObject::invokeMethod(this, [=]() {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        appendMessageBubble(privateChatContainer, sender,
                            "[接收文件] " + QFileInfo(saveFilePath).fileName(),
                            timestamp);
        if (messageId > 0)
          displayedMessages.insert(messageId);
      });
    }
  });
  Q_UNUSED(future);
}

void ChatWindow::handleOnlineUsersUpdated(const QJsonArray &users, int count) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring onlineUsersUpdated before initialization";
    return;
  }
  try {
    qDebug() << "ChatWindow: Updating online users, count =" << count;
    updateOnlineUsersList(users);
    onlineCountLabel->setText(QString("在线人数: %1").arg(count));
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in handleOnlineUsersUpdated:"
             << e.what();
    QMessageBox::warning(this, "错误",
                         QString("更新在线用户失败: %1").arg(e.what()));
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in handleOnlineUsersUpdated:"
             << e.what();
    QMessageBox::warning(this, "错误",
                         QString("更新在线用户失败: %1").arg(e.what()));
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in handleOnlineUsersUpdated";
    QMessageBox::warning(this, "错误", "更新在线用户时发生未知错误");
  }
}

void ChatWindow::handleGroupListReceived(const QJsonArray &groups) {
  if (!isInitialized) {
    qDebug() << "ChatWindow: Ignoring groupListReceived before initialization";
    return;
  }
  try {
    groupCombo->clear();
    for (const auto &g : groups) {
      groupCombo->addItem(g.toString());
    }
    if (!groups.isEmpty()) {
      groupCombo->setCurrentIndex(0);
    }
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in handleGroupListReceived:"
             << e.what();
    QMessageBox::warning(this, "错误",
                         QString("更新群组列表失败: %1").arg(e.what()));
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in handleGroupListReceived:" << e.what();
    QMessageBox::warning(this, "错误",
                         QString("更新群组列表失败: %1").arg(e.what()));
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in handleGroupListReceived";
    QMessageBox::warning(this, "错误", "更新群组列表时发生未知错误");
  }
}

void ChatWindow::handleHistoryMessagesReceived(const QJsonArray &messages) {
  if (!isInitialized) {
    qDebug()
        << "ChatWindow: Ignoring historyMessagesReceived before initialization";
    return;
  }
  try {
    for (const QJsonValue &msg : messages) {
      if (!msg.isObject())
        continue;
      QJsonObject message = msg.toObject();

      if (!message.contains("type") || !message.contains("nickname") ||
          !message.contains("content")) {
        continue;
      }

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

      QWidget *container = nullptr;
      QString displayContent = content;

      if (type == "CHAT") {
        container = publicChatContainer;
      } else if (type == "PRIVATE_CHAT") {
        container = privateChatContainer;
      } else if (type == "GROUP_CHAT") {
        if (!message.contains("groupName"))
          continue;
        displayContent = QString("[%1] %2")
                             .arg(message["groupName"].toString())
                             .arg(content);
        container = groupChatContainer;
      } else if (type == "FILE") {
        displayContent = "[接收文件] " + content;
        container = privateChatContainer;
      } else {
        continue;
      }

      appendMessageBubble(container, sender, displayContent, timestamp);
      if (messageId > 0)
        displayedMessages.insert(messageId);
    }
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in handleHistoryMessagesReceived:"
             << e.what();
    QMessageBox::warning(this, "错误",
                         QString("加载历史消息失败: %1").arg(e.what()));
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in handleHistoryMessagesReceived:"
             << e.what();
    QMessageBox::warning(this, "错误",
                         QString("加载历史消息失败: %1").arg(e.what()));
  } catch (...) {
    qDebug()
        << "ChatWindow: Unknown exception in handleHistoryMessagesReceived";
    QMessageBox::warning(this, "错误", "加载历史消息时发生未知错误");
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

void ChatWindow::handleUserSelected() {
  try {
    QListWidgetItem *item = onlineUsersList->currentItem();
    if (item) {
      selectedUser = item->data(Qt::UserRole).toString();
      qDebug() << "ChatWindow: Selected user:" << selectedUser;
    }
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in handleUserSelected:" << e.what();
    QMessageBox::warning(this, "错误",
                         QString("选择用户失败: %1").arg(e.what()));
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in handleUserSelected:" << e.what();
    QMessageBox::warning(this, "错误",
                         QString("选择用户失败: %1").arg(e.what()));
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in handleUserSelected";
    QMessageBox::warning(this, "错误", "选择用户时发生未知错误");
  }
}

void ChatWindow::appendMessageBubble(QWidget *container, const QString &sender,
                                     const QString &content,
                                     const QString &timestamp,
                                     const QString &avatar) {
  try {
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!layout) {
      qDebug() << "ChatWindow: Invalid container layout";
      QMessageBox::warning(this, "错误", "无效的容器布局");
      return;
    }

    if (layout->count() > 0) {
      QLayoutItem *item = layout->itemAt(layout->count() - 1);
      if (item->spacerItem()) {
        layout->removeItem(item);
        delete item;
      }
    }

    qDebug() << "ChatWindow: Creating message bubble for sender:" << sender;
    MessageBubble *bubble = new MessageBubble(
        avatar, sender, content, timestamp, sender == nickname, container);
    layout->addWidget(bubble);
    layout->addStretch();

    QScrollArea *scrollArea =
        qobject_cast<QScrollArea *>(container->parentWidget()->parentWidget());
    if (scrollArea) {
      bool isAtBottom = scrollArea->verticalScrollBar()->value() >=
                        scrollArea->verticalScrollBar()->maximum() - 20;
      scrollArea->viewport()->update();
      if (isAtBottom) {
        QTimer::singleShot(10, scrollArea, [=]() {
          scrollArea->verticalScrollBar()->setValue(
              scrollArea->verticalScrollBar()->maximum());
        });
      }
    }
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in appendMessageBubble:" << e.what();
    QMessageBox::warning(this, "错误",
                         QString("创建消息气泡失败: %1").arg(e.what()));
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in appendMessageBubble:" << e.what();
    QMessageBox::warning(this, "错误",
                         QString("创建消息气泡失败: %1").arg(e.what()));
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in appendMessageBubble";
    QMessageBox::warning(this, "错误", "创建消息气泡时发生未知错误");
  }
}

void ChatWindow::updateOnlineUsersList(const QJsonArray &users) {
  try {
    qDebug() << "ChatWindow: Clearing online users list";
    onlineUsersList->clear();
    for (const QJsonValue &user : users) {
      QJsonObject userObj = user.toObject();
      QString username = userObj["username"].toString();
      QString nickname = userObj["nickname"].toString();
      QString displayText = QString("%1 (%2)").arg(nickname).arg(username);
      qDebug() << "ChatWindow: Adding user:" << displayText;
      QListWidgetItem *item = new QListWidgetItem(displayText);
      item->setData(Qt::UserRole, username);
      onlineUsersList->addItem(item);
    }
  } catch (const QException &e) {
    qDebug() << "ChatWindow: Qt exception in updateOnlineUsersList:"
             << e.what();
    throw;
  } catch (const std::exception &e) {
    qDebug() << "ChatWindow: Exception in updateOnlineUsersList:" << e.what();
    throw;
  } catch (...) {
    qDebug() << "ChatWindow: Unknown exception in updateOnlineUsersList";
    throw std::runtime_error("Unknown error in updateOnlineUsersList");
  }
}