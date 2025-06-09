#ifndef GLOBALEVENTBUS_H
#define GLOBALEVENTBUS_H

#include <QObject>
#include <QString>
#include <QJsonObject>  // 包含 QJsonObject
// #include <QDateTime> // timestamp 已经是 QString，所以 QDateTime 不一定需要，但通常用于内部处理
#include "utils/GroupTask.h"
class GlobalEventBus : public QObject
{
    Q_OBJECT

   public:
    // 获取单例实例的方法
    static GlobalEventBus* instance();

   signals:
    // 定义用于文件接收事件的信号。
    // 这个信号的签名必须与 MessageProcessor 原始信号以及 PrivateChatSession 槽的签名完全匹配。

    void globalAppendMessage(const QString& sender, const QString& receiver,
                             const QJsonValue& fileInfo, const QString& timestamp, bool isFile);

    // 用户信息可以从userinfo中获得, 因为是当前用户发送信息
    void sendGroupMessage(long groupId, const QString& content);

    void sendGroupInfo(const QJsonValue& content);
    //-------------
    // void sendGroupResponse(const QJsonValue& content); // 在messageProcessor类中处理消息,
    // 然后使用多个不同的信号总线进行传递

    void sendGroupCreate(long groupId, const QString& groupName, long creatorId);

    void sendGroupDelete(long groupId);

    void sendGroupAdd(long userId, long groupId);

    void sendGroupRemove(long userId, long groupId);

    void sendGroupError();  // 之后再改好了
    //-------------
    // 居然重名了
    void appendGroupMessage(const QString& senderUsername, const QString& senderNickname,
                            long groupId, const QString& content, const QString& timestamp);
    void taskSubmitted(GroupTask* task);

   private:
    // 私有构造函数，防止外部直接创建实例
    explicit GlobalEventBus(QObject* parent = nullptr);
    // 禁用拷贝构造函数和赋值运算符，确保单例唯一性
    GlobalEventBus(const GlobalEventBus&) = delete;
    GlobalEventBus& operator=(const GlobalEventBus&) = delete;

    static GlobalEventBus* m_instance;  // 静态成员，保存单例实例
};

#endif  // GLOBALEVENTBUS_H