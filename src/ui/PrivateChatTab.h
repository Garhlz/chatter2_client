// ui/PrivateChatTab.h
#ifndef PRIVATECHATTAB_H
#define PRIVATECHATTAB_H

#include <QWidget>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>

#include "utils/UserManager.h"  // 包含UserManager和User类

class ChatClient;
class PrivateChatSession;

class PrivateChatTab : public QWidget
{
    Q_OBJECT
   public:
    explicit PrivateChatTab(ChatClient* client, const QString& username, const QString& nickname,
                            UserManager* userManager, QWidget* parent = nullptr);
    ~PrivateChatTab() override = default;

   public slots:
    void appendMessage(const QString& sender, const QString& receiver, const QJsonValue& content,
                       const QString& timestamp, bool isFile);

   private:
    void setupUi();
    void connectSignals();
    PrivateChatSession* getOrCreateSession(
        const QString& targetUsername);  // 保持targetUsername为字符串
    PrivateChatSession* getOrCreateSessionTwo(const QString& sender, const QString& receiver);

    // 辅助函数，用于根据UserManager的信号更新UI列表
    void refreshUserLists();  // 初始或大幅度变更时刷新

    // 新增/移除列表项，直接使用 User* 对象
    void addUserToListUI(User* user);
    void removeUserFromListUI(long userId, QListWidget* targetList);

   private slots:
    void handleUserSelected(QListWidgetItem* item);
    void handleSessionSelected(QListWidgetItem* item);

    // **新增：连接到UserManager信号的槽函数**
    void onUsersInitialized();             // 当UserManager初始化所有用户数据时
    void onUserStatusChanged(User* user);  // 某个用户状态发生变化
    void onUserAdded(User* user);          // 新用户被添加

   private:
    ChatClient* chatClient;
    QString curUsername;
    QString curNickname;
    QMap<QString, PrivateChatSession*> sessions;  // 键是用户名

    UserManager* userManager;  // 存储 UserManager 指针
    QListWidget* onlineUsersList;
    QListWidget* offlineUsersList;
    QListWidget* sessionList;
    QStackedWidget* sessionStack;
};

#endif  // PRIVATECHATTAB_H