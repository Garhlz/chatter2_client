#ifndef MESSAGEBUBBLE_H
#define MESSAGEBUBBLE_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

class MessageBubble : public QWidget {
    Q_OBJECT

public:
    MessageBubble(const QString &avatar, const QString &nickname, 
                  const QString &content, const QString &timestamp,
                  bool isOwn, QWidget *parent = nullptr);
    ~MessageBubble();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *avatarLabel;
    QLabel *nicknameLabel;
    QLabel *contentLabel;
    QLabel *timeLabel;
    bool isOwnMessage;
};

#endif // MESSAGEBUBBLE_H