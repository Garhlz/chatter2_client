#include "MessageBubble.h"
#include <QDebug>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QFile>
#include <QStyle>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

// 构造函数，初始化消息气泡
MessageBubble::MessageBubble(const QString& avatar, const QString& nickname,
                             const QJsonValue& content, const QString& timestamp, bool isOwn,
                             bool isFile, QWidget* parent)
    : QWidget(parent), isOwnMessage(isOwn), isFileMessage(false)  // isFileMessage 初始为 false
{
    // --- 统一初始化所有成员变量，确保它们不是野指针 ---
    // 这是关键改动，将QLabel和QProgressBar的new操作提到最前面
    avatarLabel = new QLabel(this);  // 传递this作为父对象，Qt会自动管理内存
    nicknameLabel = new QLabel(nickname, this);
    contentLabel = new QLabel(this);  // 内容标签统一在这里初始化
    timeLabel = new QLabel(timestamp, this);
    progressBar = new QProgressBar(this);  // 进度条统一在这里初始化
    statusLabel = new QLabel(this);        // 状态标签统一在这里初始化

    // 初始化主布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(8);
    setObjectName("MessageBubble");

    // 设置头像
    avatarLabel->setObjectName("avatarLabel");
    QPixmap pixmap;
    if (!avatar.isEmpty() && pixmap.load(avatar))
    {
        pixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        pixmap = QPixmap(40, 40);
        pixmap.fill(Qt::gray);  // 默认灰色头像
    }
    avatarLabel->setPixmap(pixmap);
    avatarLabel->setFixedSize(40, 40);

    // 消息内容布局
    QVBoxLayout* contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(2, 2, 2, 2);
    contentLayout->setSpacing(2);

    // 设置昵称
    nicknameLabel->setObjectName("nicknameLabel");
    nicknameLabel->setProperty("own", isOwn ? "true" : "false");

    // --- 处理文件消息 ---
    if (isFile && content.isObject())
    {
        QJsonObject obj = content.toObject();
        // 进一步判断是否确实是"file"类型，增强健壮性
        if (obj["type"].toString() == "file")
        {
            isFileMessage = true;  // 确认是文件消息

            fileUrl = obj["fileUrl"].toString();
            fileName = obj["fileName"].toString();

            isSender = obj["isSender"].toBool();
            haveTransmitted = obj["haveTransmitted"].toBool();
            localFilePath = obj["localFilePath"].toString();
            taskId = obj["taskId"].toString();

            // 移除这里的 `this->setEnabled(false)`，让鼠标事件统一在 `mousePressEvent` 中处理
            // 文件气泡通常需要保持 enabled，以便用户点击触发下载/打开等操作

            // 使用 toVariant().toLongLong() 获取文件大小，避免溢出
            // todo
            QString fileSizeStr = formatFileSize(obj["fileSize"].toInt());
            QString displayText = QString("📄 %1 (%2)").arg(fileName).arg(fileSizeStr);

            contentLabel->setText(displayText);         // 设置 contentLabel 的文本
            contentLabel->setProperty("file", "true");  // 标记为文件消息

            // 初始化进度条和状态标签的显示/隐藏和文本
            progressBar->setRange(0, 100);
            progressBar->setValue(0);  // 初始进度为0
            progressBar->setTextVisible(true);
            progressBar->setFixedHeight(18);

            if (haveTransmitted)
            {
                progressBar->setVisible(false);  // 已传输完成，隐藏进度条
                if (isSender)
                {
                    statusLabel->setText("已发送");
                }
                else
                {
                    statusLabel->setText("已下载");
                }
            }
            else
            {                                   // 未传输完成 (上传中/下载中/未下载)
                progressBar->setVisible(true);  // 显示进度条

                statusLabel->setText("等待中...");  // 发送方正在上传
            }
            statusLabel->setObjectName("statusLabel");
            statusLabel->setVisible(true);  // 状态标签应该总是可见，只是内容不同
        }
    }

    // --- 处理文本消息 ---
    // 只有在不是文件消息时才处理文本。
    if (!isFileMessage)
    {  // 如果上面 isFileMessage 被设置为 true，则此块不会执行
        contentLabel->setText(content.toString());  // 设置 contentLabel 的文本
    }

    // --- 设置内容标签通用属性 (现在 contentLabel 保证已被初始化) ---
    contentLabel->setObjectName("contentLabel");
    contentLabel->setProperty("own", isOwn ? "true" : "false");
    contentLabel->setWordWrap(true);
    QFontMetrics fm(contentLabel->font());
    // 获取父部件宽度，并设定最大宽度。考虑构造函数中父部件可能为空或未调整大小的情况
    int parentWidth = parentWidget() ? parentWidget()->width() : 800;  // 默认值 800
    int maxWidth = qBound(250, static_cast<int>(parentWidth * 0.7), 800);
    contentLabel->setMaximumWidth(maxWidth);

    // 确保可以接收鼠标事件
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);

    // 确保内容标签可以接收鼠标事件
    contentLabel->setAttribute(Qt::WA_Hover, true);
    contentLabel->setMouseTracking(true);

    // 设置时间标签 (已在开头初始化)
    timeLabel->setObjectName("timeLabel");
    timeLabel->setProperty("own", isOwn ? "true" : "false");

    // 组装布局
    contentLayout->addWidget(nicknameLabel);
    contentLayout->addWidget(contentLabel);
    if (isFileMessage)
    {  // 只有文件消息才添加进度条和状态标签
        contentLayout->addWidget(progressBar);
        contentLayout->addWidget(statusLabel);
    }
    contentLayout->addWidget(timeLabel);

    // 根据消息方向调整主布局
    if (isOwn)
    {
        mainLayout->addStretch();
        mainLayout->addLayout(contentLayout);
        mainLayout->addWidget(avatarLabel);
    }
    else
    {
        mainLayout->addWidget(avatarLabel);
        mainLayout->addLayout(contentLayout);
        mainLayout->addStretch();
    }

    // 最后调用 adjustSize() 确保气泡大小正确，通常由布局管理
    adjustSize();
}

MessageBubble::~MessageBubble() {}

void MessageBubble::mousePressEvent(QMouseEvent* event)
{
    // 将事件位置转换为 contentLabel 的局部坐标
    QPoint labelPos = contentLabel->mapFromParent(event->pos());
    if (isFileMessage && contentLabel->rect().contains(labelPos))
    {
        if (!isSender && !haveTransmitted)
        {  // 只有不是发送者且没有下载的时候才会开始下载
            qDebug() << "Emitting fileMessageClicked with URL:" << fileUrl;
            emit fileMessageClicked(fileUrl, fileName, taskId);  // 下载接口
            this->setEnabled(false);
            // 在这里关闭, 下载完成才会打开
        }
        else if (haveTransmitted)
        {  // 已经传输完成
            openFileInExplorer(localFilePath);
        }
    }
    else
    {
        qDebug() << "Click not on content label or not a file message";
    }
    QWidget::mousePressEvent(event);
}

void MessageBubble::updateProgress(qint64 bytesProcessed, qint64 bytesTotal)
{
    if (bytesTotal > 0)
    {
        int progress = static_cast<int>((bytesProcessed * 100) / bytesTotal);
        progressBar->setValue(progress);
        progressBar->setVisible(true);
        statusLabel->setText("传输中...");
        statusLabel->setVisible(true);
    }
}

void MessageBubble::updateStatus(const QString& status)
{
    statusLabel->setText(status);
    statusLabel->setVisible(true);
    progressBar->setVisible(false);
}

void MessageBubble::setTranmittingStatus(bool status)
{
    haveTransmitted = status;
}

void MessageBubble::setLocalFilePath(const QString& filePath)
{
    localFilePath = filePath;
}

void MessageBubble::updateFileInfo(const QJsonObject& fileInfo)
{
    if (!isFileMessage) return;
    // 更新逻辑
    haveTransmitted = true;

    QJsonObject content = fileInfo["content"].toObject();
    // 更新文件URL
    fileUrl = content["fileUrl"].toString();

    // 更新文件大小
    qint64 fileSize = content["fileSize"].toInt();
    QString fileSizeStr = formatFileSize(fileSize);

    // 更新显示文本
    QString displayText = QString("📄 %1 (%2)").arg(fileName).arg(fileSizeStr);
    contentLabel->setText(displayText);

    // 设置鼠标指针样式
    contentLabel->setCursor(Qt::PointingHandCursor);

    // 确保标签可以接收鼠标事件
    contentLabel->setAttribute(Qt::WA_Hover, true);
    contentLabel->setAttribute(Qt::WA_UnderMouse, true);

    // 强制更新样式
    contentLabel->style()->unpolish(contentLabel);
    contentLabel->style()->polish(contentLabel);
    contentLabel->update();

    qDebug() << "File info updated, URL:" << fileUrl;
}

QString MessageBubble::formatFileSize(qint64 bytes)
{
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

void MessageBubble::openFileInExplorer(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists())
    {
        QMessageBox::warning(this, "错误", "文件不存在: " + filePath);
        return;
    }

#ifdef Q_OS_WIN
    // Windows 系统使用 explorer /select 命令打开文件所在目录并选中文件
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(filePath);
    QProcess::startDetached("explorer.exe", args);
#elif defined(Q_OS_MAC)
    // macOS 系统使用 open -R 命令
    QStringList args;
    args << "-R" << filePath;
    QProcess::startDetached("open", args);
#else
    // Linux 系统使用 xdg-open 打开所在目录
    QProcess::startDetached("xdg-open", {fileInfo.absolutePath()});
#endif
}
