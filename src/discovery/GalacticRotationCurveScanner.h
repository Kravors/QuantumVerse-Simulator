/**
 * @file GalacticRotationCurveScanner.h
 * @brief Scans for universal residual patterns indicating modified gravity
 */

#ifndef QUANTUMVERSE_GALACTIC_ROTATION_CURVE_SCANNER_H
#define QUANTUMVERSE_GALACTIC_ROTATION_CURVE_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Universal rotation curve residual scanner for modified gravity
 *
 * Compares observed galactic rotation curves against GR+NFW predictions.
 * Identifies systematic residuals that could indicate modified gravity
 * (MOND, TeVeS, f(R)) or dark matter substructure.
 */
class GalacticRotationCurveScanner : public DiscoveryInstrument
{
public:
    GalacticRotationCurveScanner();
    ~GalacticRotationCurveScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GalacticRotationCurveScanner"; }
    std::string getDescription() const override
    {
        return "Universal residual scanner for modified gravity. Compares rotation "
               "curve observations against GR+NFW predictions to detect MOND, TeVeS, "
               "or f(R) signatures.";
    }
    std::string getCategory() const override { return "Galactic"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

private:
    double computeNFWPrediction(double radius, double virialMass, double concentration);
    double computeMONDAcceleration(double newtonianAccel, double a0);
    bool detectSystematicResidual(const std::vector<double>& residuals);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GALACTIC_ROTATION_CURVE_SCANNER_H