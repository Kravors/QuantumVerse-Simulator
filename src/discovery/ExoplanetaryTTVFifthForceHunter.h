/**
 * @file ExoplanetaryTTVFifthForceHunter.h
 * @brief Detects anomalous Transit Timing Variations indicating a fifth force
 *
 * Searches for deviations from Keplerian TTV curves that could indicate
 * a Yukawa-type fifth force between planets.
 */

#ifndef QUANTUMVERSE_EXOPLANETARY_TTV_FIFTH_FORCE_HUNTER_H
#define QUANTUMVERSE_EXOPLANETARY_TTV_FIFTH_FORCE_HUNTER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"
#include "PlanetaryGridGenerator.h"

namespace quantumverse {

/**
 * @brief Hunts for fifth-force signatures in exoplanetary TTV data
 *
 * Analyzes transit timing variations of exoplanets for deviations
 * from GR-predicted Keplerian motion. A statistically significant
 * residual pattern could indicate a Yukawa-type fifth force.
 *
 * References:
 * - plan5.md: Exoplanetary TTV Fifth-Force Hunter
 * - Classical fifth-force constraints from Solar System tests
 */
class ExoplanetaryTTVFifthForceHunter : public DiscoveryInstrument
{
public:
    ExoplanetaryTTVFifthForceHunter();
    ~ExoplanetaryTTVFifthForceHunter() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::vector<InstrumentFinding> analyzeGrid(const std::vector<PlanetaryGridResult>& gridResults);

    std::string getName() const override { return "ExoplanetaryTTVFifthForceHunter"; }
    std::string getDescription() const override
    {
        return "Detects anomalous Transit Timing Variations indicating a fifth force. "
               "Searches for deviations from Keplerian TTV curves that could indicate "
               "a Yukawa-type fifth force between planets.";
    }
    std::string getCategory() const override { return "Exoplanetary"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    double computeTTVResidual(double observedTransitTime, double predictedTransitTime,
        double orbitalPeriod, double semiMajorAxis);
    bool isAnomalousPattern(const std::vector<double>& residuals, double threshold);
    double estimateFifthForceStrength(const std::vector<double>& residuals,
        const std::vector<double>& distances);
    double estimateStrengthFromGrid(const std::vector<PlanetaryGridResult>& gridResults);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_EXOPLANETARY_TTV_FIFTH_FORCE_HUNTER_H