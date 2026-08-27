/**
 * @file FastRadioBurstAnalyzer.h
 * @brief Fast Radio Burst (FRB) analyzer with GW coincidence detection
 *
 * Detects Fast Radio Burst signals in radio flux time series and searches
 * for temporal correlations with gravitational-wave triggers. Models the
 * dispersed pulse profile of FRBs and flags multi-messenger candidates
 * when a GW trigger falls within a configurable time window.
 *
 * References:
 * - Lorimer et al. (2007) - A bright millisecond radio burst of extragalactic origin
 * - CHIME/FRB Collaboration (2020) - A second source of repeating fast radio bursts
 * - Bochenek et al. (2020) - FRB 200428 associated with a Galactic magnetar
 */

#ifndef QUANTUMVERSE_FAST_RADIO_BURST_ANALYZER_H
#define QUANTUMVERSE_FAST_RADIO_BURST_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects FRB signals and correlates with GW triggers
 *
 * Searches for dispersed radio bursts in flux time series using a
 * signal-to-noise ratio threshold. When a burst is detected, checks
 * for temporal coincidence with a list of GW trigger times. Models
 * the frequency-dependent dispersion sweep characteristic of FRBs.
 *
 * The dispersion measure (DM) proxy is estimated from the pulse
 * width and frequency sweep rate. Higher DM indicates more
 * intervening free electrons (more distant or denser medium).
 */
class FastRadioBurstAnalyzer : public DiscoveryInstrument
{
public:
    FastRadioBurstAnalyzer();
    ~FastRadioBurstAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "FastRadioBurstAnalyzer"; }
    std::string getDescription() const override
    {
        return "Fast Radio Burst analyzer. Detects dispersed radio bursts "
               "and correlates with GW triggers for multi-messenger "
               "astronomy.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Signal-to-noise ratio of a burst peak.
    static double computeSNR(double peakFlux, double noiseSigma);

    /// @brief Dispersion measure proxy from pulse width and sweep rate.
    static double estimateDM(double pulseWidthSec, double freqSweepMHz);

private:
    static constexpr size_t kMinSamples = 32;

    struct BurstResult {
        double peakFlux = 0.0;
        double peakTime = 0.0;
        double snr = 0.0;
        double duration = 0.0;
        double dmProxy = 0.0;
        bool hasCoincidence = false;
        double gwTimeOffset = 0.0;
    };

    static BurstResult detectBurst(
        const std::vector<Event4D>& trajectory,
        double snrThreshold,
        const std::vector<double>& gwTriggerTimes,
        double timeWindowSec);

    static double estimateNoiseSigma(const std::vector<Event4D>& trajectory,
                                     double& medianFlux);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FAST_RADIO_BURST_ANALYZER_H
