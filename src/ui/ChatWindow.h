#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "GroupChatTab.h"
#include "MessageBubble.h"
#include "PrivateChatTab.h"
#include "PublicChatTab.h"
#include "network/ChatClient.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMainWindow>
#include <QSet>
#include <QTabWidget>

class ChatWindow : public QMainWindow
{
    Q_OBJECT

   public:
    explicit ChatWindow(ChatClient* client, const QString& username, const QString& nickname,
                        QWidget* parent = nullptr);
    ~ChatWindow();

   public slots:
    void handleMessageReceived(const QString& sender, const QString& content, qint64 messageId);
    void handlePrivateMessageReceived(const QString& sender, const QString& reveicer,
                                      const QString& content, qint64 messageId);
    void handleGroupMessageReceived(const QString& sender, const QString& groupName,
                                    const QString& content, qint64 messageId);
    void handleOnlineUsersInit(const QJsonArray& users);
    void handleOfflineUsersInit(const QJsonArray& users);
    // void handleUserAppend(const QJsonObject& user);
    // void handleUserRemove(const QJsonObject& user);
    void handleGroupListReceived(const QJsonArray& groups);
    void handleHistoryMessagesReceived(const QJsonArray& messages);
    void handleSomeoneLogin(const QJsonObject& loginUser);
    void handleSomeoneLogout(const QJsonObject& logoutUser);
   private slots:
    void handleLogout();
    void handleError(const QString& error);  // 新增声明

   private:
    void setupUi();
    void connectSignals();
    void appendMessageBubble(QWidget* container, const QString& sender, const QString& content,
                             const QString& timestamp, const QString& avatar = QString());
    int onlineNumbers;
    int offlineNumbers;

    ChatClient* chatClient;
    QString curUsername;
    QString nickname;
    QWidget* centralWidget;
    QTabWidget* chatTabs;
    QSet<qint64> displayedMessages;

    // Tabs
    PublicChatTab* publicChatTab;
    PrivateChatTab* privateChatTab;
    GroupChatTab* groupChatTab;

    // Status Bar
    QLabel* statusLabel;
    QLabel* onlineCountLabel;

    // Initialization Flag
    bool isInitialized;
};

#endif  // CHATWINDOW_H