#include "PrivateChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>

PrivateChatTab::PrivateChatTab(ChatClient *client, const QString &nickname,
                               QWidget *parent)
    : QWidget(parent), chatClient(client), nickname(nickname) {
  setupUi();
  connectSignals();
}

void PrivateChatTab::setupUi() {
  QVBoxLayout *privateTabLayout = new QVBoxLayout(this);
  privateTabLayout->setContentsMargins(8, 8, 8, 8);
  privateTabLayout->setSpacing(10);

  QSplitter *privateSplitter = new QSplitter(Qt::Horizontal);
  privateSplitter->setObjectName("privateSplitter");

  QWidget *chatWidget = new QWidget();
  QVBoxLayout *privateChatLayout = new QVBoxLayout(chatWidget);
  privateChatLayout->setContentsMargins(0, 0, 0, 0);
  privateChatDisplay = new QScrollArea();
  privateChatDisplay->setObjectName("privateChatDisplay");
  privateChatDisplay->setMinimumHeight(400);
  privateChatDisplay->setMinimumWidth(600);
  privateChatContainer = new QWidget();
  privateChatContainer->setObjectName("privateChatContainer");
  QVBoxLayout *privateMessagesLayout = new QVBoxLayout(privateChatContainer);
  privateMessagesLayout->setAlignment(Qt::AlignTop);
  privateMessagesLayout->setContentsMargins(0, 0, 0, 0);
  privateChatDisplay->setWidget(privateChatContainer);
  privateChatDisplay->setWidgetResizable(true);

  QHBoxLayout *privateInputLayout = new QHBoxLayout();
  privateMessageInput = new QLineEdit();
  privateMessageInput->setObjectName("privateMessageInput");
  privateMessageInput->setPlaceholderText("输入私聊消息...");
  privateSendButton = new QPushButton("发送");
  privateSendButton->setObjectName("privateSendButton");
  sendFileButton = new QPushButton("文件");
  sendFileButton->setObjectName("sendFileButton");
  privateInputLayout->addWidget(privateMessageInput);
  privateInputLayout->addWidget(privateSendButton);
  privateInputLayout->addWidget(sendFileButton);
  privateChatLayout->addWidget(privateChatDisplay);
  privateChatLayout->addLayout(privateInputLayout);

  QWidget *usersWidget = new QWidget();
  QVBoxLayout *usersLayout = new QVBoxLayout(usersWidget);
  usersLayout->setContentsMargins(0, 0, 0, 0);
  QLabel *usersLabel = new QLabel("在线用户");
  usersLabel->setObjectName("usersLabel");
  onlineUsersList = new QListWidget();
  onlineUsersList->setObjectName("onlineUsersList");
  onlineUsersList->setMaximumWidth(200);
  usersLayout->addWidget(usersLabel);
  usersLayout->addWidget(onlineUsersList);

  privateSplitter->addWidget(chatWidget);
  privateSplitter->addWidget(usersWidget);
  privateSplitter->setSizes({800, 200});
  privateTabLayout->addWidget(privateSplitter);
}

void PrivateChatTab::connectSignals() {
  connect(privateSendButton, &QPushButton::clicked, this,
          &PrivateChatTab::sendPrivateMessage);
  connect(privateMessageInput, &QLineEdit::returnPressed, this,
          &PrivateChatTab::sendPrivateMessage);
  connect(sendFileButton, &QPushButton::clicked, this,
          &PrivateChatTab::sendFile);
  connect(onlineUsersList, &QListWidget::itemClicked, this,
          &PrivateChatTab::handleUserSelected);
}

void PrivateChatTab::sendPrivateMessage() { // 这里处理选中的user
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
    appendMessage(nickname, content, timestamp);
    privateMessageInput->clear();
  } else {
    QMessageBox::warning(this, "错误", "请先选择一个在线用户");
  }
}

void PrivateChatTab::sendFile() {
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
  appendMessage(nickname, "[发送文件] " + QFileInfo(filePath).fileName(),
                timestamp);
  file.close();
}

void PrivateChatTab::handleUserSelected() {
  QListWidgetItem *item = onlineUsersList->currentItem();
  if (item) {
    selectedUser = item->data(Qt::UserRole).toString();
    qDebug() << "PrivateChatTab: Selected user id:" << selectedUser;
  }
}

void PrivateChatTab::appendMessage(const QString &sender,
                                   const QString &content,
                                   const QString &timestamp) {
  QVBoxLayout *layout =
      qobject_cast<QVBoxLayout *>(privateChatContainer->layout());
  if (!layout) {
    qDebug() << "PrivateChatTab: Invalid container layout";
    return;
  }

  if (layout->count() > 0) {
    QLayoutItem *item = layout->itemAt(layout->count() - 1);
    if (item->spacerItem()) {
      layout->removeItem(item);
      delete item;
    }
  }

  MessageBubble *bubble = new MessageBubble(
      "", sender, content, timestamp, sender == nickname, privateChatContainer);
  layout->addWidget(bubble);
  layout->addStretch();

  bool isAtBottom = privateChatDisplay->verticalScrollBar()->value() >=
                    privateChatDisplay->verticalScrollBar()->maximum() - 20;
  privateChatDisplay->viewport()->update();
  if (isAtBottom) {
    QTimer::singleShot(10, privateChatDisplay, [=]() {
      privateChatDisplay->verticalScrollBar()->setValue(
          privateChatDisplay->verticalScrollBar()->maximum());
    });
  }
}

void PrivateChatTab::handleFileReceived(const QString &sender,
                                        const QByteArray &fileContent,
                                        const QString &timestamp) {
  QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件");
  if (saveFilePath.isEmpty())
    return;
  QFuture<void> future = QtConcurrent::run([=]() {
    QFile file(saveFilePath);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(fileContent);
      file.close();
      QMetaObject::invokeMethod(this, [=]() {
        appendMessage(sender,
                      "[接收文件] " + QFileInfo(saveFilePath).fileName(),
                      timestamp);
      });
    }
  });
  Q_UNUSED(future);
}

void PrivateChatTab::updateOnlineUsers(const QJsonArray &users) {
  onlineUsersList->clear();
  for (const QJsonValue &user : users) {
    QJsonObject userObj = user.toObject();
    QString username = userObj["username"].toString();
    QString nickname = userObj["nickname"].toString();
    QString displayText = QString("%1 (%2)").arg(nickname).arg(username);
    QListWidgetItem *item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, username);
    onlineUsersList->addItem(item);
  }
}