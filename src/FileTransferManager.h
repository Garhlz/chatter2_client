#ifndef FILETRANSFERMANAGER_H
#define FILETRANSFERMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QMap>
#include <QUrl>
#include <QQueue>

// 单例类，管理文件上传和下载任务
class FileTransferManager : public QObject
{
    Q_OBJECT

   public:
    // 获取单例实例
    static FileTransferManager& instance();

    // 禁止拷贝和赋值
    FileTransferManager(const FileTransferManager&) = delete;
    FileTransferManager& operator=(const FileTransferManager&) = delete;

    // 上传文件
    void uploadFile(const QString& receiverUsername, const QString& filePath, const QUrl& uploadUrl,
                    const QString& token, const QString& taskId);

    // 下载文件
    void downloadFile(const QUrl& downloadUrl, const QString& savePath, const QString& token,
                      const QString& taskId);

    // 取消指定任务
    void cancelTask(const QString& taskId);

   signals:
    // 上传完成信号
    void uploadFinished(bool success, const QString& taskId, const QString& localFilePath,
                        const QByteArray& response);

    // 下载完成信号
    void downloadFinished(bool success, const QString& taskId, const QString& savedFilePath,
                          const QString& errorString);

    // 上传进度信号
    void uploadProgress(const QString& taskId, qint64 bytesSent, qint64 bytesTotal);

    // 下载进度信号
    void downloadProgress(const QString& taskId, qint64 bytesReceived, qint64 bytesTotal);

   private slots:
    void onUploadFinished();
    void onDownloadReadyRead();
    void onDownloadFinished(const QString& taskId);  // 似乎是由networkmanager发送的

   private:
    explicit FileTransferManager(QObject* parent = nullptr);
    void processNextTask();  // 处理队列中的下一个任务

    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QFile*> m_activeDownloads;
    QMap<QString, QNetworkReply*> m_taskMap;                    // 任务ID到Reply的映射
    QQueue<QPair<QString, std::function<void()>>> m_taskQueue;  // 任务队列
    int m_maxConcurrentTasks = 3;                               // 最大并发任务数
    int m_currentTasks = 0;                                     // 当前运行的任务数
};

#endif  // FILETRANSFERMANAGER_H
