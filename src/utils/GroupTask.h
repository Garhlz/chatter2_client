#ifndef GROUPTASK_H
#define GROUPTASK_H

#include <QObject>
#include <QString>
#include <QDebug>  // 用于 toString() 方法的调试输出

class GroupTask : public QObject
{
    Q_OBJECT

   public:
    // 构造函数：初始化所有字段
    explicit GroupTask(const QString& operationId = "", const QString& type = "", long groupId = 0,
                       long operatorId = 0, const QString& groupName = "", long userId = 0,
                       QObject* parent = nullptr);

    // 拷贝构造函数
    GroupTask(const GroupTask& other);
    // 赋值运算符
    GroupTask& operator=(const GroupTask& other);

    // Getters: 获取字段值
    QString getOperationId() const;
    QString getType() const;
    long getGroupId() const;
    long getOperatorId() const;
    QString getGroupName() const;
    long getUserId() const;

    // Setters: 设置字段值
    void setOperationId(const QString& id);
    void setType(const QString& t);
    void setGroupId(long id);
    void setOperatorId(long id);
    void setGroupName(const QString& name);
    void setUserId(long id);

    // 方便调试的打印方法
    QString toString() const;

   private:
    QString m_operationId;  // 唯一的操作ID，用于跟踪任务
    QString m_type;         // 任务类型，例如 "CREATE_GROUP", "ADD_MEMBER"
    long m_groupId;         // 相关联的群组ID
    long m_operatorId;      // 执行此操作的用户ID (谁发起的)
    QString m_groupName;    // 相关联的群组名称
    long m_userId;          // 目标用户ID (如果任务与单个用户有关，例如添加/删除特定用户)
};

#endif  // GROUPTASK_H