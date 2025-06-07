#include "GroupChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>

GroupChatTab::GroupChatTab(ChatClient* client, const QString& nickname, QWidget* parent)
    : QWidget(parent), chatClient(client), nickname(nickname)
{
    setupUi();
    connectSignals();
}

void GroupChatTab::setupUi()
{
    QVBoxLayout* groupLayout = new QVBoxLayout(this);
    groupLayout->setContentsMargins(8, 8, 8, 8);
    groupLayout->setSpacing(10);

    groupChatDisplay = new QScrollArea();
    groupChatDisplay->setObjectName("groupChatDisplay");
    groupChatDisplay->setMinimumHeight(400);
    groupChatDisplay->setMinimumWidth(600);
    groupChatContainer = new QWidget();
    groupChatContainer->setObjectName("groupChatContainer");
    QVBoxLayout* groupChatLayout = new QVBoxLayout(groupChatContainer);
    groupChatLayout->setAlignment(Qt::AlignTop);
    groupChatLayout->setContentsMargins(0, 0, 0, 0);
    groupChatDisplay->setWidget(groupChatContainer);
    groupChatDisplay->setWidgetResizable(true);

    QHBoxLayout* groupInputLayout = new QHBoxLayout();
    groupCombo = new QComboBox();
    groupCombo->setObjectName("groupCombo");
    groupCombo->setPlaceholderText("选择群组");
    groupMessageInput = new QLineEdit();
    groupMessageInput->setObjectName("groupMessageInput");
    groupMessageInput->setPlaceholderText("输入群聊消息...");
    groupSendButton = new QPushButton("发送");
    groupSendButton->setObjectName("groupSendButton");
    groupInputLayout->addWidget(groupCombo);
    groupInputLayout->addWidget(groupMessageInput);
    groupInputLayout->addWidget(groupSendButton);

    groupLayout->addWidget(groupChatDisplay);
    groupLayout->addLayout(groupInputLayout);
}

void GroupChatTab::connectSignals()
{
    connect(groupSendButton, &QPushButton::clicked, this, &GroupChatTab::sendGroupMessage);
    connect(groupMessageInput, &QLineEdit::returnPressed, this, &GroupChatTab::sendGroupMessage);
}

void GroupChatTab::sendGroupMessage()
{
    QString groupName = groupCombo->currentText().trimmed();
    QString content = groupMessageInput->text().trimmed();
    if (content.isEmpty() || groupName.isEmpty())
    {
        QMessageBox::warning(this, "错误", "群组和消息内容不能为空");
        return;
    }
    if (content.toUtf8().size() > 1000)
    {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }
    chatClient->sendGroupMessage(groupName, content);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    appendMessage(nickname, QString("[%1] %2").arg(groupName).arg(content), timestamp);
    groupMessageInput->clear();
}

void GroupChatTab::appendMessage(const QString& sender, const QString& content,
                                 const QString& timestamp)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(groupChatContainer->layout());
    if (!layout)
    {
        qDebug() << "GroupChatTab: Invalid container layout";
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

    MessageBubble* bubble = new MessageBubble("", sender, content, timestamp, sender == nickname,
                                              false, groupChatContainer);
    layout->addWidget(bubble);
    layout->addStretch();

    bool isAtBottom = groupChatDisplay->verticalScrollBar()->value() >=
                      groupChatDisplay->verticalScrollBar()->maximum() - 20;
    groupChatDisplay->viewport()->update();
    if (isAtBottom)
    {
        QTimer::singleShot(10, groupChatDisplay,
                           [=]()
                           {
                               groupChatDisplay->verticalScrollBar()->setValue(
                                   groupChatDisplay->verticalScrollBar()->maximum());
                           });
    }
}

void GroupChatTab::updateGroupList(const QJsonArray& groups)
{
    groupCombo->clear();
    for (const QJsonValue& g : groups)
    {
        groupCombo->addItem(g.toString());
    }
    if (!groups.isEmpty())
    {
        groupCombo->setCurrentIndex(0);
    }
}