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
#include "utils/UserManager.h"

class ChatWindow : public QMainWindow
{
    Q_OBJECT

   public:
    explicit ChatWindow(ChatClient* client, QWidget* parent = nullptr);
    ~ChatWindow();

   public slots:
    void handleMessageReceived(const QString& sender, const QString& content, qint64 messageId);
    void handlePrivateMessageReceived(const QString& sender, const QString& reveicer,
                                      const QString& content, qint64 messageId);
    // 用户状态相关
    void handleOnlineUsersInit(const QJsonArray& users);
    void handleOfflineUsersInit(const QJsonArray& users);
    void updateUserCountsDisplay();

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
    bool m_initialOnlineLoaded = false;   // 新增：是否已加载初始在线列表
    bool m_initialOfflineLoaded = false;  // 新增：是否已加载初始离线列表

    UserManager* userManager;
};

#endif  // CHATWINDOW_H