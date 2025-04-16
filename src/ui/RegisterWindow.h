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
    void handleRegisterSuccess(const QString& token);
    void handleError(const QString& error);
    void showLogin();

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    void setupUi();
    void connectSignals();

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
};

#endif  // REGISTERWINDOW_H