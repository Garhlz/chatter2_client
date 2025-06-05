#ifndef PRIVATECHATTAB_H
#define PRIVATECHATTAB_H

#include <QWidget>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>

class ChatClient;
class PrivateChatSession;

class PrivateChatTab : public QWidget
{
    Q_OBJECT
   public:
    explicit PrivateChatTab(ChatClient* client, const QString& username, const QString& nickname,
                            QWidget* parent = nullptr);

    void appendMessage(const QString& sender, const QString& receiver, const QString& content,
                       const QString& timestamp);
    void handleFileReceived(const QString& sender, const QString& receiver,
                            const QByteArray& fileContent, const QString& timestamp);
    void addToOnlineList(const QJsonObject& userObj);
    void removeFromOnlineList(const QJsonObject& userObj);
    void addToOfflineList(const QJsonObject& userObj);
    void removeFromOfflineList(const QJsonObject& userObj);
    void initOnlineUsers(const QJsonArray& users);
    void initOfflineUsers(const QJsonArray& users);
    void someoneChange(int status, const QJsonObject& cur_user);
    int getOnlineNumber();
    int getOfflineNumber();

   private:
    void setupUi();
    void connectSignals();
    PrivateChatSession* getOrCreateSession(const QString& targetUser);
    PrivateChatSession* getOrCreateSessionTwo(const QString& sender, const QString& receiver);

   private slots:
    void handleUserSelected(QListWidgetItem* item);
    void handleSessionSelected(QListWidgetItem* item);

   private:
    ChatClient* chatClient;
    QString curUsername;
    QString curNickname;
    QMap<QString, PrivateChatSession*> sessions;
    QMap<QString, QJsonObject> allUsers;
    QListWidget* onlineUsersList;
    QListWidget* offlineUsersList;
    QListWidget* sessionList;
    QStackedWidget* sessionStack;
    int onlineNumbers = 0;
    int offlineNumbers = 0;
};

#endif  // PRIVATECHATTAB_H