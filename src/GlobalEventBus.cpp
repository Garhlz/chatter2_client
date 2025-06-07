#include "GlobalEventBus.h"

// 初始化静态成员为 nullptr
GlobalEventBus* GlobalEventBus::m_instance = nullptr;

GlobalEventBus::GlobalEventBus(QObject* parent) : QObject{parent}
{
    // 构造函数可以为空，或者做一些初始化工作
    // 例如： qRegisterMetaType<QJsonObject>("QJsonObject"); // 如果 QJsonObject
    // 作为信号槽参数需要跨线程，则可能需要注册
}

GlobalEventBus* GlobalEventBus::instance()
{
    if (m_instance == nullptr)
    {
        // 第一次调用时创建实例
        // 确保它没有父对象，以便其生命周期与应用程序保持一致
        m_instance = new GlobalEventBus();
    }
    return m_instance;
}