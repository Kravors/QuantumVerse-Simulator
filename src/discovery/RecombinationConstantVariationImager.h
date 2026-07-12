/**
 * @file RecombinationConstantVariationImager.h
 * @brief CMB power spectrum imager for recombination constant variation
 */

#ifndef QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H
#define QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Searches for variation in the fine-structure constant at recombination
 *
 * Compares an observed CMB temperature power spectrum C_l against the standard
 * ΛCDM prediction. A systematic scaling of the observed spectrum relative to the
 * expectation is interpreted as a variation in the fine-structure constant,
 * Δα/α, imprinted at the epoch of recombination (z ~ 1100).
 *
 * Convention: each trajectory point carries the multipole ℓ in @c t and the
 * observed temperature power C_l in @c x.
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
        return "CMB recombination imager. Compares an observed CMB temperature "
               "power spectrum C_l against the standard ΛCDM prediction to detect "
               "a systematic scaling indicative of variation in the fine-structure "
               "constant (Δα/α) at the epoch of recombination (z ~ 1100).";
    }
    std::string getCategory() const override { return "CMB Physics"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

private:
    /**
     * @brief Smooth ΛCDM proxy for the temperature power spectrum at multipole ℓ.
     * @param l Multipole moment ℓ (must be positive).
     * @return Expected C_l ~ A / (ℓ (ℓ+1)); zero for ℓ <= 0.
     */
    static double expectedPowerSpectrum(double l);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_RECOMBINATION_CONSTANT_VARIATION_IMAGER_H
