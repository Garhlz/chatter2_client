#ifndef GROUPCHATSESSION_H
#define GROUPCHATSESSION_H

#include "MessageBubble.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QJsonObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "utils/User.h"

// 群组会话类
class GroupChatSession : public QWidget
{
    Q_OBJECT
   private:
    void setupUi();
    void connectSignals();
    QString formatFileSize(qint64 fileSize);
    QString generateTaskId(const QString& groupId, bool isUpload);

    long groupId;
    QString groupName;
    long creatorId;
    QJsonArray members;

    QScrollArea* groupChatDisplay;
    QWidget* groupChatContainer;
    QLineEdit* groupMessageInput;
    QPushButton* groupSendButton;
    // 想想如何管理任务id和任务之间的关系
    // 可以创建一个任务类
   private slots:
    void sendGroupMessage();

   public:
    GroupChatSession(long groupId_, const QString& groupName_, long creatorId_,
                     const QJsonArray& member, QWidget* parent = nullptr);

    void appendMessage(const QString& sender, const QJsonValue& content, const QString& timestamp);

    long getGroupId();

    QString getGroupName();

    long getCreatorId();

    QJsonArray getMembers();

    void addMemberToList(User* user);
    void removeMemberFromList(User* user);

   public slots:
    void scrollToBottom();
};

#endif