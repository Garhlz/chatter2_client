#include "ChatWindow.h"
#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QStatusBar>
#include <QGuiApplication>
#include <QScreen>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include <QFileDialog>

ChatWindow::ChatWindow(ChatClient *client, const QString &nickname, QWidget *parent)
    : QMainWindow(parent), chatClient(client), nickname(nickname) {
    setupUi();
    connectSignals();
    setWindowTitle("聊天客户端 - " + nickname);
    loadHistory();
    chatClient->requestGroupList();
}

ChatWindow::~ChatWindow() {}

void ChatWindow::setupUi() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    chatTabs = new QTabWidget(this);

    // 公共聊天页
    publicChatTab = new QWidget();
    QVBoxLayout *publicLayout = new QVBoxLayout(publicChatTab);
    publicChatDisplay = new QTextEdit();
    publicChatDisplay->setReadOnly(true);
    QHBoxLayout *publicInputLayout = new QHBoxLayout();
    publicMessageInput = new QLineEdit();
    publicSendButton = new QPushButton("发送");
    publicInputLayout->addWidget(publicMessageInput);
    publicInputLayout->addWidget(publicSendButton);
    publicLayout->addWidget(publicChatDisplay);
    publicLayout->addLayout(publicInputLayout);

    // 私聊页
    privateChatTab = new QWidget();
    QHBoxLayout *privateLayout = new QHBoxLayout(privateChatTab);
    QVBoxLayout *privateChatLayout = new QVBoxLayout();
    privateChatDisplay = new QTextEdit();
    privateChatDisplay->setReadOnly(true);
    QHBoxLayout *privateInputLayout = new QHBoxLayout();
    privateMessageInput = new QLineEdit();
    privateSendButton = new QPushButton("发送");
    sendFileButton = new QPushButton("发送文件");
    privateInputLayout->addWidget(privateMessageInput);
    privateInputLayout->addWidget(privateSendButton);
    privateInputLayout->addWidget(sendFileButton);
    privateChatLayout->addWidget(privateChatDisplay);
    privateChatLayout->addLayout(privateInputLayout);

    QVBoxLayout *usersLayout = new QVBoxLayout();
    QLabel *usersLabel = new QLabel("在线用户");
    onlineUsersList = new QListWidget();
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(onlineUsersList);

    privateLayout->addLayout(privateChatLayout);
    privateLayout->addLayout(usersLayout);

    // 群聊页
    groupChatTab = new QWidget();
    QVBoxLayout *groupLayout = new QVBoxLayout(groupChatTab);
    groupChatDisplay = new QTextEdit();
    groupChatDisplay->setReadOnly(true);
    QHBoxLayout *groupInputLayout = new QHBoxLayout();
    groupCombo = new QComboBox();
    groupCombo->setPlaceholderText("选择群组");
    groupMessageInput = new QLineEdit();
    groupMessageInput->setPlaceholderText("消息内容");
    groupSendButton = new QPushButton("发送");
    groupInputLayout->addWidget(groupCombo);
    groupInputLayout->addWidget(groupMessageInput);
    groupInputLayout->addWidget(groupSendButton);
    groupLayout->addWidget(groupChatDisplay);
    groupLayout->addLayout(groupInputLayout);

    chatTabs->addTab(publicChatTab, "公共聊天");
    chatTabs->addTab(privateChatTab, "私聊");
    chatTabs->addTab(groupChatTab, "群聊");

    QStatusBar *statusBar = new QStatusBar(this);
    statusLabel = new QLabel("已连接");
    onlineCountLabel = new QLabel("在线人数: 0");
    QPushButton *logoutButton = new QPushButton("登出");
    statusBar->addWidget(statusLabel);
    statusBar->addWidget(onlineCountLabel);
    statusBar->addPermanentWidget(logoutButton);
    setStatusBar(statusBar);

    mainLayout->addWidget(chatTabs);

    resize(800, 600);
    setStyleSheet(
        "QTextEdit { border: 1px solid #ccc; border-radius: 3px; }"
        "QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 3px; }"
        "QPushButton { padding: 8px; background-color: #4CAF50; color: white; "
        "border: none; border-radius: 3px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QListWidget { border: 1px solid #ccc; border-radius: 3px; }"
        "QLabel { color: #333; }"
    );

    // Qt 6 居中窗口
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        QSize windowSize = size();
        int x = (screenGeometry.width() - windowSize.width()) / 2;
        int y = (screenGeometry.height() - windowSize.height()) / 2;
        move(x, y);
    }
}

void ChatWindow::connectSignals() {
    connect(publicSendButton, &QPushButton::clicked, this, &ChatWindow::sendMessage);
    connect(privateSendButton, &QPushButton::clicked, this, &ChatWindow::sendPrivateMessage);
    connect(groupSendButton, &QPushButton::clicked, this, &ChatWindow::sendGroupMessage);
    connect(sendFileButton, &QPushButton::clicked, this, &ChatWindow::sendFile);

    connect(publicMessageInput, &QLineEdit::returnPressed, this, &ChatWindow::sendMessage);
    connect(privateMessageInput, &QLineEdit::returnPressed, this, &ChatWindow::sendPrivateMessage);
    connect(groupMessageInput, &QLineEdit::returnPressed, this, &ChatWindow::sendGroupMessage);

    connect(onlineUsersList, &QListWidget::itemClicked, this, &ChatWindow::handleUserSelected);

    connect(chatClient, &ChatClient::messageReceived, this, &ChatWindow::handleMessageReceived);
    connect(chatClient, &ChatClient::privateMessageReceived, this, &ChatWindow::handlePrivateMessageReceived);
    connect(chatClient, &ChatClient::groupMessageReceived, this, &ChatWindow::handleGroupMessageReceived);
    connect(chatClient, &ChatClient::fileReceived, this, &ChatWindow::handleFileReceived);
    connect(chatClient, &ChatClient::onlineUsersUpdated, this, &ChatWindow::handleOnlineUsersUpdated);
    connect(chatClient, &ChatClient::groupListReceived, this, &ChatWindow::handleGroupListReceived);
    connect(chatClient, &ChatClient::errorOccurred, this, &ChatWindow::handleError);

    connect(statusBar()->findChild<QPushButton*>(), &QPushButton::clicked,
            this, &ChatWindow::handleLogout);
}

void ChatWindow::sendMessage() {
    QString content = publicMessageInput->text().trimmed();
    if (content.isEmpty()) return;
    if (content.toUtf8().size() > 1000) {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }
    chatClient->sendMessage(content);
    appendMessage(publicChatDisplay, nickname, content, 0);
    saveHistory(nickname, content, "CHAT");
    publicMessageInput->clear();
}

void ChatWindow::sendPrivateMessage() {
    QString content = privateMessageInput->text().trimmed();
    if (content.isEmpty()) return;
    if (content.toUtf8().size() > 1000) {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }
    if (!selectedUser.isEmpty()) {
        chatClient->sendPrivateMessage(selectedUser, content);
        appendMessage(privateChatDisplay, nickname, content, 0);
        saveHistory(nickname, content, "PRIVATE_CHAT");
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
    appendMessage(groupChatDisplay, nickname, content, 0);
    saveHistory(nickname, content, "GROUP_CHAT");
    groupMessageInput->clear();
}

void ChatWindow::sendFile() {
    if (selectedUser.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择一个在线用户");
        return;
    }
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (filePath.isEmpty()) return;
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
    appendMessage(privateChatDisplay, nickname, "[发送文件] " + QFileInfo(filePath).fileName(), 0);
    saveHistory(nickname, "[发送文件] " + QFileInfo(filePath).fileName(), "FILE");
    file.close();
}

void ChatWindow::handleMessageReceived(const QString &sender, const QString &content, qint64 messageId) {
    appendMessage(publicChatDisplay, sender, content, messageId);
    saveHistory(sender, content, "CHAT");
}

void ChatWindow::handlePrivateMessageReceived(const QString &sender, const QString &content, qint64 messageId) {
    appendMessage(privateChatDisplay, sender, content, messageId);
    saveHistory(sender, content, "PRIVATE_CHAT");
    chatTabs->setCurrentWidget(privateChatTab);
}

void ChatWindow::handleGroupMessageReceived(const QString &sender, const QString &groupName,
                                           const QString &content, qint64 messageId) {
    QString message = QString("[%1] %2").arg(groupName).arg(content);
    appendMessage(groupChatDisplay, sender, message, messageId);
    saveHistory(sender, message, "GROUP_CHAT");
    chatTabs->setCurrentWidget(groupChatTab);
}

void ChatWindow::handleFileReceived(const QString &sender, const QByteArray &fileContent, qint64 messageId) {
    QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件");
    if (saveFilePath.isEmpty()) return;
    QFuture<void> future = QtConcurrent::run([=]() {
        QFile file(saveFilePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(fileContent);
            file.close();
            QMetaObject::invokeMethod(this, [=]() {
                appendMessage(privateChatDisplay, sender, "[接收文件] " + QFileInfo(saveFilePath).fileName(), messageId);
                saveHistory(sender, "[接收文件] " + QFileInfo(saveFilePath).fileName(), "FILE");
            });
        }
    });
    Q_UNUSED(future); // 显式忽略 QFuture，避免 nodiscard 警告
}

void ChatWindow::handleOnlineUsersUpdated(const QJsonArray &users, int count) {
    updateOnlineUsersList(users);
    onlineCountLabel->setText(QString("在线人数: %1").arg(count));
}

void ChatWindow::handleGroupListReceived(const QJsonArray &groups) {
    groupCombo->clear();
    for (const auto &g : groups) {
        groupCombo->addItem(g.toString());
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
    QListWidgetItem *item = onlineUsersList->currentItem();
    if (item) {
        selectedUser = item->data(Qt::UserRole).toString();
    }
}

void ChatWindow::appendMessage(QTextEdit *textEdit, const QString &sender,
                              const QString &content, qint64 messageId) {
    if (messageId > 0 && displayedMessages.contains(messageId)) return;
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString message = QString("[%1] %2: %3").arg(timestamp).arg(sender).arg(content);
    textEdit->append(message);
    if (messageId > 0) displayedMessages.insert(messageId);
    QScrollBar *scrollBar = textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatWindow::updateOnlineUsersList(const QJsonArray &users) {
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

void ChatWindow::loadHistory() {
    QFile file("chat_history.txt");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains("[CHAT]")) {
                publicChatDisplay->append(line);
            } else if (line.contains("[PRIVATE_CHAT]")) {
                privateChatDisplay->append(line);
            } else if (line.contains("[GROUP_CHAT]")) {
                groupChatDisplay->append(line);
            } else if (line.contains("[FILE]")) {
                privateChatDisplay->append(line);
            }
        }
        file.close();
    }
}

void ChatWindow::saveHistory(const QString &sender, const QString &content, const QString &type) {
    QFile file("chat_history.txt");
    if (file.open(QIODevice::Append)) {
        QTextStream out(&file);
        out << QString("[%1] [%2] %3: %4\n")
               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
               .arg(type)
               .arg(sender)
               .arg(content);
        file.close();
    }
}