/**
 * @file CMBLensingScanner.h
 * @brief CMB lensing convergence map anomaly detector
 */

#ifndef QUANTUMVERSE_CMB_LENSING_SCANNER_H
#define QUANTUMVERSE_CMB_LENSING_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief CMB lensing reconstruction anomaly detector
 *
 * Processes a lensing convergence map (or CMB power-spectrum trajectory)
 * and searches for deviations from the ΛCDM prediction. Flags excess
 * small-scale power, non-Gaussian bispectrum peaks, or anomalous
 * smoothing that could indicate dark matter / neutrino-mass anomalies.
 */
class CMBLensingScanner : public DiscoveryInstrument {
public:
    CMBLensingScanner();
    ~CMBLensingScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "CMBLensingScanner"; }
    std::string getDescription() const override
    {
        return "CMB lensing convergence-map anomaly detector. "
               "Compares observed C_l to the ΛCDM prediction and flags "
               "excess small-scale power, bispectrum peaks, or anomalous "
               "smoothing as potential dark-matter / neutrino-mass anomalies.";
    }
    std::string getCategory() const override { return "CMB Physics"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static double theoreticalCl(double ell);
    static double lensingKernel(double ell);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CMB_LENSING_SCANNER_H
