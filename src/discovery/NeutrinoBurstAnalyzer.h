/**
 * @file NeutrinoBurstAnalyzer.h
 * @brief Multi-messenger neutrino-GW coincidence detector
 *
 * Detects neutrino burst events in temporal and spatial coincidence with
 * gravitational-wave triggers. Models the neutrino emission from core-collapse
 * supernovae and compact binary mergers, and flags multi-messenger candidates
 * when both channels fire within a configurable time window.
 *
 * References:
 * - Abbott et al. (2017) - Multi-messenger Observations of a Binary Neutron Star Merger
 * - IceCube Collaboration (2017) - Multimessenger observations of a flaring blazar
 * - Murase & Waxman (2016) - High energy neutrinos from astrophysical transients
 */

#ifndef QUANTUMVERSE_NEUTRINO_BURST_ANALYZER_H
#define QUANTUMVERSE_NEUTRINO_BURST_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects neutrino-GW coincidence events
 *
 * Searches for neutrino burst signatures in the GW strain trajectory's
 * post-merger phase. Models the neutrino luminosity from a compact binary
 * merger as a function of the remnant mass and accretion disk mass, then
 * checks for temporal coincidence with the GW trigger time.
 *
 * The neutrino emission model:
 *   L_nu ~ 10^52 erg/s * (M_disk / 0.01 M_sun) * (t / 1s)^(-5/3)
 *
 * Detection requires:
 *   1. Post-merger neutrino flux above energy threshold
 *   2. Temporal coincidence within time_window of GW trigger
 *   3. Consistent with astrophysical neutrino spectrum (power-law Gamma ~ 2-3)
 */
class NeutrinoBurstAnalyzer : public DiscoveryInstrument
{
public:
    NeutrinoBurstAnalyzer();
    ~NeutrinoBurstAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "NeutrinoBurstAnalyzer"; }
    std::string getDescription() const override
    {
        return "Multi-messenger neutrino-GW coincidence detector. Identifies "
               "neutrino burst signatures in temporal coincidence with GW "
               "triggers from compact binary mergers.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Neutrino luminosity from accretion disk (erg/s).
    static double neutrinoLuminosity(double diskMassMsun, double timeSec);

    /// @brief Neutrino energy flux at distance d (erg/s/cm^2).
    static double neutrinoFlux(double luminosity, double distanceMpc);

private:
    static constexpr size_t kMinSamples = 32;

    struct CoincidenceResult {
        double neutrinoLuminosity = 0.0;
        double neutrinoFlux = 0.0;
        double timeOffset = 0.0;
        double significance = 0.0;
        double peakTime = 0.0;
        double diskMass = 0.0;
    };

    static CoincidenceResult computeCoincidence(
        const std::vector<Event4D>& trajectory,
        double gwTriggerTime,
        double diskMassMsun,
        double distanceMpc,
        double timeWindowSec,
        double energyThreshold);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_NEUTRINO_BURST_ANALYZER_H
