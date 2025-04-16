#ifndef PRIVATECHATTAB_H
#define PRIVATECHATTAB_H

#include "MessageBubble.h"
#include "PrivateChatSession.h"
#include "network/ChatClient.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
class PrivateChatTab : public QWidget
{
    Q_OBJECT

   public:
    explicit PrivateChatTab(ChatClient* client, const QString& curUsername,
                            const QString& curNickname, QWidget* parent = nullptr);
    void appendMessage(const QString& sender, const QString& receiver, const QString& content,
                       const QString& timestamp);
    void handleFileReceived(const QString& sender, const QString& receiver,
                            const QByteArray& fileContent, const QString& timestamp);
    void initOnlineUsers(const QJsonArray& users);
    void initOfflineUsers(const QJsonArray& users);
    void someoneChange(int status, const QJsonObject& cur_user);
    int getOnlineNumber();
    int getOfflineNumber();
    void addToOnlineList(const QJsonObject& userObj);
    void removeFromOnlineList(const QJsonObject& userObj);
    void addToOfflineList(const QJsonObject& userObj);
    void removeFromOfflineList(const QJsonObject& userObj);

   private slots:
    void handleUserSelected(QListWidgetItem* item);

   private:
    void setupUi();
    void connectSignals();
    PrivateChatSession* getOrCreateSession(const QString& targetUser);
    PrivateChatSession* getOrCreateSessionTwo(const QString& sender, const QString& receiver);

    ChatClient* chatClient;
    QString curUsername;
    QString curNickname;
    QTabWidget* chatSessions;
    QListWidget* onlineUsersList;
    QListWidget* offlineUsersList;
    QMap<QString, PrivateChatSession*> sessions;
    QMap<QString, QJsonObject> allUsers;
    // 这样声明之后已经算是初始化了
    QString selectedUser;
    int onlineNumbers;
    int offlineNumbers;
};

#endif  // PRIVATECHATTAB_H