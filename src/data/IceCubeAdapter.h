/**
 * @file IceCubeAdapter.h
 * @brief Minimal IceCube neutrino alert adapter
 *
 * Receives high-energy neutrino candidate alerts and forwards them to a
 * subscriber callback. Alerts can be injected via simulateAlert() for testing
 * or (with a network backend) pushed from the IceCube alert stream.
 */

#ifndef QUANTUMVERSE_ICEUBE_ADAPTER_H
#define QUANTUMVERSE_ICEUBE_ADAPTER_H

#include <string>
#include <vector>
#include <functional>

namespace quantumverse {

/**
 * @brief A high-energy neutrino candidate alert
 */
struct NeutrinoAlert {
    std::string event_id;         ///< Event identifier (e.g. "IC250601A")
    double energy_tev = 0.0;      ///< Reconstructed energy (TeV)
    double false_alarm_rate = 0.0;///< False-alarm rate (per year); lower => stronger
    double ra = 0.0;              ///< Right ascension (degrees)
    double dec = 0.0;             ///< Declination (degrees)
    double confidence = 0.0;      ///< Optional explicit confidence in [0,1]
};

/**
 * @brief Adapter that delivers neutrino alerts to a subscriber callback
 */
class IceCubeAdapter {
public:
    using AlertCallback = std::function<void(const NeutrinoAlert&)>;

    /** @brief Register the callback invoked for each alert. */
    void setCallback(AlertCallback cb) { m_callback = std::move(cb); }

    /** @brief Push an alert to subscribers (used by tests and live ingest). */
    void simulateAlert(const NeutrinoAlert& alert) {
        m_received.push_back(alert);
        if (m_callback) m_callback(alert);
    }

    /** @brief Begin listening for live alerts (no-op stub without a network backend). */
    void start() {}

    /** @brief Alerts received so far. */
    const std::vector<NeutrinoAlert>& receivedAlerts() const { return m_received; }

private:
    AlertCallback m_callback;
    std::vector<NeutrinoAlert> m_received;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ICEUBE_ADAPTER_H
