/**
 * @file MultiMessengerCorrelator.h
 * @brief Correlates live alerts from multiple messengers in real time
 *
 * Ingests findings from LIGO, IceCube, and TESS adapters and detects
 * coincident alerts based on spatial proximity (RA/Dec) and temporal
 * coincidence within configurable windows. Emits a correlation event
 * whenever a multi-messenger coincidence is identified.
 */

#ifndef QUANTUMVERSE_MULTI_MESSENGER_CORRELATOR_H
#define QUANTUMVERSE_MULTI_MESSENGER_CORRELATOR_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <cmath>

#include "discovery/DiscoveryInstrument.h"

namespace quantumverse {

/**
 * @brief Single correlated multi-messenger event
 */
struct CorrelationEvent {
    QString id;
    QStringList messengers;          // e.g. ["LIGO", "IceCube"]
    double ra = 0.0;                 // degrees
    double dec = 0.0;                // degrees
    double timestamp = 0.0;          // Unix seconds
    double combinedConfidence = 0.0;  // joint confidence
    double spatialScore = 0.0;       // 0..1 proximity score
    QString description;
    AlertSeverity severity = AlertSeverity::INFO;
    QVector<QString> alertIds;       // source finding IDs
};

/**
 * @brief Real-time multi-messenger correlation engine
 */
class MultiMessengerCorrelator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double spatialThresholdDeg READ spatialThresholdDeg WRITE setSpatialThresholdDeg NOTIFY spatialThresholdDegChanged)
    Q_PROPERTY(double timeWindowSec READ timeWindowSec WRITE setTimeWindowSec NOTIFY timeWindowSecChanged)
    Q_PROPERTY(int correlationCount READ correlationCount NOTIFY correlationCountChanged)

public:
    explicit MultiMessengerCorrelator(QObject* parent = nullptr);
    ~MultiMessengerCorrelator() override = default;

    /** @brief Ingest a new alert finding for correlation. */
    Q_INVOKABLE void addAlert(const InstrumentFinding& finding);

    /** @brief Remove all stored alerts and correlations. */
    Q_INVOKABLE void clear();

    /** @brief Current spatial coincidence threshold in degrees. */
    double spatialThresholdDeg() const { return m_spatialThresholdDeg; }

    /** @brief Current temporal coincidence window in seconds. */
    double timeWindowSec() const { return m_timeWindowSec; }

    /** @brief Number of detected correlations so far. */
    int correlationCount() const { return m_correlations.size(); }

    /** @brief All detected correlations. */
    QVector<CorrelationEvent> correlations() const { return m_correlations; }

    /** @brief Set spatial threshold in degrees (default 1.0). */
    void setSpatialThresholdDeg(double deg);

    /** @brief Set time window in seconds (default 86400 = 24h). */
    void setTimeWindowSec(double sec);

signals:
    void spatialThresholdDegChanged();
    void timeWindowSecChanged();
    void correlationCountChanged();
    /** @brief Emitted when a new multi-messenger correlation is detected. */
    void correlationDetected(const CorrelationEvent& correlation);

private:
    struct Alert {
        QString messenger;
        double ra = 0.0;
        double dec = 0.0;
        double timestamp = 0.0;
        double confidence = 0.0;
        QString id;
    };

    static double angularSeparationDeg(double ra1, double dec1, double ra2, double dec2);
    static QString severityToString(AlertSeverity sev);

    QVector<Alert> m_alerts;
    QVector<CorrelationEvent> m_correlations;
    double m_spatialThresholdDeg = 1.0;
    double m_timeWindowSec = 86400.0;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_MULTI_MESSENGER_CORRELATOR_H
