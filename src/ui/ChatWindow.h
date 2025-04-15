#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "GroupChatTab.h"
#include "MessageBubble.h"
#include "PrivateChatTab.h"
#include "PublicChatTab.h"
#include "network/ChatClient.h"
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QMainWindow>
#include <QSet>
#include <QTabWidget>

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
  void handleLogout();
  void handleError(const QString &error); // 新增声明

private:
  void setupUi();
  void connectSignals();
  void appendMessageBubble(QWidget *container, const QString &sender,
                           const QString &content, const QString &timestamp,
                           const QString &avatar = QString());

  ChatClient *chatClient;
  QString nickname;
  QWidget *centralWidget;
  QTabWidget *chatTabs;
  QSet<qint64> displayedMessages;

  // Tabs
  PublicChatTab *publicChatTab;
  PrivateChatTab *privateChatTab;
  GroupChatTab *groupChatTab;

  // Status Bar
  QLabel *statusLabel;
  QLabel *onlineCountLabel;

  // Initialization Flag
  bool isInitialized;
};

#endif // CHATWINDOW_H