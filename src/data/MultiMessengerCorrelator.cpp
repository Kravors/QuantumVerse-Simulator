/**
 * @file MultiMessengerCorrelator.cpp
 * @brief Implementation of real-time multi-messenger correlation
 */

#include "MultiMessengerCorrelator.h"

#include <algorithm>
#include <cmath>
#include <QDateTime>
#include <QDebug>

namespace quantumverse {

MultiMessengerCorrelator::MultiMessengerCorrelator(QObject* parent)
    : QObject(parent)
{
}

void MultiMessengerCorrelator::setSpatialThresholdDeg(double deg)
{
    if (deg <= 0.0) deg = 1.0;
    if (qAbs(m_spatialThresholdDeg - deg) < 1e-9) return;
    m_spatialThresholdDeg = deg;
    emit spatialThresholdDegChanged();
}

void MultiMessengerCorrelator::setTimeWindowSec(double sec)
{
    if (sec <= 0.0) sec = 86400.0;
    if (qAbs(m_timeWindowSec - sec) < 1e-9) return;
    m_timeWindowSec = sec;
    emit timeWindowSecChanged();
}

void MultiMessengerCorrelator::addAlert(const InstrumentFinding& finding)
{
    Alert a;
    a.messenger = QString::fromStdString(finding.instrumentName);
    a.ra = finding.location.x;
    a.dec = finding.location.y;
    a.timestamp = finding.timestamp;
    a.confidence = finding.confidence;
    a.id = QString::fromStdString(finding.id);
    m_alerts.append(a);

    QVector<Alert> candidates;
    const double tNow = a.timestamp;
    for (const Alert& other : std::as_const(m_alerts)) {
        if (other.id == a.id) continue;
        if (qAbs(other.timestamp - tNow) > m_timeWindowSec) continue;
        candidates.append(other);
    }

    for (const Alert& other : candidates) {
        const double sep = angularSeparationDeg(a.ra, a.dec, other.ra, other.dec);
        if (sep > m_spatialThresholdDeg) continue;

        const double tDiff = qAbs(a.timestamp - other.timestamp);
        const double tScore = 1.0 - std::min(1.0, tDiff / m_timeWindowSec);
        const double sScore = 1.0 - std::min(1.0, sep / m_spatialThresholdDeg);
        const double jointScore = 0.5 * (tScore + sScore);

        const double combinedConf = std::min(1.0, a.confidence * other.confidence + 0.25 * jointScore);

        AlertSeverity sev = AlertSeverity::INFO;
        if (combinedConf >= 0.99) sev = AlertSeverity::CRITICAL;
        else if (combinedConf >= 0.95) sev = AlertSeverity::HIGH;
        else if (combinedConf >= 0.80) sev = AlertSeverity::MEDIUM;
        else if (combinedConf >= 0.50) sev = AlertSeverity::LOW;

        const double raAvg = (a.ra + other.ra) / 2.0;
        const double decAvg = (a.dec + other.dec) / 2.0;

        CorrelationEvent ev;
        ev.id = QStringLiteral("CORR_%1_%2").arg(a.id, other.id);
        ev.messengers = { a.messenger, other.messenger };
        ev.ra = raAvg;
        ev.dec = decAvg;
        ev.timestamp = qMin(a.timestamp, other.timestamp);
        ev.combinedConfidence = combinedConf;
        ev.spatialScore = sScore;
        ev.description = QStringLiteral("Multi-messenger coincidence: %1 + %2 | separation=%.3f deg, joint=%.1f%%")
                             .arg(a.messenger, other.messenger)
                             .arg(sep)
                             .arg(combinedConf * 100.0);
        ev.severity = sev;
        ev.alertIds = { a.id, other.id };

        m_correlations.append(ev);
        emit correlationDetected(ev);
        emit correlationCountChanged();
    }
}

void MultiMessengerCorrelator::clear()
{
    m_alerts.clear();
    m_correlations.clear();
    emit correlationCountChanged();
}

double MultiMessengerCorrelator::angularSeparationDeg(double ra1, double dec1, double ra2, double dec2)
{
    const double toRad = M_PI / 180.0;
    const double ra1r = ra1 * toRad;
    const double dec1r = dec1 * toRad;
    const double ra2r = ra2 * toRad;
    const double dec2r = dec2 * toRad;

    const double cosSep = std::cos(M_PI / 2.0 - dec1r) * std::cos(M_PI / 2.0 - dec2r)
                        + std::sin(M_PI / 2.0 - dec1r) * std::sin(M_PI / 2.0 - dec2r)
                        * std::cos(ra1r - ra2r);

    const double clamped = std::max(-1.0, std::min(1.0, cosSep));
    const double sepRad = std::acos(clamped);
    return sepRad / toRad;
}

QString MultiMessengerCorrelator::severityToString(AlertSeverity sev)
{
    switch (sev) {
    case AlertSeverity::INFO:     return "INFO";
    case AlertSeverity::LOW:      return "LOW";
    case AlertSeverity::MEDIUM:   return "MEDIUM";
    case AlertSeverity::HIGH:     return "HIGH";
    case AlertSeverity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

} // namespace quantumverse
