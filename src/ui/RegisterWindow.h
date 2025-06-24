#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include "network/ChatClient.h"
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>

class RegisterWindow : public QMainWindow
{
    Q_OBJECT

   public:
    explicit RegisterWindow(ChatClient* client, QWidget* parent = nullptr);
    ~RegisterWindow();

   signals:
    void registerSuccessful();
    void showLoginWindow();

   private slots:
    void handleRegister();
    void handleRegisterSuccess();
    void handleError(const QString& error);
    void showLogin();

    // 新增, 用于使用异步机制处理socket连接..
    void onChatClientConnected(); // 监听 ChatClient 的 connected 信号
    void onChatClientError(const QString& error); // 监听 ChatClient 的 errorOccurred 信号 (业务错误)
    void onChatClientConnectionError(
        const QString& message);  // 监听 ChatClient 的 connectionError 信号 (连接错误)
    void setUiEnabled(bool enabled);
   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    void setupUi();
    void connectSignals();

    QString currentUsername;
    QString currentPassword;
    QString currentNickname;

    ChatClient* chatClient;
    QWidget* centralWidget;
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QLineEdit* nicknameEdit;
    QPushButton* registerButton;
    QPushButton* backToLoginButton;
    QLabel* statusLabel;
    bool isDragging;
    QPoint dragPosition;

    // 新增标志,标记当前是否有注册尝试正在进行
    bool m_isRegisterAttemptActive = false;
};

#endif  // REGISTERWINDOW_H