#ifndef PRIVATECHATSESSION_H
#define PRIVATECHATSESSION_H

#include "MessageBubble.h"
#include "network/ChatClient.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QJsonObject>
#include <QMap>

// 私聊会话类，管理聊天UI和文件传输
class PrivateChatSession : public QWidget
{
    Q_OBJECT

   public:
    explicit PrivateChatSession(ChatClient* client, const QString& curUsername_,
                                const QString& curNickname_, const QString& targetUsername_,
                                const QString& targetNickname_, QWidget* parent = nullptr);
    void appendMessage(const QString& sender, const QString& receiver, const QJsonValue& content,
                       const QString& timestamp, bool isFile);

    QString getTargetUser() const { return targetUsername; }

   public slots:
    void scrollToBottom();  // 新增的公共槽
   signals:
    void sendMessageRequested(const QString& targetUser, const QString& content);

   private slots:
    void sendPrivateMessage();
    void sendFile();
    void onFileMessageClicked(const QString& fileUrl, const QString& suggestedFileName,
                              const QString& taskId);
    void handleFileReceived(const QString& sender, const QString& receiver,
                            const QJsonObject& fileInfo, qint64 messageId,
                            const QString& timestamp);
    void onUploadFinished(bool success, const QString& taskId, const QString& localFilePath,
                          const QByteArray& response);
    void onDownloadTaskFinished(bool success, const QString& taskId, const QString& savedFilePath,
                                const QString& errorString);
    void onUploadProgressUpdated(const QString& taskId, qint64 bytesSent, qint64 bytesTotal);
    void onDownloadProgressUpdated(const QString& taskId, qint64 bytesReceived, qint64 bytesTotal);
    void cancelFileTransfer(const QString& taskId);

   private:
    void setupUi();
    void connectSignals();
    QString formatFileSize(qint64 fileSize);
    QString generateTaskId(const QString& filePath, bool isUpload);

    ChatClient* chatClient;
    QString curUsername;
    QString curNickname;
    QString targetUsername;
    QString targetNickname;

    // 组件
    QScrollArea* privateChatDisplay;
    QWidget* privateChatContainer;
    QLineEdit* privateMessageInput;
    QPushButton* privateSendButton;
    QPushButton* sendFileButton;
    QString httpHost;
    quint16 httpPort;
    QMap<QString, MessageBubble*> fileMessageMap;
};

#endif  // PRIVATECHATSESSION_H