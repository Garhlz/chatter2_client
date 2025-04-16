#ifndef JSONCONVERTER_H
#define JSONCONVERTER_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

class JsonConverter
{
   public:
    static QByteArray toUtf8Json(const QJsonObject& jsonObj);
    static QJsonObject toJsonObject(const QByteArray& data);
};

#endif  // JSONCONVERTER_H