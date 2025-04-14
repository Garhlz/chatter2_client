#include "MessageBubble.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QDebug>

MessageBubble::MessageBubble(const QString &avatar, const QString &nickname,
                             const QString &content, const QString &timestamp,
                             bool isOwn, QWidget *parent)
    : QWidget(parent), isOwnMessage(isOwn) {
    try {
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        if (!mainLayout) throw std::runtime_error("Failed to create mainLayout");

        // 头像
        avatarLabel = new QLabel();
        if (!avatarLabel) throw std::runtime_error("Failed to create avatarLabel");
        QPixmap pixmap;
        if (!avatar.isEmpty()) {
            try {
                if (pixmap.load(avatar)) {
                    pixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                } else {
                    qDebug() << "MessageBubble: Failed to load avatar:" << avatar;
                    pixmap = QPixmap(":/default_avatar.png");
                    if (pixmap.isNull()) {
                        pixmap = QPixmap(40, 40);
                        pixmap.fill(Qt::gray);
                    }
                }
            } catch (...) {
                qDebug() << "MessageBubble: Exception while loading avatar:" << avatar;
                pixmap = QPixmap(40, 40);
                pixmap.fill(Qt::gray);
            }
        } else {
            pixmap = QPixmap(40, 40);
            pixmap.fill(Qt::gray);
        }
        avatarLabel->setPixmap(pixmap);
        avatarLabel->setFixedSize(40, 40);

        // 消息内容
        QVBoxLayout *contentLayout = new QVBoxLayout();
        if (!contentLayout) throw std::runtime_error("Failed to create contentLayout");
        nicknameLabel = new QLabel(nickname);
        if (!nicknameLabel) throw std::runtime_error("Failed to create nicknameLabel");
        nicknameLabel->setStyleSheet("font-weight: bold; color: #333;");

        contentLabel = new QLabel(content);
        if (!contentLabel) throw std::runtime_error("Failed to create contentLabel");
        contentLabel->setWordWrap(true);
        contentLabel->setStyleSheet(
            QString("background-color: %1; color: %2; padding: 10px; border-radius: 5px;")
            .arg(isOwnMessage ? "#DCF8C6" : "#E5E5EA")
            .arg(isOwnMessage ? "#000" : "#000")
        );

        timeLabel = new QLabel(timestamp);
        if (!timeLabel) throw std::runtime_error("Failed to create timeLabel");
        timeLabel->setStyleSheet("color: #888; font-size: 10px;");

        contentLayout->addWidget(nicknameLabel);
        contentLayout->addWidget(contentLabel);
        contentLayout->addWidget(timeLabel);

        // 根据消息方向调整布局
        if (isOwnMessage) {
            mainLayout->addStretch();
            mainLayout->addLayout(contentLayout);
            mainLayout->addWidget(avatarLabel);
        } else {
            mainLayout->addWidget(avatarLabel);
            mainLayout->addLayout(contentLayout);
            mainLayout->addStretch();
        }

        setLayout(mainLayout);
        qDebug() << "MessageBubble: Created for nickname:" << nickname;
    } catch (const std::exception &e) {
        qDebug() << "MessageBubble: Exception during construction:" << e.what();
        throw;
    } catch (...) {
        qDebug() << "MessageBubble: Unknown exception during construction";
        throw std::runtime_error("Unknown error in MessageBubble construction");
    }
}

MessageBubble::~MessageBubble() {
    qDebug() << "MessageBubble: Destructor called";
}

void MessageBubble::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QWidget::paintEvent(event);
}