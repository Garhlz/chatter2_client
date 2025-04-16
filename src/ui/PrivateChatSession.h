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

class PrivateChatSession : public QWidget
{
    Q_OBJECT

   public:
    explicit PrivateChatSession(ChatClient* client, const QString& curUsername_,
                                const QString& curNickname_, const QString& targetUsername_,
                                const QString& targetNickname_, QWidget* parent = nullptr);
    void appendMessage(const QString& sender, const QString& receiver, const QString& content,
                       const QString& timestamp);
    void handleFileReceived(const QString& sender, const QString& receiver,
                            const QByteArray& fileContent, const QString& timestamp);
    QString getTargetUser() const { return targetUsername; }

   signals:
    void sendMessageRequested(const QString& targetUser, const QString& content);
    void sendFileRequested(const QString& targetUser, const QByteArray& fileContent);

   private slots:
    void sendPrivateMessage();
    void sendFile();

   private:
    void setupUi();
    void connectSignals();

    ChatClient* chatClient;
    QString curUsername;
    QString curNickname;
    QString targetUsername;
    QString targetNickname;
    QScrollArea* privateChatDisplay;
    QWidget* privateChatContainer;
    QLineEdit* privateMessageInput;
    QPushButton* privateSendButton;
    QPushButton* sendFileButton;
};

#endif  // PRIVATECHATSESSION_H