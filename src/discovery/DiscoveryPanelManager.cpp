/**
 * @file DiscoveryPanelManager.cpp
 * @brief Implementation of the Discovery Panel Manager
 */

#include "DiscoveryPanelManager.h"
#include "DiscoveryEngine.h"
#include "data/AlertToFinding.h"
#ifdef QUANTUMVERSE_USE_QML
#include "data/MultiMessengerCorrelator.h"
#endif
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

#ifdef QUANTUMVERSE_USE_QML

#include <QDebug>
#include <fstream>
#include <QDateTime>
#include <QJsonObject>
#include "data/GCNNoticeParser.h"

namespace quantumverse {

DiscoveryPanelManager::DiscoveryPanelManager(QObject* parent)
    : QObject(parent)
    , m_activeInstrumentIndex(0)
    , m_scanRunning(false)
    , m_scanProgress(0.0)
{
#ifdef QUANTUMVERSE_USE_QML
    m_correlator = std::make_unique<MultiMessengerCorrelator>(this);
    connect(m_correlator.get(), &MultiMessengerCorrelator::correlationDetected,
            this, &DiscoveryPanelManager::onCorrelationDetected);
    connect(m_correlator.get(), &MultiMessengerCorrelator::followUpTriggered,
            this, &DiscoveryPanelManager::onFollowUpTriggered);
#endif
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

int DiscoveryPanelManager::correlationCount() const
{
#ifdef QUANTUMVERSE_USE_QML
    return m_correlator ? m_correlator->correlationCount() : 0;
#else
    return 0;
#endif
}

void DiscoveryPanelManager::onCorrelationDetected(const CorrelationEvent& correlation)
{
#ifdef QUANTUMVERSE_USE_QML
    emit correlationCountChanged();
    emit correlationsListChanged();
#endif
    Q_UNUSED(correlation);
}

void DiscoveryPanelManager::onFollowUpTriggered(const CorrelationEvent& correlation)
{
#ifdef QUANTUMVERSE_USE_QML
    emit liveAlertProcessed(QStringLiteral("GW+EM follow-up: %1").arg(correlation.description));
#endif
    Q_UNUSED(correlation);
}

QString DiscoveryPanelManager::severityToString(AlertSeverity sev) const
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

QVariantList DiscoveryPanelManager::correlationsList() const
{
#ifdef QUANTUMVERSE_USE_QML
    QVariantList list;
    if (!m_correlator) return list;
    for (const CorrelationEvent& ev : m_correlator->correlations()) {
        QVariantMap item;
        item["id"] = ev.id;
        item["messengers"] = ev.messengers;
        item["ra"] = ev.ra;
        item["dec"] = ev.dec;
        item["timestamp"] = ev.timestamp;
        item["combinedConfidence"] = ev.combinedConfidence;
        item["spatialScore"] = ev.spatialScore;
        item["description"] = ev.description;
        item["severity"] = severityToString(ev.severity);
        item["alertIds"] = ev.alertIds;
        list.append(item);
    }
    return list;
#else
    return {};
#endif
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
    {
        std::ofstream df("viewport_diag.log", std::ios::app);
        if (df) {
            df << QDateTime::currentMSecsSinceEpoch() << "  [DIAG-startScan] metric="
               << (m_metric ? "set" : "null")
               << " locationSet=" << m_locationSet
               << " trajectoryPoints=" << trajectory.size() << "\n";
        }
    }
    runScan(*m_metric, m_location, trajectory);
}

void DiscoveryPanelManager::ingestAlert(const QJsonObject& alertJson)
{
    const ParsedGCNNotice parsed = GCNNoticeParser::parse(alertJson);

    InstrumentFinding finding;
    finding.id = QStringLiteral("LIVE_%1").arg(parsed.raw_type).toStdString();
    finding.timestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    switch (parsed.origin) {
    case AlertOrigin::LIGO: {
        finding.instrumentName = "LIGO (Live)";
        finding.description = QStringLiteral("Live GW alert %1 (SNR=%2, FAR=%3)")
            .arg(QString::fromStdString(parsed.gw.event_id))
            .arg(parsed.gw.snr)
            .arg(parsed.gw.false_alarm_rate)
            .toStdString();
        finding.confidence = alertConfidence(parsed.gw.false_alarm_rate, parsed.gw.confidence);
        finding.severity = DiscoveryInstrument::confidenceToSeverity(finding.confidence);
        finding.location = Event4D(finding.timestamp, parsed.gw.ra, parsed.gw.dec, 0.0);
        finding.parameters["snr"] = parsed.gw.snr;
        finding.parameters["m1"] = parsed.gw.m1;
        finding.parameters["m2"] = parsed.gw.m2;
        break;
    }
    case AlertOrigin::IceCube: {
        finding.instrumentName = "IceCube (Live)";
        finding.description = QStringLiteral("Live neutrino alert %1 (E=%2 TeV, FAR=%3)")
            .arg(QString::fromStdString(parsed.neutrino.event_id))
            .arg(parsed.neutrino.energy_tev)
            .arg(parsed.neutrino.false_alarm_rate)
            .toStdString();
        finding.confidence = alertConfidence(parsed.neutrino.false_alarm_rate, parsed.neutrino.confidence);
        finding.severity = DiscoveryInstrument::confidenceToSeverity(finding.confidence);
        finding.location = Event4D(finding.timestamp, parsed.neutrino.ra, parsed.neutrino.dec, 0.0);
        finding.parameters["energy_tev"] = parsed.neutrino.energy_tev;
        break;
    }
    case AlertOrigin::TESS: {
        finding.instrumentName = "TESS (Live)";
        finding.description = QStringLiteral("Live TOI alert %1 (P=%2 d, depth=%3 ppm)")
            .arg(QString::fromStdString(parsed.tess.toi_id))
            .arg(parsed.tess.period_days)
            .arg(parsed.tess.depth_ppm)
            .toStdString();
        finding.confidence = parsed.tess.confidence;
        finding.severity = DiscoveryInstrument::confidenceToSeverity(finding.confidence);
        finding.location = Event4D(finding.timestamp, parsed.tess.ra, parsed.tess.dec, 0.0);
        finding.parameters["period_days"] = parsed.tess.period_days;
        finding.parameters["depth_ppm"] = parsed.tess.depth_ppm;
        finding.parameters["duration_hours"] = parsed.tess.duration_hours;
        break;
    }
    case AlertOrigin::FermiGBM: {
        finding.instrumentName = "Fermi GBM (Live)";
        finding.description = QStringLiteral("Live GRB alert %1 (duration=%2 s, flux=%3)")
            .arg(QString::fromStdString(parsed.fermi_gbm.trigger_id))
            .arg(parsed.fermi_gbm.duration)
            .arg(parsed.fermi_gbm.peak_flux)
            .toStdString();
        finding.confidence = alertConfidence(parsed.fermi_gbm.false_alarm_rate, parsed.fermi_gbm.confidence);
        finding.severity = DiscoveryInstrument::confidenceToSeverity(finding.confidence);
        finding.location = Event4D(finding.timestamp, parsed.fermi_gbm.ra, parsed.fermi_gbm.dec, 0.0);
        finding.parameters["duration"] = parsed.fermi_gbm.duration;
        finding.parameters["peak_flux"] = parsed.fermi_gbm.peak_flux;
        finding.parameters["error_radius"] = parsed.fermi_gbm.error_radius;
        break;
    }
    case AlertOrigin::Swift: {
        finding.instrumentName = "Swift BAT (Live)";
        finding.description = QStringLiteral("Live X-ray transient %1 (BAT rate=%2, XRT flux=%3)")
            .arg(QString::fromStdString(parsed.swift_bat.trigger_id))
            .arg(parsed.swift_bat.bat_rate)
            .arg(parsed.swift_bat.xrt_flux)
            .toStdString();
        finding.confidence = alertConfidence(parsed.swift_bat.false_alarm_rate, parsed.swift_bat.confidence);
        finding.severity = DiscoveryInstrument::confidenceToSeverity(finding.confidence);
        finding.location = Event4D(finding.timestamp, parsed.swift_bat.ra, parsed.swift_bat.dec, 0.0);
        finding.parameters["bat_rate"] = parsed.swift_bat.bat_rate;
        finding.parameters["xrt_flux"] = parsed.swift_bat.xrt_flux;
        finding.parameters["error_radius"] = parsed.swift_bat.error_radius;
        break;
    }
    default:
        finding.instrumentName = "Unknown (Live)";
        finding.description = QStringLiteral("Unsupported alert type: %1").arg(parsed.raw_type).toStdString();
        finding.severity = AlertSeverity::INFO;
        finding.confidence = 0.0;
        break;
    }

    m_allFindings.push_back(finding);
    emit newFindingDiscovered(
        QString::fromStdString(finding.instrumentName),
        QString::fromStdString(finding.description),
        finding.confidence
    );
    emit findingsListChanged();
    emit findingsChanged();
    emit liveAlertProcessed(QString::fromStdString(finding.id));

    if (runAnomalyDetection(finding)) {
        if (!m_allFindings.empty()) {
            m_allFindings.back().isAnomaly = true;
        }
        emit findingsChanged();
    }

#ifdef QUANTUMVERSE_USE_QML
    if (m_correlator) {
        m_correlator->addAlert(finding);
    }
#endif
}

bool DiscoveryPanelManager::runAnomalyDetection(const InstrumentFinding& finding)
{
    if (!m_anomalyDetectionEnabled) return false;

    DiscoveryResult result = m_engine.detectAnomaly(
        finding.location,
        m_metric ? *m_metric : MetricTensor(),
        {}
    );

    if (result.confidence > 0.8) {
        QJsonObject anomaly;
        anomaly["id"] = QString::fromStdString(result.id);
        anomaly["type"] = QString::fromStdString(result.type);
        anomaly["description"] = QString::fromStdString(result.description);
        anomaly["confidence"] = result.confidence;
        anomaly["validated"] = result.validated;
        emit anomalyDetected(anomaly);
        return true;
    }
    return false;
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

// Plain C++ implementations when Qt is unavailable
#ifndef QUANTUMVERSE_USE_QML

namespace quantumverse {

void DiscoveryPanelManager::registerInstrument(std::unique_ptr<DiscoveryInstrument> instrument) {
    m_instruments.push_back(std::move(instrument));
}

DiscoveryInstrument* DiscoveryPanelManager::instrument(int index) {
    if (index >= 0 && index < static_cast<int>(m_instruments.size())) {
        return m_instruments[index].get();
    }
    return nullptr;
}

const DiscoveryInstrument* DiscoveryPanelManager::instrument(int index) const {
    if (index >= 0 && index < static_cast<int>(m_instruments.size())) {
        return m_instruments[index].get();
    }
    return nullptr;
}

void DiscoveryPanelManager::runScan(const MetricTensor& metric, const Event4D& location) {
    m_allFindings.clear();
    for (auto& inst : m_instruments) {
        if (!inst->isEnabled()) continue;
        auto findings = inst->analyze(metric, location);
        m_allFindings.insert(m_allFindings.end(), findings.begin(), findings.end());
    }
}

void DiscoveryPanelManager::runInstrument(int index, const MetricTensor& metric, const Event4D& location) {
    if (index < 0 || index >= static_cast<int>(m_instruments.size())) return;
    auto* inst = m_instruments[index].get();
    if (!inst->isEnabled()) return;
    auto findings = inst->analyze(metric, location);
    m_allFindings.insert(m_allFindings.end(), findings.begin(), findings.end());
}

void DiscoveryPanelManager::clearFindings() {
    m_allFindings.clear();
}

std::string DiscoveryPanelManager::exportFindings() const {
    return {};
}

std::vector<std::string> DiscoveryPanelManager::instrumentNames() const {
    std::vector<std::string> names;
    names.reserve(m_instruments.size());
    for (const auto& inst : m_instruments) {
        names.push_back(inst->getName());
    }
    return names;
}

void DiscoveryPanelManager::startScan() {
    m_scanRunning = true;
}

void DiscoveryPanelManager::stopScan() {
    m_scanRunning = false;
}

void DiscoveryPanelManager::exportCurrentFindings() {
    // Stub
}

void DiscoveryPanelManager::setMetric(const std::shared_ptr<MetricTensor>& metric) {
    m_metric = metric;
}

void DiscoveryPanelManager::setLocation(const Event4D& location) {
    m_location = location;
    m_locationSet = true;
}

} // namespace quantumverse

#endif // !QUANTUMVERSE_USE_QML