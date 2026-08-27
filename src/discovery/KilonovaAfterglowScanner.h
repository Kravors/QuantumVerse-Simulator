/**
 * @file KilonovaAfterglowScanner.h
 * @brief Electromagnetic afterglow scanner for neutron star mergers
 *
 * Detects kilonova afterglow signatures in temporal coincidence with GW
 * triggers from binary neutron star (BNS) or neutron star-black hole (NSBH)
 * mergers. Models the r-process powered kilonova light curve and flags
 * candidates when the optical/IR transient rises within the expected
 * time window post-merger.
 *
 * References:
 * - Metzger (2017) - Kilonovae
 * - Abbott et al. (2017) - GW170817: Observation of Gravitational Waves...
 * - Kasen et al. (2017) - Origin of the binary neutron star merger GW170817
 */

#ifndef QUANTUMVERSE_KILONOVA_AFTERGLOW_SCANNER_H
#define QUANTUMVERSE_KILONOVA_AFTERGLOW_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects kilonova afterglow from compact binary mergers
 *
 * Models the kilonova light curve powered by r-process radioactive
 * heating of merger ejecta:
 *
 *   L(t) = L_peak * (t / t_peak)^alpha_rise   for t < t_peak
 *   L(t) = L_peak * (t / t_peak)^alpha_decay  for t >= t_peak
 *
 * where t_peak ~ 1 day, alpha_rise ~ 1-2, alpha_decay ~ -1.3 (r-process).
 * The peak luminosity depends on ejecta mass and opacity.
 */
class KilonovaAfterglowScanner : public DiscoveryInstrument
{
public:
    KilonovaAfterglowScanner();
    ~KilonovaAfterglowScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "KilonovaAfterglowScanner"; }
    std::string getDescription() const override
    {
        return "EM kilonova afterglow scanner. Detects optical/IR transient "
               "signatures from r-process powered kilonovae in temporal "
               "coincidence with GW triggers from neutron star mergers.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Kilonova luminosity at time t (erg/s).
    static double kilonovaLuminosity(double ejectaMassMsun, double timeDay);

    /// @brief Apparent magnitude at given distance.
    static double apparentMagnitude(double luminosityErgS, double distanceMpc);

private:
    static constexpr size_t kMinSamples = 32;

    struct AfterglowResult {
        double peakLuminosity = 0.0;
        double peakTime = 0.0;
        double magnitude = 0.0;
        double timeOffset = 0.0;
        double significance = 0.0;
        double ejectaMass = 0.0;
    };

    static AfterglowResult computeAfterglow(
        const std::vector<Event4D>& trajectory,
        double gwTriggerTime,
        double ejectaMassMsun,
        double distanceMpc,
        double timeWindowDay,
        double magLimit);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_KILONOVA_AFTERGLOW_SCANNER_H
