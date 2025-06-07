#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QVariant> // 引入 QVariant 头文件

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const QString& filePath)
{
    QFile configFile(filePath);
    if (!configFile.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开配置文件:" << filePath;
        // 如果无法加载，设置默认值，这样应用仍可启动
        m_tcpHost = "127.0.0.1";
        m_tcpPort = 9999;
        m_httpHost = "127.0.0.1";
        m_httpPort = 8080;
        m_apiPrefix = "/api"; // 假设有一个默认的 API 前缀
        return false;
    }

    QByteArray jsonData = configFile.readAll();
    configFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "配置文件解析错误:" << parseError.errorString();
        // 解析失败也设置默认值
        m_tcpHost = "127.0.0.1";
        m_tcpPort = 9999;
        m_httpHost = "127.0.0.1";
        m_httpPort = 8080;
        m_apiPrefix = "/api";
        return false;
    }

    QJsonObject config = doc.object();

    // 解析TCP配置
    QJsonObject tcpConfig = config.value("tcp").toObject();
    m_tcpHost = tcpConfig.value("host").toString("127.0.0.1");

    // === 修复点 1: 使用 toDouble() 后转换为 quint16 ===
    // JSON 数字实际存储为 double。
    // 然后将 double 转换为 quint16，并进行范围检查
    double tcpPortDouble = tcpConfig.value("port").toDouble(9999.0); // 默认值也改为 double
    if (tcpPortDouble >= 0 && tcpPortDouble <= 65535 && (qAbs(tcpPortDouble - qRound(tcpPortDouble)) < 0.0001)) { // 检查是否是整数且在有效范围
        m_tcpPort = static_cast<quint16>(qRound(tcpPortDouble));
    } else {
        qWarning() << "TCP 端口配置无效 (" << tcpPortDouble << ")，使用默认值 9999";
        m_tcpPort = 9999;
    }


    // 解析HTTP配置
    QJsonObject httpConfig = config.value("http").toObject();
    m_httpHost = httpConfig.value("host").toString("127.0.0.1");

    // === 修复点 2: 使用 toDouble() 后转换为 quint16 ===
    double httpPortDouble = httpConfig.value("port").toDouble(8080.0); // 默认值也改为 double
    if (httpPortDouble >= 0 && httpPortDouble <= 65535 && (qAbs(httpPortDouble - qRound(httpPortDouble)) < 0.0001)) { // 检查是否是整数且在有效范围
        m_httpPort = static_cast<quint16>(qRound(httpPortDouble));
    } else {
        qWarning() << "HTTP 端口配置无效 (" << httpPortDouble << ")，使用默认值 8080";
        m_httpPort = 8080;
    }

    m_apiPrefix = config.value("apiPrefix").toString("/api"); // 假设 apiPrefix 是顶层字段

    qDebug() << "Config loaded: TCP Host=" << m_tcpHost << ", TCP Port=" << m_tcpPort
             << ", HTTP Host=" << m_httpHost << ", HTTP Port=" << m_httpPort
             << ", API Prefix=" << m_apiPrefix;

    return true;
}