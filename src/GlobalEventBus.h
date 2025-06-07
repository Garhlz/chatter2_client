#ifndef GLOBALEVENTBUS_H
#define GLOBALEVENTBUS_H

#include <QObject>
#include <QString>
#include <QJsonObject> // 包含 QJsonObject
// #include <QDateTime> // timestamp 已经是 QString，所以 QDateTime 不一定需要，但通常用于内部处理

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

private:
    // 私有构造函数，防止外部直接创建实例
    explicit GlobalEventBus(QObject *parent = nullptr);
    // 禁用拷贝构造函数和赋值运算符，确保单例唯一性
    GlobalEventBus(const GlobalEventBus&) = delete;
    GlobalEventBus& operator=(const GlobalEventBus&) = delete;

    static GlobalEventBus* m_instance; // 静态成员，保存单例实例
};

#endif // GLOBALEVENTBUS_H