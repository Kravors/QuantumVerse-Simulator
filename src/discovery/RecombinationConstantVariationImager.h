/**
 * @file RecombinationConstantVariationImager.h
 * @brief CMB power spectrum archaeologist for recombination constant variation
 */

#ifndef QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H
#define QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Searches for variation in fundamental constants at recombination
 *
 * Analyzes CMB power spectrum features for shifts in the recombination
 * redshift that would indicate time-varying fundamental constants
 * (alpha, electron mass, etc.) during the epoch of recombination.
 */
class RecombinationConstantVariationImager : public DiscoveryInstrument
{
public:
    RecombinationConstantVariationImager();
    ~RecombinationConstantVariationImager() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "RecombinationConstantVariationImager"; }
    std::string getDescription() const override
    {
        return "CMB power spectrum archaeologist. Searches for shifts in the "
               "recombination redshift and damping tail features that indicate "
               "time-varying fundamental constants (alpha, electron mass) "
               "during the epoch of recombination (z ~ 1100).";
    }
    std::string getCategory() const override { return "CMB Physics"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::CRITICAL; }

private:
    double computeRecombinationRedshift(double alphaOverAlpha0, double meffOverMe0);
    double computeDMultipoleShift(double deltaZrec, double lPeak);
    bool detectSpectralDistortion(const std::vector<double>& powerSpectrum,
        const std::vector<double>& multipoles);
    double constrainConstantVariation(double chi2, int dof);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H