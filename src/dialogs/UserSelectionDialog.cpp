// dialogs/UserSelectionDialog.cpp
#include "UserSelectionDialog.h"
#include <QDebug>
#include <QMessageBox>  // 添加QMessageBox

UserSelectionDialog::UserSelectionDialog(const QMap<long, User*>& availableUsers,
                                         const QSet<long>& currentGroupMembers, bool addingMembers,
                                         QWidget* parent)
    : QDialog(parent), m_selectedUserId(0)
{
    setWindowTitle(addingMembers ? tr("选择要添加的成员") : tr("选择要移除的成员"));
    setMinimumSize(300, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* infoLabel = new QLabel(
        addingMembers ? tr("请从列表中选择一个用户添加：") : tr("请从列表中选择一个用户移除："),
        this);
    mainLayout->addWidget(infoLabel);

    userListWidget = new QListWidget(this);
    userListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(userListWidget);

    // 根据是“添加”还是“移除”成员来填充列表
    for (User* user : availableUsers.values())
    {
        if (!user) continue;                   // 确保用户对象有效
        if (user->getUserId() == 0) continue;  // 跳过无效的 ID

        bool isMemberOfCurrentGroup = currentGroupMembers.contains(user->getUserId());

        if (addingMembers)
        {
            // 在添加成员模式下，只显示非当前群组成员
            if (!isMemberOfCurrentGroup)
            {
                QListWidgetItem* item = new QListWidgetItem(
                    QString("%1 (%2)").arg(user->getNickname()).arg(user->getUserId()),
                    userListWidget);
                item->setData(
                    Qt::UserRole,
                    static_cast<qint64>(user->getUserId()));  // 将用户 ID 存储在 Item 的数据中
                userListWidget->addItem(item);
            }
        }
        else
        {  // 移除成员模式
            // 在移除成员模式下，只显示当前群组成员
            if (isMemberOfCurrentGroup)
            {
                QListWidgetItem* item = new QListWidgetItem(
                    QString("%1 (%2)").arg(user->getNickname()).arg(user->getUserId()),
                    userListWidget);
                item->setData(
                    Qt::UserRole,
                    static_cast<qint64>(user->getUserId()));  // 将用户 ID 存储在 Item 的数据中
                userListWidget->addItem(item);
            }
        }
    }

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    selectButton = new QPushButton(tr("选择"), this);
    cancelButton = new QPushButton(tr("取消"), this);
    selectButton->setEnabled(false);  // 默认禁用选择按钮，直到有选择

    buttonLayout->addWidget(selectButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // 连接信号和槽
    connect(userListWidget, &QListWidget::itemSelectionChanged, this,
            &UserSelectionDialog::onSelectionChanged);
    connect(selectButton, &QPushButton::clicked, this,
            [this]()
            {
                QListWidgetItem* selectedItem = userListWidget->currentItem();
                if (selectedItem)
                {
                    m_selectedUserId = selectedItem->data(Qt::UserRole).toLongLong();
                    accept();  // 接受对话框并关闭
                }
                else
                {
                    QMessageBox::warning(this, tr("提示"), tr("请选择一个用户。"));
                }
            });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);  // 拒绝对话框并关闭
}

long UserSelectionDialog::getSelectedUserId() const
{
    return m_selectedUserId;
}

void UserSelectionDialog::onSelectionChanged()
{
    // 如果有选中项，则启用选择按钮
    selectButton->setEnabled(userListWidget->currentItem() != nullptr);
}