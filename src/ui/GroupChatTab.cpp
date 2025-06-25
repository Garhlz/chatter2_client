#include "GroupChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>  // 用于获取用户输入
#include <QScrollBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QUuid>
#include <QMap>
#include "utils/User.h"
#include "utils/UserInfo.h"
#include "GlobalEventBus.h"
#include "utils/GroupTask.h"
#include "dialogs/UserSelectionDialog.h"

GroupChatTab::GroupChatTab(ChatClient* client, const QString& nickname, UserManager* userManager_,
                           QWidget* parent)
    : QWidget(parent), chatClient(client), nickname(nickname), userManager(userManager_)
{
    setupUi();
    connectSignals();
}

void GroupChatTab::setupUi()
{
    this->setObjectName("GroupChatTab");

    // 1.主布局: 整个 GroupChatTab 的垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);

    // 2. 核心内容区域的水平布局
    // 这个布局将包含：左侧的群组列表 | 右侧的聊天内容区 + 操作按钮区
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);  // 内部不设额外边距

    // --- 左侧：群组列表 ---
    groupList = new QListWidget();
    groupList->setObjectName("groupList");
    groupList->setMaximumWidth(200);  // 限制宽度
    groupList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    groupList->setSelectionMode(QAbstractItemView::SingleSelection);
    contentLayout->addWidget(groupList);  // 将群组列表添加到内容布局

    // --- 右侧：聊天内容区 + 操作按钮区 ---
    // 我们需要一个QWidget来承载这两部分，并让它们水平排列
    QWidget* chatAndButtonsWidget = new QWidget();

    QHBoxLayout* chatAndButtonsLayout = new QHBoxLayout(chatAndButtonsWidget);
    chatAndButtonsLayout->setContentsMargins(0, 0, 0, 0);

    // 聊天内容堆栈 (类似于PrivateChatTab的sessionStack)
    groupContentStack = new QStackedWidget();
    groupContentStack->setObjectName("groupContentStack");
    chatAndButtonsLayout->addWidget(groupContentStack);  // 放在左边，占据大部分空间

    // 右侧的按钮区域 (垂直布局)
    QWidget* buttonsPanel = new QWidget();
    QVBoxLayout* buttonsLayout = new QVBoxLayout(buttonsPanel);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(5);  // 按钮之间可以有小间距

    // 添加各个按钮
    // 创建群组的按钮
    createGroupButton = new QPushButton("新建群组", this);
    createGroupButton->setObjectName("newGroupButton");
    buttonsLayout->addWidget(createGroupButton);

    // 删除或者退出群组的按钮
    deleteGroupButton = new QPushButton("删除当前群组", this);
    deleteGroupButton->setObjectName("deleteGroupButton");
    buttonsLayout->addWidget(deleteGroupButton);

    // 添加成员
    addMemberButton = new QPushButton("添加成员", this);
    addMemberButton->setObjectName("addMemberButton");
    buttonsLayout->addWidget(addMemberButton);

    // 移除成员, 先在客户端检测权限
    removeMemberButton = new QPushButton("移除成员", this);
    removeMemberButton->setObjectName("removeMemberButton");
    buttonsLayout->addWidget(removeMemberButton);

    buttonsLayout->addStretch();  // 只有一个拉伸器，确保按钮堆在上方

    chatAndButtonsLayout->addWidget(buttonsPanel);  // 将按钮面板添加到 chatAndButtonsLayout 的右侧

    // 将 chatAndButtonsWidget 添加到主内容布局的右侧
    contentLayout->addWidget(chatAndButtonsWidget);

    // 将内容布局添加到主布局
    mainLayout->addLayout(contentLayout);
}

void GroupChatTab::connectSignals()
{
    // Connect button signals to their respective handler slots
    connect(createGroupButton, &QPushButton::clicked, this,
            &GroupChatTab::on_createGroupButton_clicked);
    connect(deleteGroupButton, &QPushButton::clicked, this,
            &GroupChatTab::on_deleteGroupButton_clicked);
    connect(addMemberButton, &QPushButton::clicked, this,
            &GroupChatTab::on_addMemberButton_clicked);
    connect(removeMemberButton, &QPushButton::clicked, this,
            &GroupChatTab::on_removeMemberButton_clicked);

    // 还有会话点击的连接
    connect(groupList, &QListWidget::itemClicked, this, &GroupChatTab::on_groupList_itemClicked);

    connect(GlobalEventBus::instance(), &GlobalEventBus::appendGroupMessage, this,
            &GroupChatTab::appendMessage);

    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupInfo, this,
            &GroupChatTab::receiveGroupInfo);

    // 群组任务不同类型的响应, 需要在这里执行任务
    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupCreate, this,
            &GroupChatTab::on_receiveGroupCreateResponse);

    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupDelete, this,
            &GroupChatTab::on_receiveGroupDeleteResponse);

    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupAdd, this,
            &GroupChatTab::on_receiveGroupAddResponse);

    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupRemove, this,
            &GroupChatTab::on_receiveGroupRemoveResponse);

    // 新增
    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupBroadcastAdd, this,
            &GroupChatTab::on_receiveBroadcastAdd);

    connect(GlobalEventBus::instance(), &GlobalEventBus::sendGroupBroadcastRemove, this,
            &GroupChatTab::on_receiveBroadcastRemove);
}

void GroupChatTab::on_groupList_itemClicked(QListWidgetItem* item)
{
    if (!item) return;

    long groupId = item->data(Qt::UserRole).toLongLong();  // todo 不知道是否有问题

    GroupChatSession* session = sessionsMap.value(groupId, nullptr);

    if (session)
    {
        groupContentStack->setCurrentWidget(session);
        curGroupId = session->getGroupId();
        curGroupName = session->getGroupName();
        curGroupCreatorId = session->getCreatorId();

        session->scrollToBottom();
    }
}

// 只有初始化历史消息, 或者用户创建或者被加入了新的群组, 才会用这个.
// 只有这一个方法会创建并插入新的会话..
GroupChatSession* GroupChatTab::getOrCreateSession(long groupId, const QString& groupName,
                                                   long creatorId, const QJsonArray& members)
{
    // 其实get的时候直接从map中获取即可
    if (sessionsMap.contains(groupId))  // getSession
    {
        return sessionsMap[groupId];
    }
    // 群组信息直接存储在session的字段中即可
    // groupId groupName creatorId

    GroupChatSession* session = new GroupChatSession(groupId, groupName, creatorId, members, this);

    // fuck
    sessionsMap.insert(groupId, session);

    groupContentStack->addWidget(session);

    QString displayText = groupName;

    QListWidgetItem* item = new QListWidgetItem(displayText);

    item->setData(Qt::UserRole, static_cast<qint64>(groupId));
    groupList->addItem(item);
    groupList->setCurrentItem(item);

    groupContentStack->setCurrentWidget(session);
    // 直接在显示session的时候设置当前的变量
    curGroupId = session->getGroupId();
    curGroupName = session->getGroupName();
    curGroupCreatorId = session->getCreatorId();
    session->scrollToBottom();
    return session;
}

void GroupChatTab::appendMessage(const QString& senderUsername, const QString& senderNickname,
                                 long groupId, const QString& content, const QString& timestamp)
{
    // 这里直接获取群组
    auto session = getOrCreateSession(groupId, "", -1, QJsonArray());
    // 这里修改了签名
    session->appendMessage(senderUsername, senderNickname, content, timestamp);
}

/*
以下是收到的群组信息消息, 会在登录的时候收到, 此时需要加载
{
    "type": "GROUP_INFO",
    "content":
    {
      [
        {
          "groupId": 123,
          "groupName": "123",
          "creatorId":123,
          "createdAt": null,
          "members":[user1, user2, ...]
        },
        {
          "groupId": 123,
          "groupName": "123",
          "creatorId":123,
          "createdAt": null,
          "members":[user1, user2, ...]
        },
      ]
    }
}
*/
void GroupChatTab::receiveGroupInfo(const QJsonValue& content)
{
    QJsonArray groupInfos = content.toArray();
    for (const auto& info : groupInfos)
    {
        QJsonObject obj = info.toObject();
        long groupId = obj["groupId"].toInt();
        QString groupName = obj["groupName"].toString();
        long creatorId = obj["creatorId"].toInt();
        QJsonArray members = obj["members"].toArray();

        // 只是创建会话, 将数据放入会话类中
        getOrCreateSession(groupId, groupName, creatorId, members);
    }
}

QString GroupChatTab::generateTaskId()
{
    // 使用 QUuid::createUuid() 生成一个全局唯一的ID
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

/**
 * @brief 创建并初始化一个 GroupTask 对象
 * @param type 任务类型，例如 "CREATE_GROUP", "DELETE_GROUP", "ADD_MEMBER", "REMOVE_MEMBER"
 * @param groupId 相关联的群组ID
 * @param operatorId 执行此操作的用户ID (谁发起的)
 * @param groupName 相关联的群组名称 (用于创建群组时，或显示给用户)
 * @param userId 目标用户ID (当任务与单个用户有关时，例如添加/删除成员)
 * @return 指向新创建的 GroupTask 对象的指针。
 * 注意：此对象在堆上创建，其生命周期由接收者（如 NetworkRequestManager）管理。
 */
GroupTask* GroupChatTab::getGroupTask(const QString& type, long groupId, long operatorId,
                                      const QString& groupName, long userId)
{
    QString operationId = generateTaskId();  // 生成唯一任务 ID
    // 在堆上创建 GroupTask 对象，并将其父对象设置为 GroupChatTab，
    // 这样当 GroupChatTab 析构时，如果 GroupTask 还没有被处理，Qt 会自动删除它。
    // 但通常，这些任务会被 NetworkRequestManager 取走并处理其生命周期。
    GroupTask* task =
        new GroupTask(operationId, type, groupId, operatorId, groupName, userId, this);
    qDebug() << "Created GroupTask: " << task->toString();
    return task;
}

// --- 新建群组按钮点击事件处理 ---
void GroupChatTab::on_createGroupButton_clicked()
{
    bool ok;
    // 弹出输入框，获取用户输入的群组名称
    QString newGroupName = QInputDialog::getText(this, tr("新建群组"), tr("请输入群组名称:"),
                                                 QLineEdit::Normal, QString(), &ok);

    // 检查用户是否点击了“确定”且输入了群组名称
    if (ok && !newGroupName.isEmpty())
    {
        // 调用 getGroupTask 创建一个“创建群组”类型的任务
        // - type: "CREATE_GROUP" (创建群组)
        // - groupId: 0 (群组 ID 由后端生成，此处设为默认值)
        // - operatorId: m_currentUserId (当前操作的用户 ID)
        // - groupName: newGroupName (用户输入的群组名称)
        // - userId: 0 (创建群组通常不涉及特定的目标用户 ID，设为默认值)
        GroupTask* task =
            getGroupTask("GROUP_CREATE", 0, UserInfo::instance().userId(), newGroupName, 0);

        // 如果任务成功创建
        if (task)
        {
            GlobalEventBus::instance()->taskSubmitted(task);
            qDebug() << "已提交创建群组任务: " << task->toString();
        }
    }
    else if (ok)
    {
        // 用户点击了“确定”但输入为空，给出警告
        QMessageBox::warning(this, tr("警告"), tr("群组名称不能为空。"));
    }
}

// --- 删除群组按钮点击事件处理 ---
void GroupChatTab::on_deleteGroupButton_clicked()
{
    // 获取当前在群组列表 (groupList) 中选中的项
    QListWidgetItem* currentItem = groupList->currentItem();

    // 如果没有选中任何项，则给出警告并返回
    if (!currentItem)
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择群组。"));
        return;
    }

    // 从选中项的数据中获取群组 ID (之前通过 Qt::UserRole 存储)
    // long groupIdToDelete = currentItem->data(Qt::UserRole).toLongLong();
    long groupIdToDelete = curGroupId;

    GroupChatSession* session = sessionsMap.value(groupIdToDelete);

    // 获取选中项显示的群组名称
    QString groupNameToDelete = session->getGroupName();

    long creatorId = session->getCreatorId();

    long curUserId = UserInfo::instance().userId();

    // 弹出确认对话框. 如果是创建者就删除群组, 如果不是就是退出群组
    // todo修改这个按钮的显示逻辑
    if (QMessageBox::question(
            this, tr("确认删除"),
            (curUserId == creatorId ? QString("您确定要删除群组 '%1' (ID: %2) 吗？此操作不可撤销。")
                                          .arg(groupNameToDelete)
                                          .arg(groupIdToDelete)
                                    : QString("您确定要退出群组 '%1' (ID: %2) 吗？此操作不可撤销。")
                                          .arg(groupNameToDelete)
                                          .arg(groupIdToDelete)),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // 用户确认删除
        // 调用 getGroupTask 创建一个“删除群组”类型的任务
        // - type: "DELETE_GROUP" (删除群组)
        // - groupId: groupIdToDelete (要删除的群组 ID)
        // - operatorId: m_currentUserId (当前操作的用户 ID)
        // - groupName: groupNameToDelete (仅用于日志或用户提示)
        // - userId: 0 (删除群组不涉及特定的目标用户 ID)
        // todo 后端修改此处逻辑
        GroupTask* task =
            getGroupTask("GROUP_DELETE", groupIdToDelete, curUserId, groupNameToDelete, 0);

        if (task)
        {
            // 通过 GlobalEventBus 提交任务
            GlobalEventBus::instance()->taskSubmitted(task);
            qDebug() << "已提交删除群组任务: " << task->toString();
        }
    }
}

// --- 添加成员按钮点击事件处理 ---
void GroupChatTab::on_addMemberButton_clicked()
{
    // 获取当前在群组列表 (groupList) 中选中的项
    QListWidgetItem* currentItem = groupList->currentItem();

    // 如果没有选中任何群组，给出警告
    if (!currentItem)
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择一个群组来添加成员。"));
        return;
    }

    // 获取目标群组的 ID 和名称
    // long targetGroupId = currentItem->data(Qt::UserRole).toLongLong();
    long targetGroupId = curGroupId;
    // QString targetGroupName = currentItem->text();
    QString targetGroupName = curGroupName;

    // 获取所有用户
    QMap<long, User*> allUsers = userManager->getAllUsers();

    // 获取当前选中群组的成员 ID 列表
    GroupChatSession* session = sessionsMap.value(targetGroupId);
    QSet<long> currentGroupMemberIds;
    if (session)
    {
        QJsonArray members = session->getMembers();  //getMembers() 返回 QJsonArray
        for (const QJsonValue& memberValue : members)
        {
            if (memberValue.isObject())
            {
                QJsonObject memberObj = memberValue.toObject();
                long memberId = memberObj["userId"].toVariant().toLongLong();
                if (memberId != 0)
                {
                    currentGroupMemberIds.insert(memberId);
                }
            }
        }
    }
    else
    {
        qWarning() << "无法找到群组会话：" << targetGroupId << "来获取成员列表。";
        // 这里可以考虑给用户一个提示，或者从其他地方获取成员列表
        QMessageBox::warning(this, tr("错误"), tr("无法获取群组成员信息。"));
        return;
    }

    // 创建并显示用户选择对话框，用于添加成员（显示非群组成员）
    UserSelectionDialog dialog(allUsers, currentGroupMemberIds, true, this);  // true 表示添加模式
    if (dialog.exec() == QDialog::Accepted)  // 如果用户点击了“选择”
    {
        long memberIdToAdd = dialog.getSelectedUserId();

        // 检查用户是否选择了一个有效的成员 ID
        if (memberIdToAdd > 0)
        {
            // 检查要添加的成员是否是当前用户自己
            if (memberIdToAdd == UserInfo::instance().userId())
            {
                QMessageBox::warning(this, tr("警告"), tr("不能添加自己。"));
                return;
            }
            // 检查要添加的成员是否已经是群组成员
            if (currentGroupMemberIds.contains(memberIdToAdd))
            {
                QMessageBox::information(this, tr("提示"), tr("该用户已是群组成员。"));
                return;
            }

            // 调用 getGroupTask 创建一个“添加成员”类型的任务
            GroupTask* task =
                getGroupTask("GROUP_ADD", targetGroupId, UserInfo::instance().userId(),
                             targetGroupName, memberIdToAdd);

            if (task)
            {
                // 通过 GlobalEventBus 提交任务
                GlobalEventBus::instance()->taskSubmitted(task);
                qDebug() << "已提交添加成员任务: " << task->toString();
            }
        }
        else
        {
            // 用户点击了“选择”但没有选中任何用户（理论上不会发生，因为按钮会禁用）
            QMessageBox::warning(this, tr("警告"), tr("请选择一个有效的用户。"));
        }
    }
    // 如果 dialog.exec() != QDialog::Accepted，表示用户点击了取消，无需处理
}

// --- 移除成员按钮点击事件处理 ---
void GroupChatTab::on_removeMemberButton_clicked()
{
    // 获取当前在群组列表 (groupList) 中选中的项
    QListWidgetItem* currentItem = groupList->currentItem();

    // 如果没有选中任何群组，给出警告
    if (!currentItem)
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择一个群组来移除成员。"));
        return;
    }

    // 获取目标群组的 ID 和名称
    // long targetGroupId = currentItem->data(Qt::UserRole).toLongLong();
    long targetGroupId = curGroupId;
    // QString targetGroupName = currentItem->text();
    QString targetGroupName = curGroupName;

    // 获取所有用户 (用于获取昵称等信息，虽然移除时只关心 ID)
    QMap<long, User*> allUsers = userManager->getAllUsers();

    // 获取当前选中群组的成员 ID 列表
    GroupChatSession* session = sessionsMap.value(targetGroupId);
    QSet<long> currentGroupMemberIds;
    if (session)
    {
        QJsonArray members = session->getMembers();  // 假设 getMembers() 返回 QJsonArray
        for (const QJsonValue& memberValue : members)
        {
            if (memberValue.isObject())
            {
                QJsonObject memberObj = memberValue.toObject();

                long memberId = memberObj["userId"].toVariant().toLongLong();
                if (memberId != 0)
                {
                    currentGroupMemberIds.insert(memberId);
                }
            }
        }
    }
    else
    {
        qWarning() << "无法找到群组会话：" << targetGroupId << "来获取成员列表。";
        QMessageBox::warning(this, tr("错误"), tr("无法获取群组成员信息。"));
        return;
    }

    // 创建并显示用户选择对话框，用于移除成员（显示当前群组成员）
    UserSelectionDialog dialog(allUsers, currentGroupMemberIds, false, this);  // false 表示移除模式
    if (dialog.exec() == QDialog::Accepted)  // 如果用户点击了“选择”
    {
        long memberIdToRemove = dialog.getSelectedUserId();

        // 检查用户是否选择了一个有效的成员 ID
        if (memberIdToRemove > 0)
        {
            // 检查是否尝试移除自己
            if (memberIdToRemove == UserInfo::instance().userId())
            {
                QMessageBox::warning(this, tr("警告"), tr("不能移除自己。"));
                return;
            }
            // 检查该用户是否确实是群组成员 (尽管对话框已经过滤了)
            if (!currentGroupMemberIds.contains(memberIdToRemove))
            {
                QMessageBox::information(this, tr("提示"), tr("该用户不是群组成员。"));
                return;
            }

            // 调用 getGroupTask 创建一个“移除成员”类型的任务
            GroupTask* task =
                getGroupTask("GROUP_REMOVE", targetGroupId, UserInfo::instance().userId(),
                             targetGroupName, memberIdToRemove);

            if (task)
            {
                // 通过 GlobalEventBus 提交任务
                GlobalEventBus::instance()->taskSubmitted(task);
                qDebug() << "已提交移除成员任务: " << task->toString();
            }
        }
        else
        {
            // 用户点击了“选择”但没有选中任何用户
            QMessageBox::warning(this, tr("警告"), tr("请选择一个有效的用户。"));
        }
    }
    // 如果 dialog.exec() != QDialog::Accepted，表示用户点击了取消，无需处理
}

void GroupChatTab::on_receiveGroupCreateResponse(long groupId, const QString& groupName,
                                                 long creatorId)
{
    getOrCreateSession(groupId, groupName, creatorId, QJsonArray());
}

void GroupChatTab::on_receiveGroupDeleteResponse(long groupId)
{
    // Check if the group exists in the sessions map
    if (!sessionsMap.contains(groupId))
    {
        qWarning() << "Received delete response for non-existent group ID:" << groupId;
        return;
    }

    // Get the session to access group details
    GroupChatSession* session = sessionsMap.value(groupId);
    QString groupName = session->getGroupName();

    // Remove the session from the sessions map
    sessionsMap.remove(groupId);

    // Remove the group from the group list (UI)
    for (int i = 0; i < groupList->count(); ++i)
    {
        QListWidgetItem* item = groupList->item(i);
        if (item->data(Qt::UserRole).toLongLong() == groupId)
        {
            delete groupList->takeItem(i);
            break;
        }
    }

    // Remove the session widget from the stacked widget
    groupContentStack->removeWidget(session);
    session->deleteLater();  // Schedule the session for deletion

    // If the deleted group was currently displayed, switch to another group or clear the view
    if (groupContentStack->currentWidget() == session)
    {
        if (groupContentStack->count() > 0)
        {
            groupContentStack->setCurrentIndex(0);
            if (groupList->count() > 0)
            {
                groupList->setCurrentRow(0);
            }
        }
        else
        {
            // No groups left, clear the selection
            groupList->clearSelection();
        }
    }

    // Notify the user
    QMessageBox::information(
        this, tr("群组删除"),
        tr("群组 '%1' (ID: %2) 已删除或您已退出。").arg(groupName).arg(groupId));

    qDebug() << "Group deleted: ID =" << groupId << ", Name =" << groupName;
}

void GroupChatTab::on_receiveGroupAddResponse(long userId, long groupId)
{
    // Check if the group exists
    if (!sessionsMap.contains(groupId))
    {
        qWarning() << "Received add member response for non-existent group ID:" << groupId;
        return;
    }

    // Get the group session and user details
    GroupChatSession* session = sessionsMap.value(groupId);
    QString groupName = session->getGroupName();
    User* user = userManager->getUserById(userId);
    QString username = user->getUsername();

    session->addMemberToList(user);

    // Notify the user
    QMessageBox::information(this, tr("成员添加"),
                             tr("用户 '%1' (ID: %2) 已添加到群组 '%3' (ID: %4)。")
                                 .arg(username)
                                 .arg(userId)
                                 .arg(groupName)
                                 .arg(groupId));

    qDebug() << "Member added: User ID =" << userId << "to Group ID =" << groupId;
}

void GroupChatTab::on_receiveGroupRemoveResponse(long userId, long groupId)
{
    // Check if the group exists
    if (!sessionsMap.contains(groupId))
    {
        qWarning() << "Received remove member response for non-existent group ID:" << groupId;
        return;
    }

    // Get the group session and user details
    GroupChatSession* session = sessionsMap.value(groupId);
    QString groupName = session->getGroupName();
    User* user = userManager->getUserById(userId);

    QString username = user->getUsername();

    session->removeMemberFromList(user->getUserId());

    // Notify the user
    QMessageBox::information(this, tr("成员移除"),
                             tr("用户 '%1' (ID: %2) 已从群组 '%3' (ID: %4) 中移除。")
                                 .arg(username)
                                 .arg(userId)
                                 .arg(groupName)
                                 .arg(groupId));

    qDebug() << "Member removed: User ID =" << userId << "from Group ID =" << groupId;
}

// 这个用户被添加到这个群聊中
void GroupChatTab::on_receiveBroadcastAdd(long groupId, const QString& groupName,long creatorId, const QJsonArray& members, const QJsonArray& history)
{
    // 还需要creatorId, members字段
    auto session = getOrCreateSession(groupId, groupName, creatorId, members);
    // 把自己添加进去即可
    long curUserId = UserInfo::instance().userId();
    session->addMemberToList(userManager->getUserById(curUserId));

    for(const auto& info : history)
    {
        // 现在这是一个后端的MessageDTO
        QJsonObject message = info.toObject();
        qDebug()<<"message: "<<message;

        long senderId = message["userId"].toVariant().toLongLong();
        User* user = userManager->getUserById(senderId);

        QString senderUsername = user->getUsername();
        QString senderNickname = user->getNickname();
        QString content = message["content"].toString();
        // 注意这里的名称是createdAt

        QString timestamp = message["timestamp"].toString();
        qDebug()<<senderNickname << " " << content << " time is: "<<timestamp;
        session->appendMessage(senderUsername, senderNickname, content, timestamp);
    }
}

void GroupChatTab::on_receiveBroadcastRemove(long groupId, const QString& groupName)
{
    // Check if the group exists in the sessions map
    if (!sessionsMap.contains(groupId))
    {
        qWarning() << "Received delete response for non-existent group ID:" << groupId;
        return;
    }

    // Get the session to access group details
    GroupChatSession* session = sessionsMap.value(groupId);

    // Remove the session from the sessions map
    sessionsMap.remove(groupId);

    // Remove the group from the group list (UI)
    for (int i = 0; i < groupList->count(); ++i)
    {
        QListWidgetItem* item = groupList->item(i);
        if (item->data(Qt::UserRole).toLongLong() == groupId)
        {
            delete groupList->takeItem(i);
            break;
        }
    }

    // Remove the session widget from the stacked widget
    groupContentStack->removeWidget(session);
    session->deleteLater();  // Schedule the session for deletion

    // If the deleted group was currently displayed, switch to another group or clear the view
    if (groupContentStack->currentWidget() == session)
    {
        if (groupContentStack->count() > 0)
        {
            groupContentStack->setCurrentIndex(0);
            if (groupList->count() > 0)
            {
                groupList->setCurrentRow(0);
            }
        }
        else
        {
            // No groups left, clear the selection
            groupList->clearSelection();
        }
    }

    // Notify the user
    QMessageBox::information(
        this, tr("群组删除"),
        tr("群组 '%1' (ID: %2) 已删除或您已退出。").arg(groupName).arg(groupId));

    qDebug() << "Group deleted: ID =" << groupId << ", Name =" << groupName;

}
