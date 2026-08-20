/**
 * @file GWEchoHunter.h
 * @brief Post-ringdown gravitational-wave echo hunter
 *
 * Searches GW ringdown strain for a delayed "echo": a secondary, damped
 * replica of the ringdown waveform arriving a characteristic time after the
 * main quasi-normal-mode ringdown. Such echoes are a predicted observational
 * signature of quantum structure near the horizon (Planck-scale corrections,
 * firewalls) or of exotic compact objects lacking a classical event horizon.
 *
 * References:
 * - Cardoso, Franzin & Pani (2016) - Is the gravitational-wave ringdown a
 *   probe of the event horizon? (arXiv:1602.07309)
 * - Abedi, Dykaar & Afshordi (2017) - Echoes from the abyss
 */

#ifndef QUANTUMVERSE_GW_ECHO_HUNTER_H
#define QUANTUMVERSE_GW_ECHO_HUNTER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Post-ringdown gravitational-wave echo hunter
 *
 * The instrument models the ringdown as a damped sinusoid, estimates the
 * exponential decay constant from the early ringdown window, extrapolates the
 * expected strain envelope to later times, then scans a delayed "echo window"
 * for a localized strain excess above that extrapolated tail. A significant
 * excess (observed/expected ratio above `echo_ratio_threshold`) is reported
 * as a potential horizon-echo anomaly.
 *
 * The decay constant is estimated from the ringdown envelope crests (local
 * maxima of |h|), which trace the true exponential envelope without the
 * oscillation bias of a point-wise fit.  An echo is reported when an
 * echo-window crest exceeds the extrapolated tail by `echo_ratio_threshold`.
 *
 * Convention: each trajectory sample is an Event4D whose `t` is the time
 * coordinate and `x` is the GW strain amplitude h(t).
 */
class GWEchoHunter : public DiscoveryInstrument
{
public:
    GWEchoHunter();
    ~GWEchoHunter() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GWEchoHunter"; }
    std::string getDescription() const override
    {
        return "GW echo hunter. Scans post-ringdown strain for a delayed, damped "
               "replica of the ringdown (horizon echoes), a predicted signature of "
               "quantum structure near the horizon or exotic compact objects.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

private:
    static constexpr double kMinTrajectorySize = 24;
    static constexpr double kMaxTau = 10.0;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GW_ECHO_HUNTER_H
