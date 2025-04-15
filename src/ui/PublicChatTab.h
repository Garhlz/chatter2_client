#ifndef PUBLICCHATTAB_H
#define PUBLICCHATTAB_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class PublicChatTab : public QWidget {
  Q_OBJECT

public:
  explicit PublicChatTab(ChatClient *client, const QString &nickname,
                         QWidget *parent = nullptr);
  void appendMessage(const QString &sender, const QString &content,
                     const QString &timestamp); // 修正参数名

private slots:
  void sendMessage();

private:
  void setupUi();
  void connectSignals();

  ChatClient *chatClient;
  QString nickname;
  QScrollArea *publicChatDisplay;
  QWidget *publicChatContainer;
  QLineEdit *publicMessageInput;
  QPushButton *publicSendButton;
};

#endif // PUBLICCHATTAB_H