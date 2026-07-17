/**
 * @file GWRingdownScanner.h
 * @brief Gravitational-wave ringdown quasi-normal mode scanner
 *
 * Tests the black hole no-hair theorem by comparing observed
 * ringdown quasi-normal mode (QNM) frequencies to the GR-predicted
 * spectrum for a Kerr black hole.  Deviations flag potential
 * exotic compact objects or modified gravity.
 *
 * References:
 * - plan5.md: GW Ringdown Scanner
 * - Detweiler (1980) - QNM frequencies for Kerr black holes
 * - Berti et al. (2009) - Gravitational-wave spectroscopy
 */

#ifndef QUANTUMVERSE_GW_RINGDOWN_SCANNER_H
#define QUANTUMVERSE_GW_RINGDOWN_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Gravitational-wave ringdown quasi-normal mode scanner
 *
 * Compares observed ringdown QNM frequencies to the GR-predicted
 * spectrum.  The fundamental (l=2, m=2, n=0) mode frequency for a
 * Schwarzschild black hole is approximately:
 *
 *   M ω_R ≈ 0.37367
 *   M ω_I ≈ -0.0890
 *
 * For a Kerr black hole with dimensionless spin a* = Jc/(GM²):
 *
 *   ω = ω_R + i ω_I
 *
 * where both real and imaginary parts depend on a*.
 *
 * The instrument flags ringdown events where the observed frequencies
 * deviate from the GR prediction by more than the configured sigma
 * threshold.
 */
class GWRingdownScanner : public DiscoveryInstrument
{
public:
    GWRingdownScanner();
    ~GWRingdownScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GWRingdownScanner"; }
    std::string getDescription() const override
    {
        return "GW ringdown scanner. Tests the black hole no-hair theorem by "
               "comparing observed ringdown quasi-normal mode frequencies to "
               "the GR-predicted Kerr spectrum. Flags deviations as potential "
               "exotic compact objects or modified gravity.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

private:
    static constexpr double kSigmaThreshold = 3.0;
    static constexpr double kMinTrajectorySize = 2;

    static double schwarzschildQNMReal();
    static double schwarzschildQNMImag();
    static double predictedFrequency(double mass, double spin);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GW_RINGDOWN_SCANNER_H
