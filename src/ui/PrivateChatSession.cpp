#include "PrivateChatSession.h"
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

PrivateChatSession::PrivateChatSession(ChatClient* client, const QString& curUsername_,
                                       const QString& curNickname_, const QString& targetUsername_,
                                       const QString& targetNickname_, QWidget* parent)
    : QWidget(parent),
      chatClient(client),
      curUsername(curUsername_),
      curNickname(curNickname_),
      targetUsername(targetUsername_),
      targetNickname(targetNickname_),
      httpHost(ConfigManager::instance().httpHost()),
      httpPort(ConfigManager::instance().httpPort())
{
    setObjectName("PrivateChatSession_" + targetUsername);
    setupUi();
    connectSignals();
}

void PrivateChatSession::setupUi()
{
    // 初始化主布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    // 设置消息显示区域
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

    // 设置输入区域
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
    // connect(GlobalEventBus::instance(), &GlobalEventBus::globalAppendMessage,
    //         this, &PrivateChatSession::handleFileReceived);
    connect(&FileTransferManager::instance(), &FileTransferManager::uploadFinished, this,
            &PrivateChatSession::onUploadFinished);
    connect(&FileTransferManager::instance(), &FileTransferManager::downloadFinished, this,
            &PrivateChatSession::onDownloadTaskFinished);
    connect(&FileTransferManager::instance(), &FileTransferManager::uploadProgress, this,
            &PrivateChatSession::onUploadProgressUpdated);
    connect(&FileTransferManager::instance(), &FileTransferManager::downloadProgress, this,
            &PrivateChatSession::onDownloadProgressUpdated);
}

QString PrivateChatSession::generateTaskId(const QString& filePath, bool isUpload)
{
    // 生成唯一任务ID
    QString prefix = isUpload ? "upload_" : "download_";
    return prefix + QUuid::createUuid().toString(QUuid::WithoutBraces) + "_" +
           QFileInfo(filePath).fileName();
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
    emit sendMessageRequested(targetUsername, content);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    appendMessage(curUsername, targetUsername, content, timestamp, false);
    privateMessageInput->clear();
}

void PrivateChatSession::sendFile()
{
    // 选择文件
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
    if (filePath.isEmpty()) return;

    QJsonObject fileInfoObj;
    // 生成任务ID
    QString taskId = generateTaskId(filePath, true);

    fileInfoObj["taskId"] = taskId;

    // 创建文件消息气泡
    fileInfoObj["type"] = "file";
    fileInfoObj["fileName"] = QFileInfo(filePath).fileName();
    fileInfoObj["fileSize"] = QFileInfo(filePath).size();

    // 新增字段, 表示发送接受状态和传输状态
    fileInfoObj["isSender"] = true;
    fileInfoObj["haveTransmitted"] = false;

    // 还要表示本地的位置
    fileInfoObj["localFilePath"] = filePath;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");

    appendMessage(curUsername, targetUsername, fileInfoObj, timestamp, true);

    // 准备上传URL
    QUrl uploadUrl;
    uploadUrl.setScheme("http");
    uploadUrl.setHost(httpHost);
    uploadUrl.setPort(httpPort);
    uploadUrl.setPath("/api/files/upload");

    // 发起上传
    FileTransferManager::instance().uploadFile(targetUsername, filePath, uploadUrl,
                                               UserInfo::instance().token(), taskId);
}

void PrivateChatSession::onFileMessageClicked(const QString& fileUrl,
                                              const QString& suggestedFileName,
                                              const QString& taskId)
{
    // 选择保存路径
    // qDebug() << "come here onFileMessageClicked";
    QString savePath = QFileDialog::getSaveFileName(
        this, "保存文件",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" +
            suggestedFileName,
        "All Files (*.*)");
    if (savePath.isEmpty()) return;

    MessageBubble* bubble = fileMessageMap.value(taskId);
    bubble->setLocalFilePath(savePath);
    bubble->setEnabled(false);
    // 下载过程中不可以修改外观
    // 发起下载
    FileTransferManager::instance().downloadFile(QUrl(fileUrl), savePath,
                                                 UserInfo::instance().token(), taskId);
}

// 上传任务结束
void PrivateChatSession::onUploadFinished(bool success, const QString& taskId,
                                          const QString& localFilePath, const QByteArray& response)
{
    MessageBubble* bubble = fileMessageMap.value(taskId);
    if (!bubble) return;

    if (!success)
    {
        bubble->updateStatus("发送失败");
        QMessageBox::warning(this, "文件上传失败", "错误: " + QString::fromUtf8(response));
        fileMessageMap.remove(taskId);
        return;
    }

    // 解析服务器响应
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        bubble->updateStatus("发送失败");
        QMessageBox::warning(this, "错误", "服务器响应解析失败: " + parseError.errorString());
        fileMessageMap.remove(taskId);
        return;
    }

    QJsonObject fileInfoObj = doc.object();
    bubble->setEnabled(true);
    // 这里启用bubble的交互
    bubble->setTranmittingStatus(true);
    bubble->updateStatus("已发送");
    bubble->updateFileInfo(fileInfoObj);

    fileMessageMap.remove(taskId);
}

void PrivateChatSession::onDownloadTaskFinished(bool success, const QString& taskId,
                                                const QString& savedFilePath,
                                                const QString& errorString)
{
    // 可能是因为没有这个taskId?
    MessageBubble* bubble = fileMessageMap.value(taskId);
    if (!bubble) return;
    if (success)
    {
        bubble->updateStatus("已下载");
        bubble->setEnabled(true);
        // 这里启用bubble的交互
        bubble->setTranmittingStatus(true);
        QMessageBox::information(this, "下载完成",
                                 QString("文件已保存到：\n%1").arg(savedFilePath));
    }
    else
    {
        bubble->updateStatus("下载失败");
        QMessageBox::warning(this, "下载失败", QString("下载文件时出错：\n%1").arg(errorString));
    }
    fileMessageMap.remove(taskId);
}

void PrivateChatSession::onUploadProgressUpdated(const QString& taskId, qint64 bytesSent,
                                                 qint64 bytesTotal)
{
    MessageBubble* bubble = fileMessageMap.value(taskId);
    if (bubble)
    {
        bubble->updateProgress(bytesSent, bytesTotal);
    }
}

void PrivateChatSession::onDownloadProgressUpdated(const QString& taskId, qint64 bytesReceived,
                                                   qint64 bytesTotal)
{
    MessageBubble* bubble = fileMessageMap.value(taskId);
    if (bubble)
    {
        bubble->updateProgress(bytesReceived, bytesTotal);
    }
}

void PrivateChatSession::cancelFileTransfer(const QString& taskId)
{
    FileTransferManager::instance().cancelTask(taskId);
    MessageBubble* bubble = fileMessageMap.value(taskId);
    if (bubble)
    {
        bubble->updateStatus("已取消");
        fileMessageMap.remove(taskId);
    }
}

void PrivateChatSession::handleFileReceived(const QString& sender, const QString& receiver,
                                            const QJsonObject& fileInfo, qint64 messageId,
                                            const QString& timestamp)
{
    // 当前的fileInfo只包含返回体中的content
    // 问题是当前是const, 不可以修改, 需要创建可修改的副本
    QJsonObject modifiedFileInfo = fileInfo;  // 创建副本
    QString fileUrl = modifiedFileInfo["fileUrl"].toString();

    if (sender == curUsername)
    {  // 当前用户是发送者, 判定为已经上传, 来自服务器存储的历史记录
        modifiedFileInfo["isSender"] = true;
        modifiedFileInfo["haveTransmitted"] = true;
    }
    else
    {  // 当前是接收者, 认为没有下载
        modifiedFileInfo["isSender"] = false;
        modifiedFileInfo["haveTransmitted"] = false;
    }

    modifiedFileInfo["taskId"] = generateTaskId(fileUrl, false);  // 这里使用的taskId将会在后续使用

    appendMessage(sender, receiver, modifiedFileInfo, timestamp, true);
}

// 注意content需要包含状态
// 这里删除了taskId, 包含在content中
void PrivateChatSession::appendMessage(const QString& sender, const QString& receiver,
                                       const QJsonValue& content, const QString& timestamp,
                                       bool isFile)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(privateChatContainer->layout());
    if (!layout)
    {
        qWarning() << "无效的容器布局:" << targetUsername;
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
    // qDebug() << "PrivateChatSession::appendMessage content: " << content;
    QString displaySender = (sender == curUsername) ? curNickname : targetNickname;

    QJsonObject modifiedFileInfo;

    QString taskId = "";

    MessageBubble* bubble;

    // handleFileReceived的逻辑迁移到这里了
    if (isFile && content.isObject())
    {
        modifiedFileInfo = content.toObject();
        // qDebug() << modifiedFileInfo;
        QString fileUrl = modifiedFileInfo["fileUrl"].toString();

        if (sender == curUsername)
        {  // 当前用户是发送者, 判定为已经上传, 来自服务器存储的历史记录
            modifiedFileInfo["isSender"] = true;
            modifiedFileInfo["haveTransmitted"] = true;
        }
        else
        {  // 当前是接收者, 认为没有下载
            modifiedFileInfo["isSender"] = false;
            modifiedFileInfo["haveTransmitted"] = false;
        }

        // 有两种可能, 可能在sendFile中设置了
        if (modifiedFileInfo["taskId"].toString().isEmpty())
        {
            modifiedFileInfo["taskId"] = generateTaskId(fileUrl, false);
        }

        QString taskId = modifiedFileInfo["taskId"].toString();

        modifiedFileInfo["type"] = "file";
        bubble = new MessageBubble("", displaySender, modifiedFileInfo, timestamp,
                                   sender == curUsername, isFile, privateChatContainer);
        connect(bubble, &MessageBubble::fileMessageClicked, this,
                &PrivateChatSession::onFileMessageClicked);

        if (!taskId.isEmpty())
        {
            // 这里直接加入即可
            fileMessageMap.insert(taskId, bubble);
        }
    }
    else
    {
        bubble = new MessageBubble("", displaySender, content, timestamp, sender == curUsername,
                                   isFile, privateChatContainer);
    }
    // 最傻的是这个bubble出错了
    // 这里把content的类型判断交给MessageBubble处理了
    // 这个content需要包含状态

    // bool isSender = obj["isSender"].toBool();
    // bool haveTransmitted = obj["haveTransmitted"].toBool();

    // if(isSender && !haveTransmitted){
    //     bubble->setEnabled(false); // 上传, 暂时关闭点击的选项
    // }

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

QString PrivateChatSession::formatFileSize(qint64 fileSize)
{
    if (fileSize < 1024) return QString("%1 B").arg(fileSize);
    if (fileSize < 1024 * 1024) return QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1);
    return QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);
}
