#ifndef GROUPCHATTAB_H
#define GROUPCHATTAB_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QComboBox>
#include <QJsonArray> // 新增包含
#include <QJsonValue> // 新增包含
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class GroupChatTab : public QWidget {
  Q_OBJECT

public:
  explicit GroupChatTab(ChatClient *client, const QString &nickname,
                        QWidget *parent = nullptr);
  void appendMessage(const QString &sender, const QString &content,
                     const QString &timestamp);
  void updateGroupList(const QJsonArray &groups);

private slots:
  void sendGroupMessage();

private:
  void setupUi();
  void connectSignals();

  ChatClient *chatClient;
  QString nickname;
  QScrollArea *groupChatDisplay;
  QWidget *groupChatContainer;
  QComboBox *groupCombo;
  QLineEdit *groupMessageInput;
  QPushButton *groupSendButton;
};

#endif // GROUPCHATTAB_H