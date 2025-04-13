#include "JsonConverter.h"

QByteArray JsonConverter::toUtf8Json(const QJsonObject &jsonObj) {
    QJsonDocument doc(jsonObj);
    return doc.toJson(QJsonDocument::Compact);
}

QJsonObject JsonConverter::toJsonObject(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.object();
}