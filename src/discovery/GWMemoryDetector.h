/**
 * @file GWMemoryDetector.h
 * @brief Detects excess permanent gravitational-wave strain displacement (memory effect)
 *
 * Analyzes GW strain memory trajectories for deviations from GR predictions.
 * Flags statistically significant (>3σ) permanent displacement as a potential anomaly.
 *
 * References:
 * - plan5.md: GW Memory Detector
 * - Christodoulou (1991) - GW memory effect
 */

#ifndef QUANTUMVERSE_GW_MEMORY_DETECTOR_H
#define QUANTUMVERSE_GW_MEMORY_DETECTOR_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects excess permanent gravitational-wave strain displacement
 *
 * Searches for deviations from GR-predicted GW memory amplitudes.
 * The memory effect is the permanent displacement of test masses after
 * a GW passes, predicted by GR and first measured in binary black hole mergers.
 */
class GWMemoryDetector : public DiscoveryInstrument
{
public:
    GWMemoryDetector();
    ~GWMemoryDetector() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GWMemoryDetector"; }
    std::string getDescription() const override
    {
        return "GW memory detector. Searches for excess permanent strain displacement "
               "deviating from GR-predicted memory amplitudes in merger events.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static constexpr double kSigmaThreshold = 3.0;
    static constexpr double kMinTrajectorySize = 2;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GW_MEMORY_DETECTOR_H
