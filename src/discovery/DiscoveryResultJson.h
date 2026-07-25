#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include "discovery/DiscoveryEngine.h"

namespace quantumverse {

inline QJsonObject discoveryResultToJson(const DiscoveryResult& r) {
    QJsonObject obj;
    obj["id"] = QString::fromStdString(r.id);
    obj["type"] = QString::fromStdString(r.type);
    obj["description"] = QString::fromStdString(r.description);
    obj["confidence"] = r.confidence;
    QJsonObject loc;
    loc["t"] = r.location.t;
    loc["x"] = r.location.x;
    loc["y"] = r.location.y;
    loc["z"] = r.location.z;
    obj["location"] = loc;
    QJsonObject params;
    for (const auto& [key, val] : r.parameters) {
        params[QString::fromStdString(key)] = val;
    }
    obj["parameters"] = params;
    obj["fieldEquation"] = QString::fromStdString(r.fieldEquation);
    obj["timestamp"] = r.timestamp;
    obj["validated"] = r.validated;
    return obj;
}

inline DiscoveryResult discoveryResultFromJson(const QJsonObject& obj) {
    DiscoveryResult r;
    r.id = obj["id"].toString().toStdString();
    r.type = obj["type"].toString().toStdString();
    r.description = obj["description"].toString().toStdString();
    r.confidence = obj["confidence"].toDouble();
    QJsonObject loc = obj["location"].toObject();
    r.location = Event4D(loc["t"].toDouble(), loc["x"].toDouble(),
                         loc["y"].toDouble(), loc["z"].toDouble());
    QJsonObject params = obj["parameters"].toObject();
    for (auto it = params.begin(); it != params.end(); ++it) {
        r.parameters[it.key().toStdString()] = it.value().toDouble();
    }
    r.fieldEquation = obj["fieldEquation"].toString().toStdString();
    r.timestamp = obj["timestamp"].toDouble();
    r.validated = obj["validated"].toBool();
    return r;
}

} // namespace quantumverse
