/**
 * @file FRBDispersionScanner.h
 * @brief Fast Radio Burst dispersion measure anomaly scanner
 */

#ifndef QUANTUMVERSE_FRB_DISPERSION_SCANNER_H
#define QUANTUMVERSE_FRB_DISPERSION_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Fast Radio Burst dispersion measure scanner
 *
 * Compares observed FRB dispersion measures (DM) against the expected
 * Galactic electron-density model.  Excess DM can indicate:
 *   - a new population of nearby repeating FRBs,
 *   - a previously unknown dense plasma screen, or
 *   - modified dispersion physics (e.g. axion-like photon coupling).
 *
 * Uses a reduced chi-squared test across the supplied trajectory of
 * DM measurements and flags trajectories whose scatter exceeds the
 * instrument threshold.
 */
class FRBDispersionScanner : public DiscoveryInstrument {
public:
    FRBDispersionScanner();
    ~FRBDispersionScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "FRBDispersionScanner"; }
    std::string getDescription() const override
    {
        return "Fast Radio Burst dispersion-measure scanner. "
               "Compares observed FRB DM to the Galactic electron-density "
               "model and flags excess scatter as potential new plasma "
               "structures or modified dispersion physics.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static double expectedGalacticDM(double gl, double gb);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FRB_DISPERSION_SCANNER_H
