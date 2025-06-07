#include "FileTransferManager.h"
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDebug>

FileTransferManager& FileTransferManager::instance()
{
    static FileTransferManager manager;
    return manager;
}

FileTransferManager::FileTransferManager(QObject* parent) : QObject(parent)
{
    // 初始化网络管理器
}

void FileTransferManager::uploadFile(const QString& receiverUsername, const QString& filePath,
                                     const QUrl& uploadUrl, const QString& token,
                                     const QString& taskId)
{
    // 如果当前任务数超过限制，加入队列
    if (m_currentTasks >= m_maxConcurrentTasks)
    {
        m_taskQueue.enqueue(
            {taskId, [=]() { uploadFile(receiverUsername, filePath, uploadUrl, token, taskId); }});
        qDebug() << "任务" << taskId << "已加入上传队列";
        return;
    }

    // 打开文件
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly))
    {
        qWarning() << "无法打开文件:" << filePath;
        delete file;
        emit uploadFinished(false, taskId, filePath, "无法打开文件");
        processNextTask();
        return;
    }

    // 创建 multipart/form-data 请求体
    QFileInfo fileInfo(*file);
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 添加文件部分
    QHttpPart filePart;
    QMimeDatabase db;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       QVariant(db.mimeTypeForFile(fileInfo).name()));
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant("form-data; name=\"file\"; filename=\"" + fileInfo.fileName() + "\""));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    // 添加 receiverUsername 字段
    QHttpPart receiverPart;
    receiverPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant("form-data; name=\"receiverUsername\""));
    receiverPart.setBody(receiverUsername.toUtf8());
    multiPart->append(receiverPart);

    // 配置网络请求
    QNetworkRequest request(uploadUrl);
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    }

    // 发送请求
    QNetworkReply* reply = m_networkManager.post(request, multiPart);
    multiPart->setParent(reply);
    reply->setParent(this);
    reply->setProperty("taskId", taskId);
    reply->setProperty("localFilePath", filePath);
    m_taskMap.insert(taskId, reply);
    m_currentTasks++;

    // 连接信号
    connect(reply, &QNetworkReply::uploadProgress, this, [this, taskId](qint64 sent, qint64 total)
            { emit uploadProgress(taskId, sent, total); });
    connect(reply, &QNetworkReply::finished, this, &FileTransferManager::onUploadFinished);
}

void FileTransferManager::downloadFile(const QUrl& downloadUrl, const QString& savePath,
                                       const QString& token, const QString& taskId)
{
    if (m_currentTasks >= m_maxConcurrentTasks)
    {
        m_taskQueue.enqueue(
            {taskId, [=]() { downloadFile(downloadUrl, savePath, token, taskId); }});
        qDebug() << "任务" << taskId << "已加入下载队列";
        return;
    }

    QFile* file = new QFile(savePath);
    if (!file->open(QIODevice::WriteOnly))
    {
        qWarning() << "无法创建文件:" << savePath;
        delete file;
        emit downloadFinished(false, taskId, savePath, "无法创建文件");
        processNextTask();
        return;
    }

    QNetworkRequest request(downloadUrl);
    // 确实设置了token
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    }

    QNetworkReply* reply = m_networkManager.get(request);
    reply->setParent(this);
    m_activeDownloads.insert(reply, file);
    m_taskMap.insert(taskId, reply);
    m_currentTasks++;

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, taskId](qint64 received, qint64 total)
            { emit downloadProgress(taskId, received, total); });
    connect(reply, &QNetworkReply::readyRead, this, &FileTransferManager::onDownloadReadyRead);
    // connect(reply, &QNetworkReply::finished, this, &FileTransferManager::onDownloadFinished);
    connect(reply, &QNetworkReply::finished, this,
            [this, taskId]()
            {
                this->onDownloadFinished(taskId);  // <--- 使用 Lambda 传递 taskId
            });
}

void FileTransferManager::cancelTask(const QString& taskId)
{
    if (m_taskMap.contains(taskId))
    {
        QNetworkReply* reply = m_taskMap.take(taskId);
        reply->abort();
        if (m_activeDownloads.contains(reply))
        {
            QFile* file = m_activeDownloads.take(reply);
            file->close();
            file->remove();
            file->deleteLater();
        }
        reply->deleteLater();
        m_currentTasks--;
        processNextTask();
        qDebug() << "任务" << taskId << "已取消";
    }
}

void FileTransferManager::onUploadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString taskId = reply->property("taskId").toString();
    QString localFilePath = reply->property("localFilePath").toString();
    QByteArray response = reply->readAll();

    if (reply->error() == QNetworkReply::NoError)
    {
        emit uploadFinished(true, taskId, localFilePath, response);
    }
    else
    {
        qWarning() << "上传失败:" << localFilePath << ":" << reply->errorString();
        emit uploadFinished(false, taskId, localFilePath, reply->errorString().toUtf8());
    }

    m_taskMap.remove(taskId);
    reply->deleteLater();
    m_currentTasks--;
    processNextTask();
}

void FileTransferManager::onDownloadReadyRead()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply && m_activeDownloads.contains(reply))
    {
        QFile* file = m_activeDownloads.value(reply);
        file->write(reply->readAll());
    }
}

void FileTransferManager::onDownloadFinished(const QString& taskId)  // <--- 槽函数签名更改
{
    // 根据 taskId 从 m_taskMap 中查找 reply
    QNetworkReply* reply = m_taskMap.value(taskId, nullptr);

    // 检查 reply 是否有效且是否仍在 m_activeDownloads 中
    // 如果 reply 为空，或它已经从 m_activeDownloads 中移除 (例如被 cancelTask 处理过)
    if (!reply || !m_activeDownloads.contains(reply))
    {
        qWarning() << "onDownloadFinished: Reply or active download not found for taskId:" << taskId
                   << ". Possibly already processed or cancelled.";
        // 即使没有找到，也需要确保任务计数正确减少，以允许下一个任务处理
        // 如果这里没有找到 reply，通常意味着它已经被 `cancelTask` 处理过了
        // 但为了健壮性，确保 m_taskMap 中没有残留
        if (m_taskMap.contains(taskId))
        {
            m_taskMap.remove(taskId);
            m_currentTasks--;
            processNextTask();
        }
        return;
    }

    QFile* file = m_activeDownloads.take(reply);  // 从活跃下载映射中移除文件
    QString filePath = file->fileName();

    // 调试信息：检查是否有错误
    qDebug() << "Download finished for taskId:" << taskId;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "下载失败:" << filePath << ":" << reply->errorString();
    }

    if (reply->error() == QNetworkReply::NoError)
    {
        file->close();                                      // 关闭文件
        emit downloadFinished(true, taskId, filePath, "");  // 成功，内容为空
    }
    else
    {
        file->close();   // 关闭文件
        file->remove();  // 删除部分下载的文件
        emit downloadFinished(false, taskId, filePath, reply->errorString());
    }

    m_taskMap.remove(taskId);  // 从任务映射中移除
    file->deleteLater();       // 延迟删除文件对象
    reply->deleteLater();      // 延迟删除 reply 对象
    m_currentTasks--;
    processNextTask();
}

void FileTransferManager::processNextTask()
{
    if (m_taskQueue.isEmpty() || m_currentTasks >= m_maxConcurrentTasks) return;

    auto task = m_taskQueue.dequeue();
    task.second();  // 执行队列中的下一个任务
}
