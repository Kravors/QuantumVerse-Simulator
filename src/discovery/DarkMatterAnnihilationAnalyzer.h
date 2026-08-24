/**
 * @file DarkMatterAnnihilationAnalyzer.h
 * @brief Dark Matter Annihilation Signal analyzer
 *
 * Searches a photon/gamma-ray energy spectrum (e.g. from Fermi-LAT, CTA,
 * AMS-02) for the spectral signature of dark matter annihilation: a
 * monochromatic gamma-ray line at the dark-matter mass, or a continuum
 * excess, superimposed on a smooth astrophysical background.
 *
 * The analyzer fits a smooth power-law background N(E) = A * E^(-gamma) to
 * the spectrum (excluding a candidate line region), then measures the excess
 * counts in that region.  The detection significance is the excess divided by
 * the Poisson uncertainty of the background in the region,
 *     sigma = excess / sqrt(sum expected),
 * flagged when sigma exceeds the configured @c significance_threshold
 * (default 3.0 sigma) - the standard line-search convention.
 *
 * The DiscoveryInstrument::analyze override scans a configurable mass range
 * for the most significant line; the directly-testable core is
 * @ref analyzeSpectrum.  This adds a particle-astrophysics channel
 * complementing the gravitational-wave instruments.
 *
 * References:
 * - plan5.md: discovery instruments (sibling: SGWBBackgroundAnalyzer,
 *   ECORingdownAnalyzer, GWEchoHunter)
 * - Fermi-LAT / CTA dark-matter line searches
 */

#ifndef QUANTUMVERSE_DARK_MATTER_ANNIHILATION_ANALYZER_H
#define QUANTUMVERSE_DARK_MATTER_ANNIHILATION_ANALYZER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Dark Matter Annihilation Signal analyzer
 *
 * The primary, directly-testable search is @ref analyzeSpectrum, which looks
 * for a line at a specific candidate @p lineEnergy.  The analyze() override
 * scans the @c mass_min .. @c mass_max range and reports the most significant
 * candidate.
 */
class DarkMatterAnnihilationAnalyzer : public DiscoveryInstrument {
public:
    DarkMatterAnnihilationAnalyzer();
    ~DarkMatterAnnihilationAnalyzer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location, const std::vector<Event4D>& trajectory = {}) override;

    /**
     * @brief Search for a monochromatic line at @p lineEnergy in the spectrum.
     * @param energies Energy of each spectral bin (must be > 0).
     * @param counts   Photon counts in each bin (>= 0).
     * @param lineEnergy Candidate line energy to test.
     * @return Findings (non-empty when excess significance exceeds threshold).
     */
    std::vector<InstrumentFinding> analyzeSpectrum(
        const std::vector<double>& energies,
        const std::vector<double>& counts,
        double lineEnergy);

    std::string getName() const override { return "DarkMatterAnnihilationAnalyzer"; }
    std::string getDescription() const override {
        return "Dark Matter Annihilation Signal analyzer. Searches a photon/"
               "gamma-ray energy spectrum for the monochromatic line (or "
               "continuum excess) produced by dark-matter annihilation, by "
               "fitting a smooth power-law background and flagging a localized "
               "count excess whose significance exceeds the threshold.";
    }
    std::string getCategory() const override { return "Particle Astrophysics"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

private:
    /// Significance (excess / sqrt(bg)) of a line at @p lineEnergy, or 0.
    double computeLineSignificance(const std::vector<double>& energies,
        const std::vector<double>& counts, double lineEnergy, double widthFrac) const;

    static constexpr size_t kMinBins = 16;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DARK_MATTER_ANNIHILATION_ANALYZER_H
