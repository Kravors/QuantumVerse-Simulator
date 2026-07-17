/**
 * @file CosmicShearScanner.h
 * @brief Cosmic shear weak lensing anomaly detector
 */

#ifndef QUANTUMVERSE_COSMIC_SHEAR_SCANNER_H
#define QUANTUMVERSE_COSMIC_SHEAR_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Cosmic shear weak lensing anomaly detector
 *
 * Compares observed shear correlation functions to the ΛCDM prediction
 * and flags deviations in the convergence field.  Excess shear at a
 * given angular scale can indicate:
 *   - modified gravity,
 *   - massive neutrinos, or
 *   - undiscovered dark matter substructure.
 */
class CosmicShearScanner : public DiscoveryInstrument {
public:
    CosmicShearScanner();
    ~CosmicShearScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "CosmicShearScanner"; }
    std::string getDescription() const override
    {
        return "Cosmic shear weak lensing scanner. "
               "Compares observed shear correlation functions to the ΛCDM "
               "prediction and flags deviations in the convergence field as "
               "potential modified-gravity or dark-matter anomalies.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static double theoreticalShear(double theta);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_COSMIC_SHEAR_SCANNER_H
