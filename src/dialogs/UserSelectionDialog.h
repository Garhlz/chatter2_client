// dialogs/UserSelectionDialog.h
#ifndef USERSELECTIONDIALOG_H
#define USERSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>  // 添加QHBoxLayout
#include <QLabel>
#include <QSet>  // 用于高效的成员检查

#include "utils/User.h"  // 假设你的 User 类在这里

class UserSelectionDialog : public QDialog
{
    Q_OBJECT
   public:
    // availableUsers: 所有可用的用户 (QMap<long, User*>)
    // currentGroupMembers: 当前群组的成员 ID 集合 (QSet<long>)，用于过滤
    // addingMembers: true 表示添加成员模式 (显示非群组成员)，false 表示移除成员模式 (显示群组成员)
    explicit UserSelectionDialog(const QMap<long, User*>& availableUsers,
                                 const QSet<long>& currentGroupMembers, bool addingMembers,
                                 QWidget* parent = nullptr);

    // 获取选中的用户 ID
    long getSelectedUserId() const;

   private slots:
    // 处理列表选择变化，用于启用/禁用“选择”按钮
    void onSelectionChanged();

   private:
    QListWidget* userListWidget;
    QPushButton* selectButton;
    QPushButton* cancelButton;
    long m_selectedUserId;  // 存储最终选中的用户 ID
};

#endif  // USERSELECTIONDIALOG_H