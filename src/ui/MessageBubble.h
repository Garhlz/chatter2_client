#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QLabel>
#include <QWidget>
#include <QJsonObject>
#include <QProgressBar>

// 消息气泡类，显示文本或文件消息，支持文件传输进度条
class MessageBubble : public QWidget
{
    Q_OBJECT

   public:
    explicit MessageBubble(const QString& avatar, const QString& nickname,
                           const QJsonValue& content, const QString& timestamp, bool isOwn,
                           bool isFile, QWidget* parent = nullptr);
    ~MessageBubble();

    // 获取文件ID
    QString getFileId() const { return fileId; }

    // 更新文件传输进度
    void updateProgress(qint64 bytesProcessed, qint64 bytesTotal);

    // 更新文件传输状态
    void updateStatus(const QString& status);

    void updateFileInfo(const QJsonObject& FileInfo);

    void setTranmittingStatus(bool status);

    void setLocalFilePath(const QString& filePath);

   protected:
    void mousePressEvent(QMouseEvent* event) override;

   signals:
    void fileMessageClicked(const QString& fileUrl, const QString& fileName, const QString& taskId);

   private:
    QLabel* avatarLabel;                   // 头像标签
    QLabel* nicknameLabel;                 // 昵称标签
    QLabel* contentLabel;                  // 消息内容标签
    QLabel* timeLabel;                     // 时间标签
    QProgressBar* progressBar;             // 进度条
    QLabel* statusLabel;                   // 状态标签
    bool isOwnMessage;                     // 是否为自己的消息
    bool isFileMessage;                    // 是否为文件消息
    QString fileUrl;                       // 文件URL
    QString fileName;                      // 文件名
    QString fileId;                        // 文件ID
    QString formatFileSize(qint64 bytes);  // 格式化文件大小

    bool isSender;
    bool haveTransmitted;
    QString localFilePath;
    void openFileInExplorer(const QString& filePath);
    QString taskId;
};

#endif  // MESSAGEBUBBLE_H