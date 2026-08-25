/**
 * @file KerrNoHairViolationAnalyzer.h
 * @brief Kerr no-hair theorem violation analyzer via QNM spectroscopy
 *
 * Extracts quasinormal mode (QNM) frequencies from a ringdown strain
 * waveform and tests the Kerr no-hair theorem: for a true Kerr black
 * hole all QNMs are determined by exactly two parameters (M, J).  The
 * instrument measures the dominant (l=m=2, n=0) and first overtone
 * (l=m=2, n=1) frequencies, infers the implied (M, J) from each, and
 * flags a violation when the two inferences disagree beyond the
 * configured sigma threshold.
 *
 * References:
 * - Berti, Cardoso & Will (2006) - On gravitational-wave spectroscopy
 *   of black holes (arXiv:gr-qc/0601079)
 * - Berti et al. (2009) - Gravitational-wave spectroscopy
 */

#ifndef QUANTUMVERSE_KERR_NO_HAIR_VIOLATION_ANALYZER_H
#define QUANTUMVERSE_KERR_NO_HAIR_VIOLATION_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Tests the Kerr no-hair theorem via QNM spectroscopy
 *
 * The fundamental (220) and first overtone (221) QNM frequencies for
 * a Kerr black hole satisfy:
 *
 *   M ω_220 ≈ f_R(a*) + i f_I(a*)
 *   M ω_221 ≈ g_R(a*) + i g_I(a*)
 *
 * where a* = Jc/(GM²) is the dimensionless spin.  For a true Kerr BH
 * both modes imply the same (M, a*).  A statistically significant
 * disagreement flags a potential no-hair violation.
 */
class KerrNoHairViolationAnalyzer : public DiscoveryInstrument
{
public:
    KerrNoHairViolationAnalyzer();
    ~KerrNoHairViolationAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "KerrNoHairViolationAnalyzer"; }
    std::string getDescription() const override
    {
        return "Kerr no-hair violation analyzer. Extracts QNM frequencies from "
               "ringdown strain and tests whether multiple modes imply the same "
               "(M, J). Flags disagreements as potential no-hair violations.";
    }
    std::string getCategory() const override { return "Black Holes"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Kerr (220) QNM real part: Berti et al. (2006) fit.
    static double kerrQNM220Real(double spin);
    /// @brief Kerr (220) QNM imaginary part: Berti et al. (2006) fit.
    static double kerrQNM220Imag(double spin);
    /// @brief Kerr (221) QNM real part: Berti et al. (2006) fit.
    static double kerrQNM221Real(double spin);
    /// @brief Kerr (221) QNM imaginary part: Berti et al. (2006) fit.
    static double kerrQNM221Imag(double spin);

private:
    static constexpr double kMinSamples = 16;

    struct QNMResult {
        double freqReal = 0.0;
        double freqImag = 0.0;
        double amplitude = 0.0;
    };

    static QNMResult extractDominantMode(const std::vector<Event4D>& trajectory,
                                         double dt);
    static double inferSpinFrom220(double mOmegaReal);
    static double inferMassFromQNM(double mOmegaReal, double spin,
                                   double (*qnmRealFunc)(double));
};

} // namespace quantumverse

#endif // QUANTUMVERSE_KERR_NO_HAIR_VIOLATION_ANALYZER_H
