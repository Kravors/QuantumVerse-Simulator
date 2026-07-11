/**
 * @file DiscoveryInstrument.h
 * @brief Base class for all discovery instruments
 *
 * Each instrument is a specialised detector that transforms the
 * QuantumVerse simulator into a dedicated probe for a specific
 * observational window or theoretical prediction.
 *
 * References:
 * - plan5.md: Ten Previously Uncharted Instruments
 * - plan9.md Phase 5: Discovery & AI Panels
 */

#ifndef QUANTUMVERSE_DISCOVERY_INSTRUMENT_H
#define QUANTUMVERSE_DISCOVERY_INSTRUMENT_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

// Forward declaration for PlanetaryGridResult
struct PlanetaryGridResult;

/**
 * @brief Severity level for discovery alerts
 */
enum class AlertSeverity {
    INFO,       ///< Informational finding
    LOW,        ///< Low-significance anomaly
    MEDIUM,     ///< Moderate anomaly (3-5 sigma)
    HIGH,       ///< High-significance anomaly (5+ sigma)
    CRITICAL    ///< Potential discovery
};

/**
 * @brief A single finding from a discovery instrument
 */
struct InstrumentFinding {
    std::string id;
    std::string instrumentName;
    AlertSeverity severity = AlertSeverity::INFO;
    double confidence = 0.0;
    std::string description;
    std::map<std::string, double> parameters;
    Event4D location;
    double timestamp = 0.0;
};

/**
 * @brief Base class for all discovery instruments
 *
 * Each instrument monitors a specific astrophysical channel or
 * theoretical prediction and produces findings when anomalies
 * are detected.
 */
class DiscoveryInstrument {
public:
    virtual ~DiscoveryInstrument() = default;

    /**
     * @brief Get the unique name of this instrument
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get a human-readable description
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Get the category of this instrument
     */
    virtual std::string getCategory() const = 0;

    /**
     * @brief Run the instrument's analysis on current simulation state
     * @param metric Current metric tensor
     * @param location Location to analyze
     * @param trajectory Optional trajectory data
     * @return List of findings (may be empty)
     */
virtual std::vector<InstrumentFinding> analyze(
        const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}
    ) = 0;

    virtual std::vector<InstrumentFinding> analyzeGrid(
        const std::vector<PlanetaryGridResult>& gridResults) {
        return {};
    }

    virtual std::map<std::string, std::pair<double, double>> getParameterRanges() const {
        // Default implementation: return empty map
        // Derived classes should override to provide actual parameter ranges
        return {};
    }

    /**
     * @brief Set a parameter value
     */
    void setParameter(const std::string& name, double value) {
        m_parameters[name] = value;
    }

    /**
     * @brief Get current parameter values
     */
    const std::map<std::string, double>& getParameters() const {
        return m_parameters;
    }

    /**
     * @brief Get a specific parameter value by name
     * @param name Parameter name
     * @return Parameter value, or 0.0 if not found
     */
    virtual double getParameter(const std::string& name) const {
        auto it = m_parameters.find(name);
        return (it != m_parameters.end()) ? it->second : 0.0;
    }

    /**
     * @brief Get the last findings from this instrument
     */
    const std::vector<InstrumentFinding>& getLastFindings() const { return m_lastFindings; }

    /**
     * @brief Get total number of findings
     */
    size_t getTotalFindings() const { return m_totalFindings; }

    /**
     * @brief Check if instrument is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Enable/disable the instrument
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Get the default severity for findings from this instrument
     */
    virtual AlertSeverity getDefaultSeverity() const { return AlertSeverity::MEDIUM; }

    /**
     * @brief Get the alert severity threshold
     */
    AlertSeverity getAlertThreshold() const { return m_alertThreshold; }

    /**
     * @brief Set the alert severity threshold
     */
    void setAlertThreshold(AlertSeverity threshold) { m_alertThreshold = threshold; }

protected:
    /**
     * @brief Add a finding to the instrument's results
     */
    void addFinding(const InstrumentFinding& finding) {
        m_lastFindings.push_back(finding);
        if (m_lastFindings.size() > 100) m_lastFindings.erase(m_lastFindings.begin());
        m_totalFindings++;
    }

    /**
     * @brief Clear the last findings
     */
    void clearFindings() { m_lastFindings.clear(); }

    /**
     * @brief Helper: compute significance from confidence
     */
    static AlertSeverity confidenceToSeverity(double confidence) {
        if (confidence >= 0.99) return AlertSeverity::CRITICAL;
        if (confidence >= 0.95) return AlertSeverity::HIGH;
        if (confidence >= 0.80) return AlertSeverity::MEDIUM;
        if (confidence >= 0.50) return AlertSeverity::LOW;
        return AlertSeverity::INFO;
    }

    /**
     * @brief Helper: compute Gaussian significance (sigma)
     */
    static double confidenceToSigma(double confidence) {
        if (confidence >= 0.9999) return 5.0;
        if (confidence >= 0.999) return 4.0;
        if (confidence >= 0.99) return 3.5;
        if (confidence >= 0.975) return 3.0;
        if (confidence >= 0.95) return 2.5;
        if (confidence >= 0.80) return 2.0;
        if (confidence >= 0.50) return 1.0;
        return 0.0;
    }

private:
    bool m_enabled = true;
    AlertSeverity m_alertThreshold = AlertSeverity::MEDIUM;
    std::vector<InstrumentFinding> m_lastFindings;
    size_t m_totalFindings = 0;
    std::map<std::string, double> m_parameters;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DISCOVERY_INSTRUMENT_H