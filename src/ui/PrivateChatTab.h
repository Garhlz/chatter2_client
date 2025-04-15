#ifndef PRIVATECHATTAB_H
#define PRIVATECHATTAB_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

class PrivateChatTab : public QWidget {
  Q_OBJECT

public:
  explicit PrivateChatTab(ChatClient *client, const QString &nickname,
                          QWidget *parent = nullptr);
  void appendMessage(const QString &sender, const QString &content,
                     const QString &timestamp);
  void handleFileReceived(const QString &sender, const QByteArray &fileContent,
                          const QString &timestamp);
  void updateOnlineUsers(const QJsonArray &users);

private slots:
  void sendPrivateMessage();
  void sendFile();
  void handleUserSelected();

private:
  void setupUi();
  void connectSignals();

  ChatClient *chatClient;
  QString nickname;
  QScrollArea *privateChatDisplay;
  QWidget *privateChatContainer;
  QLineEdit *privateMessageInput;
  QPushButton *privateSendButton;
  QPushButton *sendFileButton;
  QListWidget *onlineUsersList;
  QString selectedUser;
};

#endif // PRIVATECHATTAB_H