#include "PublicChatTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QSizePolicy>

PublicChatTab::PublicChatTab(ChatClient* client, const QString& nickname, QWidget* parent)
    : QWidget(parent), chatClient(client), nickname(nickname)
{
    setupUi();
    connectSignals();
}

void PublicChatTab::setupUi()
{
    QVBoxLayout* publicLayout = new QVBoxLayout(this);
    publicLayout->setContentsMargins(8, 8, 8, 8);
    publicLayout->setSpacing(10);

    publicChatDisplay = new QScrollArea();
    publicChatDisplay->setObjectName("publicChatDisplay");
    publicChatDisplay->setWidgetResizable(true);
    publicChatDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    publicChatDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // 禁用水平滚动条

    publicChatContainer = new QWidget();
    publicChatContainer->setObjectName("publicChatContainer");
    QVBoxLayout* publicChatLayout = new QVBoxLayout(publicChatContainer);
    publicChatLayout->setAlignment(Qt::AlignTop);
    publicChatLayout->setContentsMargins(10, 10, 10, 10);
    publicChatLayout->setSpacing(5);

    // 移除 setSizePolicy，让容器自然扩展
    // publicChatContainer->setSizePolicy(QSizePolicy::Preferred,
    // QSizePolicy::Preferred);

    publicChatDisplay->setWidget(publicChatContainer);

    QHBoxLayout* publicInputLayout = new QHBoxLayout();
    publicMessageInput = new QLineEdit();
    publicMessageInput->setObjectName("publicMessageInput");
    publicMessageInput->setPlaceholderText("输入消息...");
    publicSendButton = new QPushButton("发送");
    publicSendButton->setObjectName("publicSendButton");
    publicInputLayout->addWidget(publicMessageInput);
    publicInputLayout->addWidget(publicSendButton);
    publicInputLayout->setStretch(0, 1);

    publicLayout->addWidget(publicChatDisplay);
    publicLayout->addLayout(publicInputLayout);
}

void PublicChatTab::connectSignals()
{
    connect(publicSendButton, &QPushButton::clicked, this, &PublicChatTab::sendMessage);
    connect(publicMessageInput, &QLineEdit::returnPressed, this, &PublicChatTab::sendMessage);
}

void PublicChatTab::sendMessage()
{
    QString content = publicMessageInput->text().trimmed();
    if (content.isEmpty()) return;
    if (content.toUtf8().size() > 1000)
    {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }
    chatClient->sendMessage(content);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    // 逻辑是在发送消息的时候, 直接添加到本地的聊天会话中
    appendMessage(nickname, content, timestamp);
    publicMessageInput->clear();
}

void PublicChatTab::appendMessage(const QString& sender, const QString& content,
                                  const QString& timestamp)
// 这里的sender一定是username而不是nickname
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(publicChatContainer->layout());
    if (!layout)
    {
        qDebug() << "PublicChatTab: Invalid container layout";
        return;
    }

    // 移除末尾的伸缩项（如果有）
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
                                              false, publicChatContainer);

    // 不设置 SizePolicy，让气泡自然适应内容
    layout->addWidget(bubble, 0, sender == nickname ? Qt::AlignRight : Qt::AlignLeft);

    // 只在末尾添加一个伸缩项，而不是每次都添加
    if (layout->count() == 0 || !layout->itemAt(layout->count() - 1)->spacerItem())
    {
        layout->addStretch();
    }

    // 强制刷新视口，确保布局更新
    publicChatDisplay->viewport()->update();
    // 使用 QTimer::singleShot 延迟执行，确保布局已经计算出新的 maximum() 值
    // 延迟设为0ms，表示尽快执行，通常已足够。如果偶尔出现滚动不到底的情况，可适当增加延迟 (如
    // 10ms)。
    QTimer::singleShot(0, publicChatDisplay,
                       [=]()
                       {
                           publicChatDisplay->verticalScrollBar()->setValue(
                               publicChatDisplay->verticalScrollBar()->maximum());
                       });
}