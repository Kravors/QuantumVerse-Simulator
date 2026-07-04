/**
 * @file DiscoveryPanelManager.h
 * @brief C++/QML bridge for the Discovery Console panel
 *
 * Manages discovery instruments and exposes them to QML for
 * the Discovery Console UI panel.
 *
 * References:
 * - plan9.md Phase 5: Discovery & AI Panels
 */

#ifndef QUANTUMVERSE_DISCOVERY_PANEL_MANAGER_H
#define QUANTUMVERSE_DISCOVERY_PANEL_MANAGER_H

// Qt headers MUST be included BEFORE the quantumverse namespace to prevent
// STL types from being incorrectly resolved as quantumverse::std::
#ifdef QUANTUMVERSE_USE_QML
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#endif

#include <memory>
#include <vector>
#include <map>
#include <cmath>
#include <string>
#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Manages discovery instruments and exposes data to QML
 *
 * Acts as the bridge between the C++ discovery system and the
 * QML Discovery Console panel. Provides instrument status,
 * findings, and controls for the UI.
 *
 * When QUANTUMVERSE_USE_QML is defined, this class inherits from
 * QObject and provides full signal/slot integration.
 * When Qt is unavailable, it is a plain C++ class with identical
 * public API but no Qt features.
 */
#ifdef QUANTUMVERSE_USE_QML

class DiscoveryPanelManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int activeInstrumentIndex READ activeInstrumentIndex WRITE setActiveInstrumentIndex NOTIFY activeInstrumentIndexChanged)
    Q_PROPERTY(int instrumentCount READ instrumentCount NOTIFY instrumentCountChanged)
    Q_PROPERTY(QStringList instrumentNames READ instrumentNames NOTIFY instrumentNamesChanged)
    Q_PROPERTY(bool scanRunning READ scanRunning NOTIFY scanRunningChanged)
    Q_PROPERTY(double scanProgress READ scanProgress NOTIFY scanProgressChanged)
    Q_PROPERTY(QVariantList findingsList READ findingsList NOTIFY findingsListChanged)
    Q_PROPERTY(QVariantMap activeInstrumentInfo READ activeInstrumentInfo NOTIFY activeInstrumentInfoChanged)

public:
    explicit DiscoveryPanelManager(QObject* parent = nullptr);
    ~DiscoveryPanelManager() override;

    // Register an instrument
    void registerInstrument(std::unique_ptr<DiscoveryInstrument> instrument);

    // Get instrument by index
    DiscoveryInstrument* instrument(int index);
    const DiscoveryInstrument* instrument(int index) const;

    // Run all active instruments
    void runScan(const MetricTensor& metric, const Event4D& location);

    // Run a specific instrument
    void runInstrument(int index, const MetricTensor& metric, const Event4D& location);

    // Clear all findings
    void clearFindings();

    // Export findings
    QString exportFindings() const;

    // Getters
    int activeInstrumentIndex() const { return m_activeInstrumentIndex; }
    int instrumentCount() const { return static_cast<int>(m_instruments.size()); }
    QStringList instrumentNames() const;
    bool scanRunning() const { return m_scanRunning; }
    double scanProgress() const { return m_scanProgress; }
    QVariantList findingsList() const;
    QVariantMap activeInstrumentInfo() const;

public slots:
    void setActiveInstrumentIndex(int index);
    void startScan();
    void stopScan();
    void exportCurrentFindings();

    // Set the metric for scan operations
    void setMetric(const std::shared_ptr<MetricTensor>& metric);
    void setLocation(const Event4D& location);

signals:
    void activeInstrumentIndexChanged();
    void instrumentCountChanged();
    void instrumentNamesChanged();
    void scanRunningChanged();
    void scanProgressChanged();
    void findingsListChanged();
    void activeInstrumentInfoChanged();
    void newFindingDiscovered(const QString& instrumentName, const QString& description, double confidence);
    void scanComplete();

private:
    std::vector<std::unique_ptr<DiscoveryInstrument>> m_instruments;
    int m_activeInstrumentIndex = 0;
    bool m_scanRunning = false;
    double m_scanProgress = 0.0;
    std::vector<InstrumentFinding> m_allFindings;
    std::shared_ptr<MetricTensor> m_metric;
    Event4D m_location;
    bool m_locationSet = false;
};

#else // QUANTUMVERSE_USE_QML not defined - plain C++ stub

class DiscoveryPanelManager
{
public:
    DiscoveryPanelManager() = default;
    ~DiscoveryPanelManager() = default;

    // Register an instrument
    void registerInstrument(std::unique_ptr<DiscoveryInstrument> instrument);

    // Get instrument by index
    DiscoveryInstrument* instrument(int index);
    const DiscoveryInstrument* instrument(int index) const;

    // Run all active instruments
    void runScan(const MetricTensor& metric, const Event4D& location);

    // Run a specific instrument
    void runInstrument(int index, const MetricTensor& metric, const Event4D& location);

    // Clear all findings
    void clearFindings();

    // Export findings (stub: returns empty string)
    std::string exportFindings() const;

    // Getters
    int activeInstrumentIndex() const { return m_activeInstrumentIndex; }
    int instrumentCount() const { return static_cast<int>(m_instruments.size()); }
    std::vector<std::string> instrumentNames() const;
    bool scanRunning() const { return m_scanRunning; }
    double scanProgress() const { return m_scanProgress; }

    // Scan control
     void startScan();
     void stopScan();
     void exportCurrentFindings();

     // Set the metric for scan operations
     void setMetric(const std::shared_ptr<MetricTensor>& metric);
     void setLocation(const Event4D& location);

private:
    std::vector<std::unique_ptr<DiscoveryInstrument>> m_instruments;
    int m_activeInstrumentIndex = 0;
    bool m_scanRunning = false;
    double m_scanProgress = 0.0;
    std::vector<InstrumentFinding> m_allFindings;
    std::shared_ptr<MetricTensor> m_metric;
    Event4D m_location;
    bool m_locationSet = false;
};

#endif // QUANTUMVERSE_USE_QML

} // namespace quantumverse

#endif // QUANTUMVERSE_DISCOVERY_PANEL_MANAGER_H