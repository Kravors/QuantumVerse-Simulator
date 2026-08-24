/**
 * @file SGWBBackgroundAnalyzer.h
 * @brief Stochastic Gravitational-Wave Background (SGWB) analyzer
 *
 * Searches for the unresolved superposition of many weak gravitational-wave
 * sources - the stochastic gravitational-wave background (SGWB) - by measuring
 * the cross-correlation between two detectors (or between two streams of a
 * single instrument).
 *
 * For an isotropic, unpolarized, stationary background the two detectors share
 * a common strain component while their instrumental noises are independent.
 * The standard optimal-filter estimator therefore reduces, at zero lag, to the
 * normalized cross-power
 *     r0 = C / sqrt(Pa * Pb),
 * where C = sum a(t) b(t), Pa = sum a(t)^2, Pb = sum b(t)^2 (means removed,
 * optional window applied).  Under independent Gaussian noise the estimator
 * has standard deviation ~ 1/sqrt(N), so the detection significance is
 *     sigma = r0 * sqrt(N).
 * A finding is raised when sigma exceeds the configured @c threshold
 * (default 3.0 sigma), following the LIGO/Virgo/KAGRA stochastic-search
 * convention.
 *
 * References:
 * - Allen & Romano (1999) - detecting a stochastic background with two
 *   detectors (cross-correlation statistic)
 * - plan5.md: Gravitational-Wave instruments (sibling: GWEchoHunter,
 *   ECORingdownAnalyzer, GWMemoryDetector, GWRingdownScanner)
 */

#ifndef QUANTUMVERSE_SGWB_BACKGROUND_ANALYZER_H
#define QUANTUMVERSE_SGWB_BACKGROUND_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Stochastic Gravitational-Wave Background (SGWB) analyzer
 *
 * The primary, directly-testable search is @ref analyzeStrains, which
 * cross-correlates two strain time series.  The DiscoveryInstrument::analyze
 * override extracts a strain series from the @p trajectory and correlates it
 * either with a configured reference strain (set via @ref setReferenceStrain)
 * or, lacking one, with the trajectory's own second half (a conservative
 * single-detector self-cross-correlation).
 */
class SGWBBackgroundAnalyzer : public DiscoveryInstrument {
public:
    SGWBBackgroundAnalyzer();
    ~SGWBBackgroundAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location, const std::vector<Event4D>& trajectory = {}) override;

    /**
     * @brief Two-detector cross-correlation SGWB search.
     * @param strainA Strain time series from detector A.
     * @param strainB Strain time series from detector B.
     * @return Findings (non-empty when the cross-correlation significance
     *         exceeds the configured threshold).
     */
    std::vector<InstrumentFinding> analyzeStrains(
        const std::vector<double>& strainA,
        const std::vector<double>& strainB);

    /**
     * @brief Configure the reference strain series used by analyze().
     */
    void setReferenceStrain(const std::vector<double>& strain) { m_referenceStrain = strain; }

    std::string getName() const override { return "SGWBBackgroundAnalyzer"; }
    std::string getDescription() const override {
        return "Stochastic Gravitational-Wave Background (SGWB) analyzer. "
               "Cross-correlates two strain time series to detect the unresolved "
               "superposition of many weak gravitational-wave sources (an "
               "isotropic, unpolarized background). Flags a detection when the "
               "normalized cross-power exceeds the significance threshold.";
    }
    std::string getCategory() const override { return "Gravitational Waves"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

private:
    std::vector<double> m_referenceStrain;
    static constexpr size_t kMinPoints = 16;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SGWB_BACKGROUND_ANALYZER_H
