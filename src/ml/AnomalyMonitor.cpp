/**
 * @file AnomalyMonitor.cpp
 * @brief Live ML anomaly detection monitor implementation
 */

#include "ml/AnomalyMonitor.h"

#include <cmath>

namespace quantumverse {

AnomalyMonitor::AnomalyMonitor(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_enabled(false)
    , m_thresholdMultiplier(1.0)
    , m_lastScore(0.0)
    , m_modelLoaded(false)
    , m_sampleCount(0)
{
    m_timer->setInterval(1000);
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, &AnomalyMonitor::onSamplingTimeout);
}

AnomalyMonitor::~AnomalyMonitor()
{
    stop();
}

void AnomalyMonitor::setUI4D(std::shared_ptr<UI4D> ui4d)
{
    m_ui4d = std::move(ui4d);
}

void AnomalyMonitor::setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector)
{
    m_detector = std::move(detector);
    bool loaded = m_detector && m_detector->isLoaded();
    setModelLoaded(loaded);
}

void AnomalyMonitor::setFindingsProvider(FindingsProvider provider)
{
    m_findingsProvider = std::move(provider);
}

void AnomalyMonitor::setTelemetryProvider(TelemetryProvider provider)
{
    m_telemetryProvider = std::move(provider);
}

void AnomalyMonitor::setEnabled(bool e)
{
    if (m_enabled == e) return;
    m_enabled = e;
    emit enabledChanged();
    if (m_enabled) {
        start();
    } else {
        stop();
    }
}

void AnomalyMonitor::setThresholdMultiplier(double m)
{
    if (qFuzzyCompare(m_thresholdMultiplier, m)) return;
    m_thresholdMultiplier = m;
    emit thresholdMultiplierChanged();
}

void AnomalyMonitor::setModelLoaded(bool loaded)
{
    if (m_modelLoaded == loaded) return;
    m_modelLoaded = loaded;
    emit modelLoadedChanged();
    if (!loaded) {
        m_lastScore = -1.0;
        emit scoreUpdated(-1.0);
    }
}

void AnomalyMonitor::setSampleIntervalMs(int ms)
{
    if (ms < 100) ms = 100;
    m_timer->setInterval(ms);
    emit sampleIntervalMsChanged();
}

void AnomalyMonitor::start()
{
    if (!m_enabled) {
        m_enabled = true;
        emit enabledChanged();
    }
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void AnomalyMonitor::stop()
{
    if (m_enabled) {
        m_enabled = false;
        emit enabledChanged();
    }
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void AnomalyMonitor::onSamplingTimeout()
{
    if (!m_enabled) return;
    processSample();
}

void AnomalyMonitor::processSample()
{
    if (!m_modelLoaded || !m_detector) {
        if (m_lastScore != -1.0) {
            m_lastScore = -1.0;
            emit scoreUpdated(-1.0);
        }
        return;
    }

    try {
        UI4D::PhysicsTelemetry telemetry{};
        std::vector<UI4D::SolarSystemBody> bodies;

        if (m_telemetryProvider) {
            auto result = m_telemetryProvider();
            telemetry = result.first;
            bodies = std::move(result.second);
        } else if (m_ui4d) {
            telemetry = m_ui4d->telemetry();
            const auto& solarData = m_ui4d->getSolarSystem();
            bodies.reserve(solarData.bodies.size());
            for (const auto& kv : solarData.bodies) {
                bodies.push_back(kv.second);
            }
        }

        std::vector<InstrumentFinding> findings;
        if (m_findingsProvider) {
            findings = m_findingsProvider();
        }

        FeatureVector fv = FeatureVector::extract(
            telemetry,
            telemetry.blackHoleMass,
            bodies,
            findings
        );

        double score = m_detector->score(fv);

        // Gate on NaN/Inf — reject garbage scores from diverged integration
        if (!std::isfinite(score)) {
            qWarning() << "AnomalyMonitor: non-finite score" << score << "—rejecting";
            if (m_lastScore != -1.0) {
                m_lastScore = -1.0;
                emit scoreUpdated(-1.0);
            }
            return;
        }

        m_lastScore = score;
        ++m_sampleCount;
        emit scoreUpdated(score);
        emit sampleCountChanged();

        double threshold = m_detector->threshold() * m_thresholdMultiplier;
        if (score > threshold) {
            QJsonObject anomaly;
            anomaly["timestamp"] = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            anomaly["score"] = score;
            anomaly["threshold"] = threshold;
            anomaly["description"] = QStringLiteral("ML anomaly detected (score=%1, threshold=%2)").arg(score).arg(threshold);
            emit anomalyDetected(anomaly);
        }
    } catch (const std::exception& e) {
        qWarning() << "AnomalyMonitor sampling error:" << e.what();
        if (m_lastScore != -1.0) {
            m_lastScore = -1.0;
            emit scoreUpdated(-1.0);
        }
    }
}

} // namespace quantumverse
