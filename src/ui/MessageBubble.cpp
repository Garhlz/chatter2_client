#include "MessageBubble.h"
#include <QDebug>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>

MessageBubble::MessageBubble(const QString &avatar, const QString &nickname,
                             const QString &content, const QString timestamp,
                             bool isOwn, QWidget *parent)
    : QWidget(parent), isOwnMessage(isOwn) {
  try {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(8);

    if (!mainLayout)
      throw std::runtime_error("Failed to create mainLayout");

    // 头像
    avatarLabel = new QLabel();
    if (!avatarLabel)
      throw std::runtime_error("Failed to create avatarLabel");
    QPixmap pixmap;
    if (!avatar.isEmpty()) {
      try {
        if (pixmap.load(avatar)) {
          pixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
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
    avatarLabel->setObjectName("avatarLabel");

    // 消息内容布局
    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(2, 2, 2, 2);
    contentLayout->setSpacing(2);

    if (!contentLayout)
      throw std::runtime_error("Failed to create contentLayout");

    nicknameLabel = new QLabel(nickname);
    if (!nicknameLabel)
      throw std::runtime_error("Failed to create nicknameLabel");
    nicknameLabel->setObjectName("nicknameLabel");
    nicknameLabel->setProperty("own", isOwn ? "true" : "false");

    contentLabel = new QLabel(content);
    if (!contentLabel)
      throw std::runtime_error("Failed to create contentLabel");
    contentLabel->setObjectName("contentLabel");
    contentLabel->setProperty("own", isOwn ? "true" : "false");
    contentLabel->setWordWrap(true);

    // 动态宽度计算
    QFontMetrics fm(contentLabel->font());
    int parentWidth = parentWidget() ? parentWidget()->width() : 800;
    int maxWidth = qBound(250, static_cast<int>(parentWidth * 0.7), 800);
    QRect textRect = fm.boundingRect(QRect(0, 0, maxWidth - 20, 0),
                                     Qt::TextWordWrap | Qt::AlignLeft, content);
    int textWidth = textRect.width() + 20;
    contentLabel->setMaximumWidth(maxWidth);
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // 估算行数
    int lineCount = textRect.height() / fm.lineSpacing();
    // qDebug() << "MessageBubble: maxWidth=" << maxWidth
    //          << ", textWidth=" << textWidth << ", lineCount=" << lineCount
    //          << ", text=" << content.left(50);

    timeLabel = new QLabel(timestamp);
    if (!timeLabel)
      throw std::runtime_error("Failed to create timeLabel");
    timeLabel->setObjectName("timeLabel");
    timeLabel->setProperty("own", isOwn ? "true" : "false");

    contentLayout->addWidget(nicknameLabel);
    contentLayout->addWidget(contentLabel);
    contentLayout->addWidget(timeLabel);

    // 消息方向
    if (isOwnMessage) {
      mainLayout->addStretch(1);
      mainLayout->addLayout(contentLayout);
      mainLayout->addWidget(avatarLabel);
    } else {
      mainLayout->addWidget(avatarLabel);
      mainLayout->addLayout(contentLayout);
      mainLayout->addStretch(1);
    }

    setObjectName("MessageBubble");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
  } catch (const std::exception &e) {
    qDebug() << "MessageBubble: Exception during construction:" << e.what();
    throw;
  } catch (...) {
    qDebug() << "MessageBubble: Unknown exception during construction";
    throw std::runtime_error("Unknown error in MessageBubble construction");
  }
}

MessageBubble::~MessageBubble() {
  // qDebug() << "MessageBubble: Destructor called";
}

void MessageBubble::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QWidget::paintEvent(event);
}