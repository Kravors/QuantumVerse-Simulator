/**
 * @file AlertToFinding.h
 * @brief Convert multi-messenger alerts into discovery InstrumentFindings
 *
 * Shared by the live wiring (main_qml.cpp) and the unit test so the
 * alert -> finding mapping stays in exactly one place.
 */

#ifndef QUANTUMVERSE_ALERT_TO_FINDING_H
#define QUANTUMVERSE_ALERT_TO_FINDING_H

#include <algorithm>
#include <string>

#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "discovery/DiscoveryInstrument.h"

namespace quantumverse {

/// @brief Map a false-alarm rate (and optional explicit confidence) to [0,1].
inline double alertConfidence(double falseAlarmRate, double explicitConfidence)
{
    if (explicitConfidence > 0.0) return std::min(1.0, explicitConfidence);
    // Lower FAR => higher confidence; 1 - FAR saturates near 1 for small FAR.
    return std::min(1.0, 1.0 - falseAlarmRate);
}

/// @brief Convert a gravitational-wave alert into an InstrumentFinding.
inline InstrumentFinding toInstrumentFinding(const GravitationalWaveAlert& a)
{
    InstrumentFinding f;
    f.id = "LIGO_" + a.event_id;
    f.instrumentName = "LIGO";
    f.description = "Gravitational-wave alert " + a.event_id +
        " (SNR=" + std::to_string(a.snr) +
        ", FAR=" + std::to_string(a.false_alarm_rate) + ")";
    f.confidence = alertConfidence(a.false_alarm_rate, a.confidence);
    f.severity = (f.confidence >= 0.99) ? AlertSeverity::CRITICAL :
                 (f.confidence >= 0.95) ? AlertSeverity::HIGH :
                 (f.confidence >= 0.80) ? AlertSeverity::MEDIUM :
                                          AlertSeverity::LOW;
    f.timestamp = 0.0;
    return f;
}

/// @brief Convert a neutrino alert into an InstrumentFinding.
inline InstrumentFinding toInstrumentFinding(const NeutrinoAlert& a)
{
    InstrumentFinding f;
    f.id = "IceCube_" + a.event_id;
    f.instrumentName = "IceCube";
    f.description = "Neutrino alert " + a.event_id +
        " (E=" + std::to_string(a.energy_tev) + " TeV" +
        ", FAR=" + std::to_string(a.false_alarm_rate) + ")";
    f.confidence = alertConfidence(a.false_alarm_rate, a.confidence);
    f.severity = (f.confidence >= 0.99) ? AlertSeverity::CRITICAL :
                 (f.confidence >= 0.95) ? AlertSeverity::HIGH :
                 (f.confidence >= 0.80) ? AlertSeverity::MEDIUM :
                                          AlertSeverity::LOW;
    f.timestamp = 0.0;
    return f;
}

} // namespace quantumverse

#endif // QUANTUMVERSE_ALERT_TO_FINDING_H
