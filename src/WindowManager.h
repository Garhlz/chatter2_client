#pragma once

#include <QObject>
#include <QString>
#include <QScopedPointer>  // 推荐使用 QScopedPointer 或 QPointer 来管理 UI 对象的生命周期

class ChatClient;
class LoginWindow;
class RegisterWindow;
class ChatWindow;

class WindowManager : public QObject
{
    Q_OBJECT

   public:
    explicit WindowManager(ChatClient* chatClient, QObject* parent = nullptr);
    ~WindowManager();  // 析构函数负责清理管理的窗口

    // 启动应用程序的初始界面
    void startApplication();

   signals:
    // 可以添加一些信号来通知外部管理器（如果需要）当前窗口状态的变化
    // 例如：void applicationClosed();

   private slots:
    // 处理 LoginWindow 发出的信号
    void handleLoginSuccessful(const QString& username, const QString& nickname);
    void handleShowRegisterWindow();

    // 处理 RegisterWindow 发出的信号
    void handleRegistrationSuccessful();  // 注册成功后显示登录窗口
    void handleShowLoginWindow();

    // 处理 ChatWindow 发出的信号
    // void handleChatWindowClosed();  // 聊天窗口被用户关闭（例如点击X）
    void handleLogoutRequested();  // 用户在聊天窗口中点击了登出按钮

    // 处理 ChatClient 发出的连接状态信号
    void handleClientConnected();
    void handleClientDisconnected();
    void handleClientConnectionError(const QString& errorMessage);
    void handleClientReconnecting(int number);  // 客户端正在尝试重连

   private:
    ChatClient* m_chatClient;
    QScopedPointer<LoginWindow> m_loginWindow;
    QScopedPointer<RegisterWindow> m_registerWindow;
    QScopedPointer<ChatWindow> m_chatWindow;  // 注意：ChatWindow在登录成功后才创建

    // 私有辅助方法
    void showLoginScreen();
    void showRegisterScreen();
    void showChatScreen(const QString& username, const QString& nickname);
    void hideAllWindows();  // 隐藏所有窗口
    void displayConnectionStatus(const QString& message,
                                 bool isError = false);  // 显示连接状态给用户
};