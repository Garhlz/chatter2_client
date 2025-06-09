#include "GroupTask.h"
// #include <QDebug> // 已在 .h 中包含，这里可选

// 构造函数实现
GroupTask::GroupTask(const QString& operationId, const QString& type, long groupId, long operatorId,
                     const QString& groupName, long userId, QObject* parent)
    : QObject(parent),
      m_operationId(operationId),
      m_type(type),
      m_groupId(groupId),
      m_operatorId(operatorId),
      m_groupName(groupName),
      m_userId(userId)
{
    // qDebug() << "GroupTask created: " << toString(); // 构造时打印，方便调试
}

// 拷贝构造函数实现
GroupTask::GroupTask(const GroupTask& other)
    : QObject(nullptr),  // 拷贝时不继承父对象
      m_operationId(other.m_operationId),
      m_type(other.m_type),
      m_groupId(other.m_groupId),
      m_operatorId(other.m_operatorId),
      m_groupName(other.m_groupName),
      m_userId(other.m_userId)
{
}

// 赋值运算符实现
GroupTask& GroupTask::operator=(const GroupTask& other)
{
    if (this != &other)
    {
        // 不需要处理QObject的父子关系，因为赋值操作不改变父子关系
        m_operationId = other.m_operationId;
        m_type = other.m_type;
        m_groupId = other.m_groupId;
        m_operatorId = other.m_operatorId;
        m_groupName = other.m_groupName;
        m_userId = other.m_userId;
    }
    return *this;
}

// Getters
QString GroupTask::getOperationId() const
{
    return m_operationId;
}

QString GroupTask::getType() const
{
    return m_type;
}

long GroupTask::getGroupId() const
{
    return m_groupId;
}

long GroupTask::getOperatorId() const
{
    return m_operatorId;
}

QString GroupTask::getGroupName() const
{
    return m_groupName;
}

long GroupTask::getUserId() const
{
    return m_userId;
}

// Setters
void GroupTask::setOperationId(const QString& id)
{
    m_operationId = id;
}

void GroupTask::setType(const QString& t)
{
    m_type = t;
}

void GroupTask::setGroupId(long id)
{
    m_groupId = id;
}

void GroupTask::setOperatorId(long id)
{
    m_operatorId = id;
}

void GroupTask::setGroupName(const QString& name)
{
    m_groupName = name;
}

void GroupTask::setUserId(long id)
{
    m_userId = id;
}

// toString方法，方便调试输出
QString GroupTask::toString() const
{
    return QString(
               "GroupTask { OperationId: %1, Type: %2, GroupId: %3, OperatorId: %4, GroupName: %5, "
               "UserId: %6 }")
        .arg(m_operationId)
        .arg(m_type)
        .arg(m_groupId)
        .arg(m_operatorId)
        .arg(m_groupName)
        .arg(m_userId);
}