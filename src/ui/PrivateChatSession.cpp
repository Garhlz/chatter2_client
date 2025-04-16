#include "PrivateChatSession.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>

PrivateChatSession::PrivateChatSession(ChatClient* client, const QString& curUsername_,
                                       const QString& curNickname_, const QString& targetUsername_,
                                       const QString& targetNickname_, QWidget* parent)
    : QWidget(parent),
      chatClient(client),
      curUsername(curUsername_),
      curNickname(curNickname_),
      targetUsername(targetUsername_),
      targetNickname(targetNickname_)
{
    setupUi();
    connectSignals();
}

void PrivateChatSession::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    privateChatDisplay = new QScrollArea();
    privateChatDisplay->setObjectName("privateChatDisplay_" + targetUsername);
    privateChatDisplay->setMinimumHeight(400);
    privateChatDisplay->setMinimumWidth(600);
    privateChatContainer = new QWidget();
    privateChatContainer->setObjectName("privateChatContainer_" + targetUsername);
    QVBoxLayout* messagesLayout = new QVBoxLayout(privateChatContainer);
    messagesLayout->setAlignment(Qt::AlignTop);
    messagesLayout->setContentsMargins(0, 0, 0, 0);
    privateChatDisplay->setWidget(privateChatContainer);
    privateChatDisplay->setWidgetResizable(true);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    privateMessageInput = new QLineEdit();
    privateMessageInput->setObjectName("privateMessageInput_" + targetUsername);
    privateMessageInput->setPlaceholderText("输入私聊消息...");
    privateSendButton = new QPushButton("发送");
    privateSendButton->setObjectName("privateSendButton_" + targetUsername);
    sendFileButton = new QPushButton("文件");
    sendFileButton->setObjectName("sendFileButton_" + targetUsername);
    inputLayout->addWidget(privateMessageInput);
    inputLayout->addWidget(privateSendButton);
    inputLayout->addWidget(sendFileButton);

    layout->addWidget(privateChatDisplay);
    layout->addLayout(inputLayout);
}

void PrivateChatSession::connectSignals()
{
    connect(privateSendButton, &QPushButton::clicked, this,
            &PrivateChatSession::sendPrivateMessage);
    connect(privateMessageInput, &QLineEdit::returnPressed, this,
            &PrivateChatSession::sendPrivateMessage);
    connect(sendFileButton, &QPushButton::clicked, this, &PrivateChatSession::sendFile);
}

void PrivateChatSession::sendPrivateMessage()
{
    QString content = privateMessageInput->text().trimmed();
    if (content.isEmpty()) return;
    if (content.toUtf8().size() > 1000)
    {
        QMessageBox::warning(this, "错误", "消息内容不能超过1000字节");
        return;
    }
    qDebug() << "Sending message to: " << targetUsername << ": " << content;
    emit sendMessageRequested(targetUsername, content);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    appendMessage(curUsername, targetUsername, content, timestamp);
    privateMessageInput->clear();
}

void PrivateChatSession::sendFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (filePath.isEmpty()) return;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }
    qint64 size = file.size();
    if (size > 10 * 1024 * 1024)
    {
        QMessageBox::warning(this, "错误", "文件不能超过10MB");
        file.close();
        return;
    }
    QByteArray fileContent = file.readAll();
    emit sendFileRequested(targetUsername, fileContent);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    appendMessage(curUsername, targetUsername, "[发送文件] " + QFileInfo(filePath).fileName(),
                  timestamp);
    file.close();
}

void PrivateChatSession::appendMessage(const QString& sender, const QString& receiver,
                                       const QString& content, const QString& timestamp)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(privateChatContainer->layout());
    if (!layout)
    {
        qDebug() << "PrivateChatSession: Invalid container layout for" << targetUsername;
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

    // 转换为显示用的 nickname
    QString displaySender = sender;
    if (sender == curUsername)
        displaySender = curNickname;
    else if (sender == targetUsername)
        displaySender = targetNickname;

    qDebug() << "Appending message: sender=" << sender << ", displaySender=" << displaySender
             << ", content=" << content;

    MessageBubble* bubble = new MessageBubble("", displaySender, content, timestamp,
                                              sender == curUsername, privateChatContainer);
    layout->addWidget(bubble);
    layout->addStretch();

    bool isAtBottom = privateChatDisplay->verticalScrollBar()->value() >=
                      privateChatDisplay->verticalScrollBar()->maximum() - 20;
    privateChatDisplay->viewport()->update();
    if (isAtBottom)
    {
        QTimer::singleShot(10, privateChatDisplay,
                           [=]()
                           {
                               privateChatDisplay->verticalScrollBar()->setValue(
                                   privateChatDisplay->verticalScrollBar()->maximum());
                           });
    }
}

void PrivateChatSession::handleFileReceived(const QString& sender, const QString& receiver,
                                            const QByteArray& fileContent, const QString& timestamp)
{
    QString saveFilePath = QFileDialog::getSaveFileName(this, "保存文件");
    if (saveFilePath.isEmpty()) return;
    QFuture<void> future = QtConcurrent::run(
        [=]()
        {
            QFile file(saveFilePath);
            if (file.open(QIODevice::WriteOnly))
            {
                file.write(fileContent);
                file.close();
                QMetaObject::invokeMethod(
                    this,
                    [=]()
                    {
                        appendMessage(sender, receiver,
                                      "[接收文件] " + QFileInfo(saveFilePath).fileName(),
                                      timestamp);
                    });
            }
            else
            {
                qDebug() << "Failed to save file:" << saveFilePath;
            }
        });
    Q_UNUSED(future);
}
