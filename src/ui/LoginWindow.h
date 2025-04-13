#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "network/ChatClient.h"

class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LoginWindow(ChatClient *client, QWidget *parent = nullptr);
    ~LoginWindow();

signals:
    void loginSuccessful(const QString &nickname);
    void showRegisterWindow();

private slots:
    void handleLogin();
    void handleLoginSuccess(const QString &nickname);
    void handleError(const QString &error);
    void showRegister();

private:
    void setupUi();
    void connectSignals();

    ChatClient *chatClient;
    QWidget *centralWidget;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QLabel *statusLabel;
};

#endif // LOGINWINDOW_H