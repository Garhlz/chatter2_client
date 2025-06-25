#include "WindowManager.h"
#include "network/ChatClient.h"
#include "ui/LoginWindow.h"
#include "ui/RegisterWindow.h"
#include "ui/ChatWindow.h"
#include "utils/ConfigManager.h"
#include "utils/UserInfo.h"

#include <QDebug>
#include <QMessageBox>  // 用于显示关键错误或状态提示

WindowManager::WindowManager(ChatClient* chatClient, QObject* parent)
    : QObject(parent),
      m_chatClient(chatClient)
      // 使用 QScopedPointer 确保窗口在 WindowManager 生命周期结束时自动删除
      // 或者当 WindowManager 决定切换窗口时，旧窗口能被正确销毁
      ,
      m_loginWindow(new LoginWindow(m_chatClient)),
      m_registerWindow(new RegisterWindow(m_chatClient))
      // m_chatWindow 初始为 nullptr，只在登录成功时才创建
      ,
      m_chatWindow(nullptr)
{
    // 连接 LoginWindow 的信号
    connect(m_loginWindow.data(), &LoginWindow::showRegisterWindow, this,
            &WindowManager::handleShowRegisterWindow);
    connect(m_loginWindow.data(), &LoginWindow::loginSuccessful, this,
            &WindowManager::handleLoginSuccessful);

    // 连接 RegisterWindow 的信号
    connect(m_registerWindow.data(), &RegisterWindow::showLoginWindow, this,
            &WindowManager::handleShowLoginWindow);
    connect(m_registerWindow.data(), &RegisterWindow::registerSuccessful, this,
            &WindowManager::handleRegistrationSuccessful);  // 注册成功后也回到登录

    // 连接 ChatClient 的连接状态信号
    connect(m_chatClient, &ChatClient::connected, this, &WindowManager::handleClientConnected);
    connect(m_chatClient, &ChatClient::disconnected, this,
            &WindowManager::handleClientDisconnected);

    connect(m_chatClient, &ChatClient::connectionError, this,
            &WindowManager::handleClientConnectionError);
    connect(m_chatClient, &ChatClient::reconnecting, this,
            &WindowManager::handleClientReconnecting);

    // 注意：ChatWindow 的信号连接将在 handleLoginSuccessful 中动态进行，
    // 因为 ChatWindow 是动态创建的。
}

WindowManager::~WindowManager()
{
    // QScopedPointer 会自动处理其管理对象的删除，无需手动 delete
    qDebug() << "WindowManager destroyed.";
}

void WindowManager::startApplication()
{
    showLoginScreen();
    // 应用程序启动时，尝试连接服务器。
    // ChatClient 内部应包含首次连接失败后的重试逻辑。
    // ! change 不在这里创建连接
    // m_chatClient->connectToServer(ConfigManager::instance().tcpHost(),
    //                               ConfigManager::instance().tcpPort());
}

// --- 槽函数实现 ---

void WindowManager::handleLoginSuccessful(const QString& username, const QString& nickname)
{
    qDebug() << "WindowManager: Handling loginSuccessful for" << username;

    // 如果之前有旧的聊天窗口，先销毁它。
    // QScopedPointer 会自动处理旧对象的销毁。
    m_chatWindow.reset();  // 释放旧的 QScopedPointer 管理的对象，即销毁旧 ChatWindow

    // 创建新的 ChatWindow
    m_chatWindow.reset(new ChatWindow(m_chatClient));
    // 设置属性，当窗口关闭时自动删除其本身。与 QScopedPointer 结合使用需小心，
    // 这里 QScopedPointer 已经管理了生命周期，但对于 Qt 控件，WA_DeleteOnClose 也很常见。
    // 对于 QScopedPointer，最好的方式是当不再需要 m_chatWindow 时直接 reset()。
    // 鉴于 LoginWindow 和 RegisterWindow 是在构造函数中创建的，其生命周期由 WindowManager 管理。
    // ChatWindow 动态创建，也应由 WindowManager 的 QScopedPointer 管理其生命周期。
    // WA_DeleteOnClose 可能会导致 QScopedPointer 管理的指针失效，建议不使用。
    // 如果 ChatWindow 内部有主动关闭的逻辑，可以发送信号通知 WindowManager。

    // 连接 ChatWindow 的信号
    // connect(m_chatWindow.data(), &ChatWindow::windowClosed, this,
    //         &WindowManager::handleChatWindowClosed);
    connect(m_chatWindow.data(),
            &ChatWindow::logoutRequested,  // 假设 ChatWindow 有一个 logoutRequested 信号
            this, &WindowManager::handleLogoutRequested);

    showChatScreen(username, nickname);
}

void WindowManager::handleShowRegisterWindow()
{
    qDebug() << "WindowManager: Showing RegisterWindow.";
    hideAllWindows();
    showRegisterScreen();
}

void WindowManager::handleRegistrationSuccessful()
{
    qDebug() << "WindowManager: Registration successful, showing LoginWindow.";
    // 注册成功通常回到登录界面
    hideAllWindows();
    showLoginScreen();
    // QMessageBox::information(nullptr, "注册成功", "恭喜！您的账号已成功注册，请登录。");
}

void WindowManager::handleShowLoginWindow()
{
    qDebug() << "WindowManager: Showing LoginWindow.";
    hideAllWindows();
    showLoginScreen();
}

// void WindowManager::handleChatWindowClosed()
// {
//     qDebug() << "WindowManager: ChatWindow closed by user.";
//     // 当用户通过点击 X 关闭聊天窗口时，回到登录界面
//     // 同时可以考虑让 ChatClient 断开连接或进入离线状态
//     // m_chatWindow->deleteLater();
//     m_chatWindow.reset();  // 销毁 ChatWindow
//     if (m_chatClient->isConnected())
//     {
//         m_chatClient->disconnectFromServer();  // 主动断开TCP/WebSocket连接
//     }
//     showLoginScreen();
// }

void WindowManager::handleLogoutRequested()
{
    qDebug() << "WindowManager: Logout requested from ChatWindow.";
    hideAllWindows();
    // 还是改回来了
    m_chatClient->disconnectFromServer(true);  // 主动断开连接，服务器会处理在线状态

    m_chatWindow.reset();  // 销毁 ChatWindow
    showLoginScreen();
    // 可以在这里提示用户已成功登出
    // displayConnectionStatus("您已成功登出。", false);
}

void WindowManager::handleClientConnected()
{
    qDebug() << "WindowManager: ChatClient connected.";
    displayConnectionStatus("连接成功。", false);
    // 如果当前在登录界面，并且是连接成功，可以考虑是否需要刷新界面或按钮状态
    // 如果ChatClient内部有自动登录机制，可以在这里触发
}

void WindowManager::handleClientDisconnected()
{
    qDebug() << "WindowManager: ChatClient disconnected.";
    // 如果当前在聊天窗口，则显示断线提示，并尝试重连
    if (m_chatWindow && m_chatWindow->isVisible())
    {
        qDebug() << "void WindowManager::handleClientDisconnected()";
        displayConnectionStatus("网络连接已断开，正在尝试重连...", true);
        // 这里不直接跳转到登录界面，而是等待 ChatClient 的重连结果
    }
    else if(UserInfo::instance().online())
    {
        // 如果不在聊天界面（例如在登录/注册界面），而且在线...
        displayConnectionStatus("服务器连接已断开，请检查网络或稍后重试。", true);
        showLoginScreen();  // 确保回到登录界面
    }
}

void WindowManager::handleClientConnectionError(const QString& errorMessage)
{
    qDebug() << "WindowManager: ChatClient connection error:" << errorMessage;
    displayConnectionStatus("连接错误: " + errorMessage + " 请检查网络或稍后重试。", true);
    // 强制回到登录界面，因为连接失败了
    hideAllWindows();
    showLoginScreen();
}

void WindowManager::handleClientReconnecting(int number)
{
    qDebug() << "WindowManager: ChatClient attempting to reconnect...";
    // 如果当前在聊天窗口，显示重连提示
    if (m_chatWindow && m_chatWindow->isVisible())
    {
        displayConnectionStatus(QString("网络已断开，正在重连...第%1次尝试").arg(number), true);
    }
    else
    {
        // 如果在登录/注册界面，连接失败时也会触发重连，可以显示相应的提示
        displayConnectionStatus("无法连接服务器，正在尝试重连...", true);
    }
}

// --- 辅助方法实现 ---

void WindowManager::showLoginScreen()
{
    if (m_loginWindow)
    {
        m_loginWindow->show();
    }
}

void WindowManager::showRegisterScreen()
{
    if (m_registerWindow)
    {
        m_registerWindow->show();
    }
}

void WindowManager::showChatScreen(const QString& username, const QString& nickname)
{
    if (m_chatWindow)
    {
        hideAllWindows();  // 确保其他窗口隐藏
        m_chatWindow->show();
        // 设置聊天窗口的欢迎信息或用户昵称
        // m_chatWindow->setUserInfo(username, nickname); // 假设 ChatWindow 有这样的方法
    }
}

void WindowManager::hideAllWindows()
{
    if (m_loginWindow && m_loginWindow->isVisible())
    {
        m_loginWindow->hide();
    }
    if (m_registerWindow && m_registerWindow->isVisible())
    {
        m_registerWindow->hide();
    }
    if (m_chatWindow && m_chatWindow->isVisible())
    {
        m_chatWindow->hide();
    }
}

// todo 展示重连信息, 最好还要展示重试次数
void WindowManager::displayConnectionStatus(const QString& message, bool isError)
{
    // 这是一个简化的显示方式，你可以根据需要集成到更复杂的通知系统
    if (m_chatWindow && m_chatWindow->isVisible())
    {
        // 可以在聊天窗口的状态栏、顶部通知条或弹出非模态对话框显示
        // m_chatWindow->showStatusMessage(message, isError); // 假设 ChatWindow 有此方法
        qDebug() << "Connection Status (ChatWindow):" << message;
    }
    else
    {
        // 在登录/注册窗口可以通过 QMessageBox 或其他方式提示
        QMessageBox::information(nullptr, isError ? "连接错误" : "连接状态", message);
    }
}