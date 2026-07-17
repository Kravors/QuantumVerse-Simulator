/**
 * @file SwiftBATAdapter.h
 * @brief Minimal Swift Burst Alert Telescope alert adapter
 *
 * Receives Swift BAT gamma-ray burst / X-ray transient alerts and forwards
 * them to a subscriber callback.  Alerts can be injected via simulateAlert()
 * for testing or pushed from a live GCN/Kafka Swift BAT stream.
 */

#ifndef QUANTUMVERSE_SWIFT_BAT_ADAPTER_H
#define QUANTUMVERSE_SWIFT_BAT_ADAPTER_H

#include <string>
#include <vector>
#include <functional>

namespace quantumverse {

/**
 * @brief A Swift BAT gamma-ray burst / X-ray transient alert
 */
struct SwiftBATAlert {
    std::string trigger_id;        ///< GCN trigger identifier
    double bat_rate = 0.0;         ///< BAT count rate (counts/s)
    double xrt_flux = 0.0;         ///< XRT afterglow flux (erg/cm^2/s)
    double duration = 0.0;         ///< Burst duration (seconds)
    double false_alarm_rate = 0.0; ///< False-alarm rate; lower => stronger
    double ra = 0.0;               ///< Right ascension (degrees)
    double dec = 0.0;              ///< Declination (degrees)
    double error_radius = 0.0;     ///< Localisation error radius (degrees)
    double confidence = 0.0;       ///< Pipeline confidence in [0,1]
};

/**
 * @brief Adapter that delivers Swift BAT alerts to a subscriber callback
 */
class SwiftBATAdapter
{
public:
    using AlertCallback = std::function<void(const SwiftBATAlert&)>;

    /** @brief Register the callback invoked for each alert. */
    void setCallback(AlertCallback cb) { m_callback = std::move(cb); }

    /** @brief Push an alert to subscribers (used by tests and live ingest). */
    void simulateAlert(const SwiftBATAlert& alert) {
        m_received.push_back(alert);
        if (m_callback) m_callback(alert);
    }

    /** @brief Begin listening for live alerts (no-op stub without a network backend). */
    void start() {}

    /** @brief Alerts received so far. */
    const std::vector<SwiftBATAlert>& receivedAlerts() const { return m_received; }

private:
    AlertCallback m_callback;
    std::vector<SwiftBATAlert> m_received;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SWIFT_BAT_ADAPTER_H
