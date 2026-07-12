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

void DiscoveryPanelManager::runScan(const MetricTensor& metric, const Event4D& location)
{
    if (m_instruments.empty()) return;

    m_scanRunning = true;
    emit scanRunningChanged();

    m_allFindings.clear();

    for (size_t i = 0; i < m_instruments.size(); ++i) {
        if (!m_instruments[i]->isEnabled()) continue;

        auto findings = m_instruments[i]->analyze(metric, location);
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

void DiscoveryPanelManager::runInstrument(int index, const MetricTensor& metric, const Event4D& location)
{
    if (index < 0 || index >= static_cast<int>(m_instruments.size())) return;

    m_scanRunning = true;
    emit scanRunningChanged();

    auto findings = m_instruments[index]->analyze(metric, location);
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
    runScan(*m_metric, m_location);
}

void DiscoveryPanelManager::stopScan()
{
    m_scanRunning = false;
    emit scanRunningChanged();
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