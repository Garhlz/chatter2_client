#ifndef GROUPCHATTAB_H
#define GROUPCHATTAB_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QComboBox>
#include <QJsonArray>  // 新增包含
#include <QJsonValue>  // 新增包含
#include <QLineEdit>
#include <QScrollArea>
#include <QWidget>

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>

#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include "GroupChatSession.h"
#include "utils/GroupTask.h"
#include "utils/UserManager.h"

class GroupChatTab : public QWidget
{
    Q_OBJECT

   public:
    explicit GroupChatTab(ChatClient* client, const QString& nickname, UserManager* userManager,
                          QWidget* parent = nullptr);

   public slots:
    // change
    void appendMessage(const QString& senderUsername, const QString& senderNickname, long groupId,
                       const QString& content, const QString& timestamp);

    void on_receiveGroupCreateResponse(long groupId, const QString& groupName, long creatorId);

    void on_receiveGroupDeleteResponse(long groupId);

    void on_receiveGroupAddResponse(long userId, long groupId);

    void on_receiveGroupRemoveResponse(long userId, long groupId);
    // 接受群组的信息, 也是初始化群组内容
    // 之前居然忘记连接总线和这个函数了
    void receiveGroupInfo(const QJsonValue& content);

   private slots:
    // 按钮的点击事件槽
    void on_createGroupButton_clicked();
    void on_deleteGroupButton_clicked();
    void on_addMemberButton_clicked();
    void on_removeMemberButton_clicked();
    // 会话列表选中
    void on_groupList_itemClicked(QListWidgetItem* item);

   private:
    void setupUi();
    void connectSignals();
    GroupChatSession* getOrCreateSession(long groupId, const QString& groupName, long creatorId,
                                         const QJsonArray& userArray);  // qmap也是用groupId标识
    QString generateTaskId();
    GroupTask* getGroupTask(const QString& type, long groupId, long operatorId,
                            const QString& m_groupName, long userId);

    ChatClient* chatClient;
    QString nickname;  // 这里表示当前用户的nickname, 虽然可以通过UserInfo获取, 但还是保留
    QScrollArea* groupChatDisplay;
    QWidget* groupChatContainer;
    QComboBox* groupCombo;
    QLineEdit* groupMessageInput;
    QPushButton* groupSendButton;
    // 分割线

    QListWidget* groupList;
    QStackedWidget* groupContentStack;

    QPushButton* createGroupButton;
    QPushButton* deleteGroupButton;  // 注意, 如果是群主, 用于删除, 如果不是, 用于退出
    QPushButton* addMemberButton;
    QPushButton* removeMemberButton;

    QMap<long, GroupChatSession*> sessionsMap;  // 用groupId标识

    UserManager* userManager;

    long curGroupId;

    QString curGroupName;

    long curGroupCreatorId;

   signals:
    void createGroupRequested(long creatorId, const QString& groupName, const QString& operationId);

    void deleteGroupRequested(long operatorId, long groupId, const QString& operationId);

    // 添加和移除一次都只能选中一个
    void addMemberToGroupRequested(long operatorId, long groupId, long userId,
                                   const QString& operationId);

    void removeMemberFromGroupRequested(long operatorId, long groupId, long userId,
                                        const QString& operationId);
};

#endif  // GROUPCHATTAB_H