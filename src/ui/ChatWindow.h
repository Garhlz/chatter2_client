#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFileDialog>
#include <QComboBox>
#include <QSet>
#include "network/ChatClient.h"

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChatWindow(ChatClient *client, const QString &nickname, QWidget *parent = nullptr);
    ~ChatWindow();

public slots:
    void handleMessageReceived(const QString &sender, const QString &content, qint64 messageId);
    void handlePrivateMessageReceived(const QString &sender, const QString &content, qint64 messageId);
    void handleGroupMessageReceived(const QString &sender, const QString &groupName, const QString &content, qint64 messageId);
    void handleFileReceived(const QString &sender, const QByteArray &fileContent, qint64 messageId);
    void handleOnlineUsersUpdated(const QJsonArray &users, int count);
    void handleGroupListReceived(const QJsonArray &groups);

private slots:
    void sendMessage();
    void sendPrivateMessage();
    void sendGroupMessage();
    void sendFile();
    void handleLogout();
    void handleError(const QString &error);
    void handleUserSelected();

private:
    void setupUi();
    void connectSignals();
    void appendMessage(QTextEdit *textEdit, const QString &sender, const QString &content, qint64 messageId);
    void updateOnlineUsersList(const QJsonArray &users);
    void loadHistory();
    void saveHistory(const QString &sender, const QString &content, const QString &type);

    ChatClient *chatClient;
    QString nickname;
    QWidget *centralWidget;
    QTabWidget *chatTabs;
    QSet<qint64> displayedMessages;

    // 公共聊天页
    QWidget *publicChatTab;
    QTextEdit *publicChatDisplay;
    QLineEdit *publicMessageInput;
    QPushButton *publicSendButton;

    // 私聊页
    QWidget *privateChatTab;
    QTextEdit *privateChatDisplay;
    QLineEdit *privateMessageInput;
    QPushButton *privateSendButton;
    QListWidget *onlineUsersList;
    QString selectedUser;

    // 群聊页
    QWidget *groupChatTab;
    QTextEdit *groupChatDisplay;
    QComboBox *groupCombo;
    QLineEdit *groupMessageInput;
    QPushButton *groupSendButton;

    // 文件传输
    QPushButton *sendFileButton;

    // 状态栏
    QLabel *statusLabel;
    QLabel *onlineCountLabel;
};

#endif // CHATWINDOW_H