/**
 * @file ECORingdownAnalyzer.h
 * @brief Exotic Compact Object (ECO) ringdown analyzer
 *
 * Searches post-merger gravitational-wave ringdown strain for the
 * hallmark of a horizonless compact object (boson star, gravastar,
 * fuzzball, wormhole, ...) instead of a Kerr black hole.
 *
 * A Kerr black hole produces a single, exponentially damped sinusoid
 * (the quasi-normal-mode ringdown).  An ECO lacks an event horizon, so
 * the outgoing wave repeatedly reflects between the angular-momentum
 * barrier and the ECO surface, producing a *train of echoes*: pulses
 * delayed and repeated at the object's light-crossing time (Cardoso et
 * al., 2016).  The analyzer fits the observed strain to both a pure
 * damped-sinusoid (BH) template and a BH+echo template, and flags an
 * ECO when the echo term explains a statistically significant fraction
 * of the ringdown variance.
 *
 * References:
 * - plan5.md: GW Ringdown Scanner (sibling instrument, Kerr QNM test)
 * - Cardoso, Franzin & Pani (2016) - echoes of transient signals
 * - arXiv:1602.07309 - gravitational-wave echoes
 */

#ifndef QUANTUMVERSE_ECO_RINGDOWN_ANALYZER_H
#define QUANTUMVERSE_ECO_RINGDOWN_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Exotic Compact Object (ECO) ringdown analyzer
 *
 * The input @p trajectory encodes the ringdown waveform as a series of
 * spacetime points where:
 *   - @c t  = observation time
 *   - @c x  = strain h(t)
 *
 * The analyzer estimates the ringdown period and damping time from the
 * strain autocorrelation, then scans candidate echo delays and reports
 * the fraction of ringdown variance explained by an additional delayed,
 * phase-shifted pulse (the ECO echo).  A finding is raised when that
 * fraction exceeds the configured @c echo_threshold.
 */
class ECORingdownAnalyzer : public DiscoveryInstrument {
public:
    ECORingdownAnalyzer();
    ~ECORingdownAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location, const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "ECORingdownAnalyzer"; }
    std::string getDescription() const override {
        return "ECO ringdown analyzer. Searches GW ringdown strain for the "
               "echo-train signature of a horizonless exotic compact object "
               "(boson star, gravastar, fuzzball). Flags ringdowns whose "
               "variance is significantly explained by a delayed, repeated "
               "pulse rather than a single Kerr quasi-normal mode.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

private:
    static constexpr size_t kMinPoints = 32;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ECO_RINGDOWN_ANALYZER_H
