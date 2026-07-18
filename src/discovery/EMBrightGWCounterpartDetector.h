/**
 * @file EMBrightGWCounterpartDetector.h
 * @brief EM-bright gravitational-wave counterpart detector
 *
 * Scans the recent multi-messenger trajectory for GW events that have
 * a plausible electromagnetic counterpart within a configurable
 * spatial/temporal window.  A counterpart is flagged when the angular
 * separation and time delay are consistent with a short GRB or kilonova.
 */

#ifndef QUANTUMVERSE_EM_BRIGHT_GW_COUNTERPART_DETECTOR_H
#define QUANTUMVERSE_EM_BRIGHT_GW_COUNTERPART_DETECTOR_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief EM-bright gravitational-wave counterpart detector
 *
 * Correlates GW triggers with EM transient positions in the recent
 * trajectory.  When a GW event and an EM event are found within the
 * configured angular and time windows, a counterpart finding is emitted.
 */
class EMBrightGWCounterpartDetector : public DiscoveryInstrument {
public:
    EMBrightGWCounterpartDetector();
    ~EMBrightGWCounterpartDetector() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "EMBrightGWCounterpartDetector"; }
    std::string getDescription() const override
    {
        return "EM-bright GW counterpart detector. "
               "Correlates GW triggers with EM transient positions and "
               "flags spatial/temporal coincidences consistent with short GRBs "
               "or kilonovae.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static double angularSeparation(const Event4D& a, const Event4D& b);
    static double timeDelaySec(const Event4D& gw, const Event4D& em);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_EM_BRIGHT_GW_COUNTERPART_DETECTOR_H
