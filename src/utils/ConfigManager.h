#pragma once
#include <QString>
#include <QJsonObject>
#include <QtGlobal> // 引入 quint16 类型

class ConfigManager
{
public:
    static ConfigManager& instance();

    bool loadConfig(const QString& filePath);

    // Getters
    QString tcpHost() const { return m_tcpHost; }
    quint16 tcpPort() const { return m_tcpPort; } // 返回 quint16
    QString httpHost() const { return m_httpHost; }
    quint16 httpPort() const { return m_httpPort; } // 返回 quint16
    QString apiPrefix() const { return m_apiPrefix; }

    // Setters - 新增，用于从命令行参数更新配置
    void setTcpHost(const QString& host) { m_tcpHost = host; }
    void setTcpPort(quint16 port) { m_tcpPort = port; }
    void setHttpHost(const QString& host) { m_httpHost = host; }
    void setHttpPort(quint16 port) { m_httpPort = port; }
    void setApiPrefix(const QString& prefix) { m_apiPrefix = prefix; }

private:
    ConfigManager() = default; // 私有构造函数，实现单例

    // 私有成员变量，端口号改为 quint16
    QString m_tcpHost;
    quint16 m_tcpPort;
    QString m_httpHost;
    quint16 m_httpPort;
    QString m_apiPrefix;
};