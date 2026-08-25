/**
 * @file MergingBinaryInspiralAnalyzer.h
 * @brief Gravitational-wave inspiral chirp analyzer
 *
 * Detects and characterises binary inspiral signals in GW strain data
 * by tracking the frequency evolution f(t) and fitting to the TaylorF2
 * post-Newtonian chirp model to extract the chirp mass M_c and mass
 * ratio q.
 *
 * References:
 * - Sathyaprakash & Dhurandhar (1991) - Inspiral signal templates
 * - Buonanno et al. (2009) - Comparison of post-Newtonian templates
 * - TaylorF3.5PN phasing formula
 */

#ifndef QUANTUMVERSE_MERGING_BINARY_INSPIRAL_ANALYZER_H
#define QUANTUMVERSE_MERGING_BINARY_INSPIRAL_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects binary inspiral chirp signals in GW strain
 *
 * The TaylorF2 chirp frequency evolution:
 *
 *   f(t) = (1 / 8π) * (M_c)^(−5/8) * (t_c − t)^(−3/8)
 *
 * where M_c = (m1*m2)^(3/5) / (m1+m2)^(1/5) is the chirp mass and
 * t_c is the coalescence time. The instrument tracks the instantaneous
 * frequency via short-time Fourier analysis, fits to the chirp model,
 * and reports the recovered M_c, mass ratio q, and confidence.
 */
class MergingBinaryInspiralAnalyzer : public DiscoveryInstrument
{
public:
    MergingBinaryInspiralAnalyzer();
    ~MergingBinaryInspiralAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "MergingBinaryInspiralAnalyzer"; }
    std::string getDescription() const override
    {
        return "Merging binary inspiral analyzer. Detects GW chirp signals from "
               "inspiralling compact binaries, extracts chirp mass and mass ratio "
               "via TaylorF2 template fitting.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief TaylorF2 chirp frequency at time-to-coalescence tau (geometric units).
    static double taylorF2Frequency(double chirpMass, double tau);

private:
    static constexpr size_t kMinSamples = 32;

    struct ChirpFit {
        double chirpMass = 0.0;
        double massRatio = 0.0;
        double snr = 0.0;
        double confidence = 0.0;
        double coalescenceTime = 0.0;
    };

    static std::vector<std::pair<double, double>> trackInstantaneousFrequency(
        const std::vector<Event4D>& trajectory, size_t windowSize);

    static ChirpFit fitTaylorF2(const std::vector<std::pair<double, double>>& freqTrack);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_MERGING_BINARY_INSPIRAL_ANALYZER_H
