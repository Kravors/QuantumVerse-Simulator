/**
 * @file LIGOAdapter.h
 * @brief Minimal LIGO/Virgo gravitational-wave alert adapter
 *
 * Receives gravitational-wave candidate alerts and forwards them to a
 * subscriber callback. Alerts can be injected via simulateAlert() for testing
 * or (with a network backend) pushed from a live GCN/Kafka stream.
 */

#ifndef QUANTUMVERSE_LIGO_ADAPTER_H
#define QUANTUMVERSE_LIGO_ADAPTER_H

#include <string>
#include <vector>
#include <functional>

namespace quantumverse {

/**
 * @brief A gravitational-wave candidate alert
 */
struct GravitationalWaveAlert {
    std::string event_id;         ///< Event identifier (e.g. "GW250601A")
    double false_alarm_rate = 0.0;///< False-alarm rate (per year); lower => stronger
    double snr = 0.0;             ///< Network signal-to-noise ratio
    double m1 = 0.0;              ///< Primary mass (solar masses)
    double m2 = 0.0;              ///< Secondary mass (solar masses)
    double confidence = 0.0;      ///< Optional explicit confidence in [0,1]
};

/**
 * @brief Adapter that delivers GW alerts to a subscriber callback
 */
class LIGOAdapter {
public:
    using AlertCallback = std::function<void(const GravitationalWaveAlert&)>;

    /** @brief Register the callback invoked for each alert. */
    void setCallback(AlertCallback cb) { m_callback = std::move(cb); }

    /** @brief Push an alert to subscribers (used by tests and live ingest). */
    void simulateAlert(const GravitationalWaveAlert& alert) {
        m_received.push_back(alert);
        if (m_callback) m_callback(alert);
    }

    /** @brief Begin listening for live alerts (no-op stub without a network backend). */
    void start() {}

    /** @brief Alerts received so far. */
    const std::vector<GravitationalWaveAlert>& receivedAlerts() const { return m_received; }

private:
    AlertCallback m_callback;
    std::vector<GravitationalWaveAlert> m_received;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_LIGO_ADAPTER_H
