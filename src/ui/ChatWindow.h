#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QVBoxLayout>


class ChatWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit ChatWindow(ChatClient *client, const QString &nickname,
                      QWidget *parent = nullptr);
  ~ChatWindow();

public slots:
  void handleMessageReceived(const QString &sender, const QString &content,
                             qint64 messageId);
  void handlePrivateMessageReceived(const QString &sender,
                                    const QString &content, qint64 messageId);
  void handleGroupMessageReceived(const QString &sender,
                                  const QString &groupName,
                                  const QString &content, qint64 messageId);
  void handleFileReceived(const QString &sender, const QByteArray &fileContent,
                          qint64 messageId);
  void handleOnlineUsersUpdated(const QJsonArray &users, int count);
  void handleGroupListReceived(const QJsonArray &groups);
  void handleHistoryMessagesReceived(const QJsonArray &messages);

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
  void appendMessageBubble(QWidget *container, const QString &sender,
                           const QString &content, const QString &timestamp,
                           const QString &avatar = QString());
  void updateOnlineUsersList(const QJsonArray &users);

  ChatClient *chatClient;
  QString nickname;
  QWidget *centralWidget;
  QTabWidget *chatTabs;
  QSet<qint64> displayedMessages;

  // 公共聊天页
  QWidget *publicChatTab;
  QScrollArea *publicChatDisplay;
  QWidget *publicChatContainer;
  QVBoxLayout *publicChatLayout;
  QLineEdit *publicMessageInput;
  QPushButton *publicSendButton;

  // 私聊页
  QWidget *privateChatTab;
  QScrollArea *privateChatDisplay;
  QWidget *privateChatContainer;
  QLineEdit *privateMessageInput;
  QPushButton *privateSendButton;
  QListWidget *onlineUsersList;
  QString selectedUser;

  // 群聊页
  QWidget *groupChatTab;
  QScrollArea *groupChatDisplay;
  QWidget *groupChatContainer;
  QVBoxLayout *groupChatLayout;
  QComboBox *groupCombo;
  QLineEdit *groupMessageInput;
  QPushButton *groupSendButton;

  // 文件传输
  QPushButton *sendFileButton;

  // 状态栏
  QLabel *statusLabel;
  QLabel *onlineCountLabel;

  // 初始化标志
  bool isInitialized;
};

#endif // CHATWINDOW_H