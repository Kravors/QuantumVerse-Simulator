/**
 * @file TESSAlertAdapter.h
 * @brief Minimal TESS transiting-exoplanet alert adapter
 *
 * Receives TESS Objects of Interest (TOI) alerts and forwards them to a
 * subscriber callback.  Alerts can be injected via simulateAlert() for
 * testing or pushed from a live GCN/Kafka TESS stream.
 */

#ifndef QUANTUMVERSE_TESS_ALERT_ADAPTER_H
#define QUANTUMVERSE_TESS_ALERT_ADAPTER_H

#include <string>
#include <vector>
#include <functional>

namespace quantumverse {

/**
 * @brief A TESS transiting-exoplanet candidate alert
 */
struct TESSAlert {
    std::string toi_id;           ///< TESS Object of Interest identifier
    double period_days = 0.0;     ///< Orbital period (days)
    double depth_ppm = 0.0;       ///< Transit depth (parts per million)
    double duration_hours = 0.0;  ///< Transit duration (hours)
    double confidence = 0.0;      ///< Pipeline confidence in [0,1]
    double ra = 0.0;              ///< Right ascension (degrees)
    double dec = 0.0;             ///< Declination (degrees)
};

/**
 * @brief Adapter that delivers TESS TOI alerts to a subscriber callback
 */
class TESSAlertAdapter
{
public:
    using AlertCallback = std::function<void(const TESSAlert&)>;

    /** @brief Register the callback invoked for each alert. */
    void setCallback(AlertCallback cb) { m_callback = std::move(cb); }

    /** @brief Push an alert to subscribers (used by tests and live ingest). */
    void simulateAlert(const TESSAlert& alert) {
        m_received.push_back(alert);
        if (m_callback) m_callback(alert);
    }

    /** @brief Begin listening for live alerts (no-op stub without a network backend). */
    void start() {}

    /** @brief Alerts received so far. */
    const std::vector<TESSAlert>& receivedAlerts() const { return m_received; }

private:
    AlertCallback m_callback;
    std::vector<TESSAlert> m_received;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_TESS_ALERT_ADAPTER_H
