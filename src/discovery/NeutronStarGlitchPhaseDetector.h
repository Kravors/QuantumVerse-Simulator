/**
 * @file NeutronStarGlitchPhaseDetector.h
 * @brief Quantum-vortex glitch statistics detector
 */

#ifndef QUANTUMVERSE_NEUTRON_STAR_GLITCH_PHASE_DETECTOR_H
#define QUANTUMVERSE_NEUTRON_STAR_GLITCH_PHASE_DETECTOR_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects neutron star glitch phase transitions via quantum-vortex statistics
 *
 * Analyzes pulsar timing residuals for glitch signatures consistent with
 * quantum-vortex unpinning in the neutron star interior. Identifies
 * phase transitions in the superfluid component.
 */
class NeutronStarGlitchPhaseDetector : public DiscoveryInstrument
{
public:
    NeutronStarGlitchPhaseDetector();
    ~NeutronStarGlitchPhaseDetector() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "NeutronStarGlitchPhaseDetector"; }
    std::string getDescription() const override
    {
        return "Quantum-vortex glitch statistics detector. Analyzes pulsar timing "
               "residuals for glitch signatures consistent with quantum-vortex "
               "unpinning in neutron star interiors, identifying superfluid "
               "phase transitions.";
    }
    std::string getCategory() const override { return "Neutron Stars"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

private:
    double computeVortexUnpinningProbability(double spinDownRate, double glitchSize);
    double estimateSuperfluidGap(double glitchRecurrenceTime);
    bool detectPhaseTransition(const std::vector<double>& glitchSizes);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_NEUTRON_STAR_GLITCH_PHASE_DETECTOR_H