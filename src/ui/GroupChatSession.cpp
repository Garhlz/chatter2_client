#include "GroupChatSession.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QTimer>
#include <QJsonParseError>
#include "MessageBubble.h"
#include "utils/UserInfo.h"
#include "utils/ConfigManager.h"
#include "FileTransferManager.h"
#include "GlobalEventBus.h"
#include <QUuid>
#include "GroupChatSession.h"
#include "GlobalEventBus.h"
GroupChatSession::GroupChatSession(long groupId_, const QString& groupName_, long creatorId_,
                                   const QJsonArray& members_, QWidget* parent)
    : QWidget(parent),
      groupId(groupId_),
      groupName(groupName_),
      creatorId(creatorId_),
      members(members_)
{
    setObjectName("GroupChatSession" + groupId_);
    setupUi();
    connectSignals();
}

void GroupChatSession::setupUi()
{
    // 初始化主布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    // 设置消息显示区域
    groupChatDisplay = new QScrollArea();
    groupChatDisplay->setObjectName("groupChatDisplay_" + groupName);
    groupChatDisplay->setMinimumHeight(400);
    groupChatDisplay->setMinimumWidth(600);
    groupChatContainer = new QWidget();
    groupChatContainer->setObjectName("groupChatContainer_" + groupName);
    QVBoxLayout* messagesLayout = new QVBoxLayout(groupChatContainer);
    messagesLayout->setAlignment(Qt::AlignTop);
    messagesLayout->setContentsMargins(0, 0, 0, 0);
    groupChatDisplay->setWidget(groupChatContainer);
    groupChatDisplay->setWidgetResizable(true);

    // 设置输入区域
    QHBoxLayout* inputLayout = new QHBoxLayout();
    groupMessageInput = new QLineEdit();
    groupMessageInput->setObjectName("groupMessageInput_" + groupName);
    groupMessageInput->setPlaceholderText("输入群聊消息...");

    groupSendButton = new QPushButton("发送");
    groupSendButton->setObjectName("groupSendButton_" + groupName);

    inputLayout->addWidget(groupMessageInput);
    inputLayout->addWidget(groupSendButton);

    layout->addWidget(groupChatDisplay);
    layout->addLayout(inputLayout);
}

void GroupChatSession::connectSignals()
{
    // 发送消息相关
    connect(groupSendButton, &QPushButton::clicked, this, &GroupChatSession::sendGroupMessage);

    connect(groupMessageInput, &QLineEdit::returnPressed, this,
            &GroupChatSession::sendGroupMessage);
}

QString GroupChatSession::generateTaskId(const QString& filePath, bool isUpload)
{
    // 生成唯一任务ID
    QString prefix = isUpload ? "upload_" : "download_";
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces) + "_" +
           QFileInfo(filePath).fileName();
}

// 通过事件总线发送消息
void GroupChatSession::sendGroupMessage()
{
    QString content = groupMessageInput->text().trimmed();
    if (content.isEmpty()) return;
    if (content.toUtf8().size() > 1000)
    {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }

    GlobalEventBus::instance()->sendGroupMessage(groupId, content);

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    appendMessage(UserInfo::instance().username(), content, timestamp);

    groupMessageInput->clear();
}

// 注意content需要包含状态
// 这里删除了taskId, 包含在content中
void GroupChatSession::appendMessage(const QString& sender, const QJsonValue& content,
                                     const QString& timestamp)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(groupChatContainer->layout());
    if (!layout)
    {
        qWarning() << "无效的容器布局:" << groupId;
        return;
    }

    if (layout->count() > 0)
    {
        QLayoutItem* item = layout->itemAt(layout->count() - 1);
        if (item->spacerItem())
        {
            layout->removeItem(item);
            delete item;
        }
    }

    // isOwn由是否和当前用户相同决定
    MessageBubble* bubble =
        new MessageBubble("", sender, content, timestamp, sender == UserInfo::instance().username(),
                          false, groupChatContainer);

    layout->addWidget(bubble);

    layout->addStretch();

    bool isAtBottom = groupChatDisplay->verticalScrollBar()->value() >=
                      groupChatDisplay->verticalScrollBar()->maximum() - 20;

    groupChatDisplay->viewport()->update();
    QTimer::singleShot(0, groupChatDisplay,  // 延迟设为0，表示尽快执行
                       [=]()
                       {
                           groupChatDisplay->verticalScrollBar()->setValue(
                               groupChatDisplay->verticalScrollBar()->maximum());
                       });
}

void GroupChatSession::scrollToBottom()
{
    groupChatDisplay->viewport()->update();
    QTimer::singleShot(0, groupChatDisplay,
                       [=]()
                       {
                           groupChatDisplay->verticalScrollBar()->setValue(
                               groupChatDisplay->verticalScrollBar()->maximum());
                       });
}

long GroupChatSession::getGroupId()
{
    return this->groupId;
}

QString GroupChatSession::getGroupName()
{
    return this->groupName;
}

long GroupChatSession::getCreatorId()
{
    return this->creatorId;
}

QJsonArray GroupChatSession::getMembers()
{
    return this->members;
}

void GroupChatSession::addMemberToList(User* user)
{
    QJsonObject obj;
    obj["userId"] = static_cast<qint64>(user->getUserId());
    obj["username"] = user->getUsername();
    obj["nickname"] = user->getNickname();
    obj["avatarUrl"] = user->getAvatarUrl();
    obj["status"] = user->getStatus();
    members.append(obj);
}

void GroupChatSession::removeMemberFromList(User* user)
{
    long userId = user->getUserId();
    for (int i = 0; i < members.count(); i++)
    {
        long curId = members.at(i).toVariant().toLongLong();
        if (curId == userId)
        {
            members.removeAt(i);
            break;
        }
    }
}