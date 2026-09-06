/**
 * @file AnomalyMonitor.h
 * @brief Live ML anomaly detection monitor with QTimer-based sampling
 *
 * Periodically extracts FeatureVector from the running simulation,
 * feeds it through AnomalyDetector, and emits anomaly signals
 * when the score exceeds the configured threshold.
 */

#ifndef QUANTUMVERSE_ANOMALY_MONITOR_H
#define QUANTUMVERSE_ANOMALY_MONITOR_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <functional>
#include <memory>

#include "ml/FeatureVector.h"
#include "ml/AnomalyDetector.h"

namespace quantumverse {

class UI4D;
class DiscoveryPanelManager;

/**
 * @brief Live anomaly detection monitor
 *
 * Samples the simulation state at a fixed interval, computes an
 * anomaly score, and emits signals when anomalies are detected.
 *
 * Exposed to QML for user control (enable/disable, sensitivity).
 */
class AnomalyMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double thresholdMultiplier READ thresholdMultiplier WRITE setThresholdMultiplier NOTIFY thresholdMultiplierChanged)
    Q_PROPERTY(double lastScore READ lastScore NOTIFY scoreUpdated)
    Q_PROPERTY(int sampleIntervalMs READ sampleIntervalMs WRITE setSampleIntervalMs NOTIFY sampleIntervalMsChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY sampleCountChanged)

public:
    using FindingsProvider = std::function<std::vector<InstrumentFinding>()>;
    using TelemetryProvider = std::function<std::pair<UI4D::PhysicsTelemetry, std::vector<UI4D::SolarSystemBody>>()>;

    /**
     * @brief Construct a new AnomalyMonitor
     * @param parent QObject parent
     */
    explicit AnomalyMonitor(QObject* parent = nullptr);

    /**
     * @brief Destroy the AnomalyMonitor and stop sampling
     */
    ~AnomalyMonitor() override;

    // --- Dependencies ---

    /**
     * @brief Set the UI4D instance for feature extraction
     */
    void setUI4D(std::shared_ptr<UI4D> ui4d);

    /**
     * @brief Set the anomaly detector (loaded ML model)
     */
    void setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector);

    /**
     * @brief Set a callable that returns the current instrument findings
     */
    void setFindingsProvider(FindingsProvider provider);

    /**
     * @brief Set a callable that returns current telemetry and bodies
     */
    void setTelemetryProvider(TelemetryProvider provider);

    // --- Properties ---

    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);

    double thresholdMultiplier() const { return m_thresholdMultiplier; }
    void setThresholdMultiplier(double m);

    double lastScore() const { return m_lastScore; }

    int sampleIntervalMs() const { return m_timer->interval(); }
    void setSampleIntervalMs(int ms);

    int sampleCount() const { return m_sampleCount; }

    // --- Control ---

    /**
     * @brief Start periodic sampling
     */
    Q_INVOKABLE void start();

    /**
     * @brief Stop periodic sampling
     */
    Q_INVOKABLE void stop();

signals:
    void enabledChanged();
    void thresholdMultiplierChanged();
    void scoreUpdated(double score);
    void sampleIntervalMsChanged();
    void sampleCountChanged();

    /**
     * @brief Emitted when an anomaly is detected
     * @param anomaly JSON object with timestamp, score, threshold, and description
     */
    void anomalyDetected(const QJsonObject& anomaly);

private slots:
    /**
     * @brief Timer callback: extract features and score
     */
    void onSamplingTimeout();

private:
    /**
     * @brief Extract features and run inference for the current frame
     */
    void processSample();

    QTimer* m_timer;
    std::shared_ptr<UI4D> m_ui4d;
    std::shared_ptr<AnomalyDetector> m_detector;
    FindingsProvider m_findingsProvider;
    TelemetryProvider m_telemetryProvider;

    bool m_enabled;
    double m_thresholdMultiplier;
    double m_lastScore;
    int m_sampleCount;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ANOMALY_MONITOR_H
