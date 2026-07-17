/**
 * @file FermiGBMAdapter.h
 * @brief Minimal Fermi Gamma-ray Burst Monitor alert adapter
 *
 * Receives Fermi GBM GRB candidate alerts and forwards them to a
 * subscriber callback. Alerts can be injected via simulateAlert() for
 * testing or pushed from a live GCN/Kafka Fermi GBM stream.
 */

#ifndef QUANTUMVERSE_FERMI_GBM_ADAPTER_H
#define QUANTUMVERSE_FERMI_GBM_ADAPTER_H

#include <string>
#include <vector>
#include <functional>

namespace quantumverse {

/**
 * @brief A Fermi GBM gamma-ray burst candidate alert
 */
struct FermiGBMAlert {
    std::string trigger_id;        ///< GBM trigger identifier (e.g. "bn240512001")
    double duration = 0.0;         ///< Burst duration (seconds)
    double peak_flux = 0.0;        ///< Peak photon flux (erg/cm^2/s)
    double false_alarm_rate = 0.0; ///< False-alarm rate; lower => stronger
    double ra = 0.0;               ///< Right ascension (degrees)
    double dec = 0.0;              ///< Declination (degrees)
    double error_radius = 0.0;     ///< Localisation error radius (degrees)
    double confidence = 0.0;       ///< Pipeline confidence in [0,1]
};

/**
 * @brief Adapter that delivers Fermi GBM GRB alerts to a subscriber callback
 */
class FermiGBMAdapter
{
public:
    using AlertCallback = std::function<void(const FermiGBMAlert&)>;

    /** @brief Register the callback invoked for each alert. */
    void setCallback(AlertCallback cb) { m_callback = std::move(cb); }

    /** @brief Push an alert to subscribers (used by tests and live ingest). */
    void simulateAlert(const FermiGBMAlert& alert) {
        m_received.push_back(alert);
        if (m_callback) m_callback(alert);
    }

    /** @brief Begin listening for live alerts (no-op stub without a network backend). */
    void start() {}

    /** @brief Alerts received so far. */
    const std::vector<FermiGBMAlert>& receivedAlerts() const { return m_received; }

private:
    AlertCallback m_callback;
    std::vector<FermiGBMAlert> m_received;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FERMI_GBM_ADAPTER_H
