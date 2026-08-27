/**
 * @file GravitationalWaveMemoryAnalyzer.h
 * @brief Christodoulou gravitational-wave memory effect analyzer
 *
 * Detects the non-oscillatory memory offset in GW strain data by comparing
 * the pre-burst baseline to the post-burst mean and matching against the
 * Christodoulou prediction:
 *
 *   Δh = (4G / c⁴r) × E_radiated × (1 / 2π)
 *
 * References:
 * - Christodoulou (1991) - Nonlinear nature of gravitation and gravitational-wave memory
 * - Favata (2010) - The gravitational-wave memory effect
 */

#ifndef QUANTUMVERSE_GRAVITATIONAL_WAVE_MEMORY_ANALYZER_H
#define QUANTUMVERSE_GRAVITATIONAL_WAVE_MEMORY_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects Christodoulou GW memory in strain trajectories
 *
 * Computes the permanent strain offset between pre-burst baseline and
 * post-burst late-time mean, then compares to the GR-predicted memory
 * amplitude from the Christodoulou formula. Flags detection when the
 * observed offset exceeds the expected noise level by a configurable
 * threshold (in sigma).
 */
class GravitationalWaveMemoryAnalyzer : public DiscoveryInstrument
{
public:
    GravitationalWaveMemoryAnalyzer();
    ~GravitationalWaveMemoryAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GravitationalWaveMemoryAnalyzer"; }
    std::string getDescription() const override
    {
        return "Christodoulou GW memory analyzer. Detects non-oscillatory memory "
               "offsets in strain data from compact binary mergers using the "
               "Christodoulou formula.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Christodoulou memory amplitude (simplified isotropic).
    static double christodoulouMemory(double energyRadiated, double distanceMpc);

private:
    static constexpr size_t kMinSamples = 32;

    struct MemoryResult {
        double observedOffset = 0.0;
        double expectedOffset = 0.0;
        double sigma = 0.0;
        double snr = 0.0;
        double baselineMean = 0.0;
        double postMean = 0.0;
    };

    static MemoryResult computeMemoryOffset(
        const std::vector<Event4D>& trajectory,
        double baselineDuration,
        double energyRadiated,
        double distanceMpc);

    static double estimateNoiseSigma(const std::vector<Event4D>& trajectory,
                                     double baselineMean,
                                     size_t baselineSamples);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GRAVITATIONAL_WAVE_MEMORY_ANALYZER_H
