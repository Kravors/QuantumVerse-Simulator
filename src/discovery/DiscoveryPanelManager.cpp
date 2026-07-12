/**
 * @file DiscoveryPanelManager.cpp
 * @brief Implementation of the Discovery Panel Manager
 */

#include "DiscoveryPanelManager.h"
#include "DiscoveryEngine.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

#ifdef QUANTUMVERSE_USE_QML

#include <QDebug>

namespace quantumverse {

DiscoveryPanelManager::DiscoveryPanelManager(QObject* parent)
    : QObject(parent)
    , m_activeInstrumentIndex(0)
    , m_scanRunning(false)
    , m_scanProgress(0.0)
{
}

DiscoveryPanelManager::~DiscoveryPanelManager() = default;

void DiscoveryPanelManager::registerInstrument(std::unique_ptr<DiscoveryInstrument> instrument)
{
    m_instruments.push_back(std::move(instrument));
    emit instrumentCountChanged();
}

DiscoveryInstrument* DiscoveryPanelManager::instrument(int index)
{
    if (index >= 0 && index < static_cast<int>(m_instruments.size())) {
        return m_instruments[index].get();
    }
    return nullptr;
}

const DiscoveryInstrument* DiscoveryPanelManager::instrument(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_instruments.size())) {
        return m_instruments[index].get();
    }
    return nullptr;
}

void DiscoveryPanelManager::runScan(const MetricTensor& metric, const Event4D& location,
                                 const std::vector<Event4D>& trajectory)
{
    if (m_instruments.empty()) return;

    m_scanRunning = true;
    emit scanRunningChanged();

    m_allFindings.clear();

    for (size_t i = 0; i < m_instruments.size(); ++i) {
        if (!m_instruments[i]->isEnabled()) continue;

        auto findings = m_instruments[i]->analyze(metric, location, trajectory);
        qWarning() << "DIAG runScan instrument[" << i << "]"
                   << QString::fromStdString(m_instruments[i]->getName()).constData()
                   << "enabled=" << m_instruments[i]->isEnabled()
                   << "findings=" << findings.size();
        for (auto& f : findings) {
            m_allFindings.push_back(f);
            emit newFindingDiscovered(
                QString::fromStdString(f.instrumentName),
                QString::fromStdString(f.description),
                f.confidence
            );
        }

        m_scanProgress = static_cast<double>(i + 1) / m_instruments.size();
        emit scanProgressChanged();
    }

    m_scanRunning = false;
    m_scanProgress = 0.0;
    emit scanRunningChanged();
    emit scanProgressChanged();
    emit findingsListChanged();
    emit findingsChanged();
    emit scanComplete();
}

void DiscoveryPanelManager::runInstrument(int index, const MetricTensor& metric, const Event4D& location,
                                       const std::vector<Event4D>& trajectory)
{
    if (index < 0 || index >= static_cast<int>(m_instruments.size())) return;

    m_scanRunning = true;
    emit scanRunningChanged();

    auto findings = m_instruments[index]->analyze(metric, location, trajectory);
    for (auto& f : findings) {
        m_allFindings.push_back(f);
        emit newFindingDiscovered(
            QString::fromStdString(f.instrumentName),
            QString::fromStdString(f.description),
            f.confidence
        );
    }

    m_scanRunning = false;
    emit scanRunningChanged();
    emit findingsListChanged();
    emit findingsChanged();
    emit scanComplete();
}

void DiscoveryPanelManager::clearFindings()
{
    m_allFindings.clear();
    emit findingsListChanged();
    emit findingsChanged();
}

QString DiscoveryPanelManager::exportFindings() const
{
    QString result;
    for (const auto& f : m_allFindings) {
        result += QString("[%1] %2 (confidence: %3)\n")
            .arg(QString::fromStdString(f.instrumentName))
            .arg(QString::fromStdString(f.description))
            .arg(f.confidence);
    }
    return result;
}

void DiscoveryPanelManager::setActiveInstrumentIndex(int index)
{
    if (m_activeInstrumentIndex != index) {
        m_activeInstrumentIndex = index;
        emit activeInstrumentIndexChanged();
        emit activeInstrumentInfoChanged();
    }
}

QStringList DiscoveryPanelManager::instrumentNames() const
{
    QStringList names;
    for (const auto& inst : m_instruments) {
        names.append(QString::fromStdString(inst->getName()));
    }
    return names;
}

void DiscoveryPanelManager::setMetric(const std::shared_ptr<MetricTensor>& metric)
{
    m_metric = metric;
}

void DiscoveryPanelManager::setLocation(const Event4D& location)
{
    m_location = location;
    m_locationSet = true;
}

void DiscoveryPanelManager::startScan()
{
    if (!m_metric || !m_locationSet) return;
    auto trajectory = generateScanTrajectory();
    qWarning() << "DIAG startScan: metric=" << (m_metric ? "set" : "null")
               << "locationSet=" << m_locationSet
               << "trajectoryPoints=" << trajectory.size();
    runScan(*m_metric, m_location, trajectory);
}

std::vector<Event4D> DiscoveryPanelManager::generateScanTrajectory() const
{
    std::vector<Event4D> traj;
    const int N = 240;
    const double a = 5.0e5;     // semi-major axis (metres)
    const double e = 0.30;      // eccentricity
    const double p = a * (1.0 - e * e);
    const double precession = 0.18; // radians of perihelion advance per orbit
    const double twoPi = 2.0 * 3.141592653589793;
    traj.reserve(N);
    for (int i = 0; i < N; ++i) {
        double frac = static_cast<double>(i) / static_cast<double>(N - 1);
        double f = twoPi * frac;                 // true anomaly
        double r = p / (1.0 + e * std::cos(f));
        double phi = f + precession * frac;      // precessing azimuth
        double x = r * std::cos(phi);
        double y = r * std::sin(phi);
        // Monotonic timestamp so direction-based detectors register motion.
        Event4D ev(static_cast<double>(i), x, y, 0.0);
        traj.push_back(ev);
    }
    return traj;
}

void DiscoveryPanelManager::stopScan()
{
    m_scanRunning = false;
    emit scanRunningChanged();
    emit scanProgressChanged();
    emit findingsListChanged();
    emit findingsChanged();
    emit scanComplete();
    qWarning() << "DIAG runScan: totalFindings=" << m_allFindings.size()
               << "instruments=" << m_instruments.size();
}

void DiscoveryPanelManager::exportCurrentFindings()
{
    QString findings = exportFindings();
    qDebug() << "=== Discovery Findings Export ===" << findings;
}

QString severityToString(AlertSeverity sev) {
    switch (sev) {
        case AlertSeverity::INFO: return "INFO";
        case AlertSeverity::LOW: return "LOW";
        case AlertSeverity::MEDIUM: return "MEDIUM";
        case AlertSeverity::HIGH: return "HIGH";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

QVariantList DiscoveryPanelManager::findingsList() const
{
    QVariantList list;
    for (const auto& f : m_allFindings) {
        QVariantMap item;
        item["id"] = QString::fromStdString(f.id);
        item["instrumentName"] = QString::fromStdString(f.instrumentName);
        item["description"] = QString::fromStdString(f.description);
        item["confidence"] = f.confidence;
        item["severity"] = severityToString(f.severity);
        item["x"] = f.location.x;
        item["y"] = f.location.y;
        item["z"] = f.location.z;
        item["timestamp"] = f.timestamp;
        list.append(item);
    }
    return list;
}

QVariantMap DiscoveryPanelManager::activeInstrumentInfo() const
{
    QVariantMap info;
    auto* inst = instrument(m_activeInstrumentIndex);
    if (!inst) return info;

    info["name"] = QString::fromStdString(inst->getName());
    info["description"] = QString::fromStdString(inst->getDescription());
    info["category"] = QString::fromStdString(inst->getCategory());
    info["enabled"] = inst->isEnabled();
    info["totalFindings"] = static_cast<int>(inst->getTotalFindings());

    QVariantMap params;
    for (const auto& p : inst->getParameters()) {
        params[QString::fromStdString(p.first)] = p.second;
    }
    info["parameters"] = params;

    QVariantMap ranges;
    for (const auto& r : inst->getParameterRanges()) {
        QVariantMap range;
        range["min"] = r.second.first;
        range["max"] = r.second.second;
        ranges[QString::fromStdString(r.first)] = range;
    }
    info["parameterRanges"] = ranges;

    return info;
}

} // namespace quantumverse

#endif // QUANTUMVERSE_USE_QML