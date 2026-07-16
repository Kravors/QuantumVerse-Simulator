/**
 * @file GCNNoticeParser.cpp
 * @brief Implementation of GCN Kafka JSON notice parser
 */

#include "GCNNoticeParser.h"
#include <QJsonDocument>
#include <QJsonValue>
#include <algorithm>

namespace quantumverse {

static QString jsonString(const QJsonObject& obj, const char* key, const QString& def = {})
{
    if (!obj.contains(key)) return def;
    const QJsonValue v = obj.value(key);
    if (!v.isString()) return def;
    return v.toString();
}

static double jsonDouble(const QJsonObject& obj, const char* key, double def = 0.0)
{
    if (!obj.contains(key)) return def;
    const QJsonValue v = obj.value(key);
    if (v.isUndefined() || v.isNull()) return def;
    return v.toDouble(def);
}

AlertOrigin GCNNoticeParser::classify(const QJsonObject& json)
{
    const QString raw = jsonString(json, "alert_type").toLower();
    if (raw.contains("ligo") || raw.contains("virgo") || raw.contains("kagra") || raw.contains("gw"))
        return AlertOrigin::LIGO;
    if (raw.contains("icecube") || raw.contains("neutrino"))
        return AlertOrigin::IceCube;
    if (raw.contains("tess") || raw.contains("toi"))
        return AlertOrigin::TESS;
    if (raw.contains("fermi") || raw.contains("gbm"))
        return AlertOrigin::FermiGBM;
    if (raw.contains("swift"))
        return AlertOrigin::Swift;
    return AlertOrigin::Unknown;
}

ParsedGCNNotice GCNNoticeParser::parse(const QJsonObject& json)
{
    ParsedGCNNotice out;
    out.raw_type = jsonString(json, "alert_type");

    switch (classify(json)) {
    case AlertOrigin::LIGO: {
        out.origin = AlertOrigin::LIGO;
        out.gw.event_id = jsonString(json, "event_id").toStdString();
        out.gw.false_alarm_rate = jsonDouble(json, "false_alarm_rate");
        out.gw.snr = jsonDouble(json, "snr");
        out.gw.m1 = jsonDouble(json, "m1");
        out.gw.m2 = jsonDouble(json, "m2");
        out.gw.confidence = jsonDouble(json, "confidence");
        out.gw.ra = jsonDouble(json, "ra");
        out.gw.dec = jsonDouble(json, "dec");
        break;
    }
    case AlertOrigin::IceCube: {
        out.origin = AlertOrigin::IceCube;
        out.neutrino.event_id = jsonString(json, "event_id").toStdString();
        out.neutrino.energy_tev = jsonDouble(json, "energy_tev");
        out.neutrino.false_alarm_rate = jsonDouble(json, "false_alarm_rate");
        out.neutrino.ra = jsonDouble(json, "ra");
        out.neutrino.dec = jsonDouble(json, "dec");
        out.neutrino.confidence = jsonDouble(json, "confidence");
        break;
    }
    case AlertOrigin::TESS: {
        out.origin = AlertOrigin::TESS;
        out.tess.toi_id = jsonString(json, "toi_id").toStdString();
        out.tess.period_days = jsonDouble(json, "period_days");
        out.tess.depth_ppm = jsonDouble(json, "depth_ppm");
        out.tess.duration_hours = jsonDouble(json, "duration_hours");
        out.tess.confidence = jsonDouble(json, "confidence");
        out.tess.ra = jsonDouble(json, "ra");
        out.tess.dec = jsonDouble(json, "dec");
        break;
    }
    default:
        out.origin = AlertOrigin::Unknown;
        break;
    }

    return out;
}

QStringList GCNNoticeParser::supportedTypes()
{
    return {
        "LIGO/Virgo", "LIGO", "Virgo", "KAGRA",
        "IceCube",
        "TESS", "TESS/TOI",
        "Fermi/GBM",
        "Swift"
    };
}

} // namespace quantumverse
