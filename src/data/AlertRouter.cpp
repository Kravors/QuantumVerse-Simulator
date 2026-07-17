/**
 * @file AlertRouter.cpp
 * @brief Implementation of the GCN alert router
 */

#include "AlertRouter.h"
#include "GCNNoticeParser.h"
#include "TESSAlertAdapter.h"
#include "FermiGBMAdapter.h"
#include "SwiftBATAdapter.h"

namespace quantumverse {

AlertRouter::AlertRouter(QObject* parent)
    : QObject(parent)
    , m_ligo(nullptr)
    , m_icecube(nullptr)
    , m_tess(nullptr)
    , m_fermi_gbm(nullptr)
    , m_swift_bat(nullptr)
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
    case AlertOrigin::FermiGBM: {
        if (!m_fermi_gbm) {
            emit routingError(QStringLiteral("No Fermi GBM adapter registered"));
            return;
        }
        FermiGBMAlert grb;
        grb.trigger_id = parsed.fermi_gbm.trigger_id;
        grb.duration = parsed.fermi_gbm.duration;
        grb.peak_flux = parsed.fermi_gbm.peak_flux;
        grb.false_alarm_rate = parsed.fermi_gbm.false_alarm_rate;
        grb.ra = parsed.fermi_gbm.ra;
        grb.dec = parsed.fermi_gbm.dec;
        grb.error_radius = parsed.fermi_gbm.error_radius;
        grb.confidence = parsed.fermi_gbm.confidence;
        m_fermi_gbm->simulateAlert(grb);
        emit alertRouted(QStringLiteral("FermiGBM"));
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
    case AlertOrigin::Swift: {
        if (!m_swift_bat) {
            emit routingError(QStringLiteral("No Swift BAT adapter registered"));
            return;
        }
        SwiftBATAlert bat;
        bat.trigger_id = parsed.swift_bat.trigger_id;
        bat.bat_rate = parsed.swift_bat.bat_rate;
        bat.xrt_flux = parsed.swift_bat.xrt_flux;
        bat.duration = parsed.swift_bat.duration;
        bat.false_alarm_rate = parsed.swift_bat.false_alarm_rate;
        bat.ra = parsed.swift_bat.ra;
        bat.dec = parsed.swift_bat.dec;
        bat.error_radius = parsed.swift_bat.error_radius;
        bat.confidence = parsed.swift_bat.confidence;
        m_swift_bat->simulateAlert(bat);
        emit alertRouted(QStringLiteral("SwiftBAT"));
        break;
    }
    default:
        emit routingError(QStringLiteral("Unsupported alert type: %1").arg(parsed.raw_type));
        break;
    }
}

} // namespace quantumverse
