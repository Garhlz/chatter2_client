#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include "network/ChatClient.h"
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>

class LoginWindow : public QMainWindow
{
    Q_OBJECT

   public:
    explicit LoginWindow(ChatClient* client, QWidget* parent = nullptr);
    ~LoginWindow();

   signals:
    void loginSuccessful(const QString& username, const QString& nickname);
    void showRegisterWindow();

   private slots:
    void handleLogin();
    void handleLoginSuccess(const QString& username, const QString& nickname);
    void handleError(const QString& error);
    void showRegister();

    // 新增, 用于使用异步机制处理socket连接..
    void onChatClientConnected(); // 监听 ChatClient 的 connected 信号
    void onChatClientError(const QString& error); // 监听 ChatClient 的 errorOccurred 信号 (业务错误)
    void onChatClientConnectionError(const QString& message); // 监听 ChatClient 的 connectionError 信号 (连接错误)


   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    void setupUi();
    void connectSignals();

    QString currentUsername;
    QString currentPassword;

    ChatClient* chatClient;
    QWidget* centralWidget;
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QPushButton* loginButton;
    QPushButton* registerButton;
    QLabel* statusLabel;
    bool isDragging;
    QPoint dragPosition;

    bool m_isLoginAttemptActive = false; // 新增：标记当前是否有登录尝试正在进行
};

#endif  // LOGINWINDOW_H