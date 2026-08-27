/**
 * @file HawkingRadiationDetector.h
 * @brief Hawking radiation signature detector
 */

#ifndef QUANTUMVERSE_HAWKING_RADIATION_DETECTOR_H
#define QUANTUMVERSE_HAWKING_RADIATION_DETECTOR_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects Hawking radiation signatures from black-hole spacetimes
 *
 * Evaluates the analytic Hawking relations in geometric units
 * (G = c = ħ = k_B = 1):
 *   - Temperature      T = 1 / (8πM)        (4D Schwarzschild, surface gravity κ = 1/(4M))
 *   - Entropy          S = A/4 = 4π M²       (Bekenstein–Hawking)
 *   - 2D linear-dilaton asymptotic flux  ⟨T₋₋⟩ = 1/48   (Giddings 1994)
 *
 * The black-hole (geometric) mass M is supplied via the "mass" parameter;
 * the metric/location provide context for the emission site. The detector is
 * intentionally metric-agnostic about mass (mirroring other discovery
 * instruments that configure their channel of interest via parameters).
 */
class HawkingRadiationDetector : public DiscoveryInstrument {
public:
    HawkingRadiationDetector();
    ~HawkingRadiationDetector() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric,
        const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "HawkingRadiationDetector"; }
    std::string getDescription() const override;
    std::string getCategory() const override { return "Black Holes"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /// @brief Hawking temperature T = 1/(8πM), M in geometric units (length).
    static double hawkingTemperature(double massGeometric);
    /// @brief Bekenstein–Hawking entropy S = A/4 = 4π M².
    static double hawkingEntropy(double massGeometric);
    /// @brief Hawking luminosity L = 4π r_s² σ T⁴ (σ = π²/60, geometric units).
    static double hawkingLuminosity(double massGeometric);
    /// @brief 2D linear-dilaton asymptotic flux ⟨T₋₋⟩ = 1/48.
    static double hawkingFlux2D();
};

} // namespace quantumverse

#endif // QUANTUMVERSE_HAWKING_RADIATION_DETECTOR_H
