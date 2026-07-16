/**
 * @file AlertRouter.cpp
 * @brief Implementation of the GCN alert router
 */

#include "AlertRouter.h"
#include "GCNNoticeParser.h"
#include "TESSAlertAdapter.h"

namespace quantumverse {

AlertRouter::AlertRouter(QObject* parent)
    : QObject(parent)
    , m_ligo(nullptr)
    , m_icecube(nullptr)
    , m_tess(nullptr)
{
}

void AlertRouter::routeAlert(const QJsonObject& alert)
{
    const ParsedGCNNotice parsed = GCNNoticeParser::parse(alert);
    emit parsedAlertReady(alert, parsed.origin);

    switch (parsed.origin) {
    case AlertOrigin::LIGO: {
        if (!m_ligo) {
            emit routingError(QStringLiteral("No LIGO adapter registered"));
            return;
        }
        GravitationalWaveAlert gw;
        gw.event_id = parsed.gw.event_id;
        gw.false_alarm_rate = parsed.gw.false_alarm_rate;
        gw.snr = parsed.gw.snr;
        gw.m1 = parsed.gw.m1;
        gw.m2 = parsed.gw.m2;
        gw.confidence = parsed.gw.confidence;
        m_ligo->simulateAlert(gw);
        emit alertRouted(QStringLiteral("LIGO"));
        break;
    }
    case AlertOrigin::IceCube: {
        if (!m_icecube) {
            emit routingError(QStringLiteral("No IceCube adapter registered"));
            return;
        }
        NeutrinoAlert nu;
        nu.event_id = parsed.neutrino.event_id;
        nu.energy_tev = parsed.neutrino.energy_tev;
        nu.false_alarm_rate = parsed.neutrino.false_alarm_rate;
        nu.ra = parsed.neutrino.ra;
        nu.dec = parsed.neutrino.dec;
        nu.confidence = parsed.neutrino.confidence;
        m_icecube->simulateAlert(nu);
        emit alertRouted(QStringLiteral("IceCube"));
        break;
    }
    case AlertOrigin::TESS: {
        if (!m_tess) {
            emit routingError(QStringLiteral("No TESS adapter registered"));
            return;
        }
        TESSAlert tess;
        tess.toi_id = parsed.tess.toi_id;
        tess.period_days = parsed.tess.period_days;
        tess.depth_ppm = parsed.tess.depth_ppm;
        tess.duration_hours = parsed.tess.duration_hours;
        tess.confidence = parsed.tess.confidence;
        tess.ra = parsed.tess.ra;
        tess.dec = parsed.tess.dec;
        m_tess->simulateAlert(tess);
        emit alertRouted(QStringLiteral("TESS"));
        break;
    }
    default:
        emit routingError(QStringLiteral("Unsupported alert type: %1").arg(parsed.raw_type));
        break;
    }
}

} // namespace quantumverse
