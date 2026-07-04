/**
 * @file GalacticTidalStreamCartographer.h
 * @brief Dark matter subhalo finder via stellar streams
 */

#ifndef QUANTUMVERSE_GALACTIC_TIDAL_STREAM_CARTOGRAPHER_H
#define QUANTUMVERSE_GALACTIC_TIDAL_STREAM_CARTOGRAPHER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Maps dark matter subhalo distribution via tidal stream perturbations
 *
 * Analyzes stellar stream morphology to detect gravitational perturbations
 * from dark matter subhalos, mapping the local dark matter distribution.
 */
class GalacticTidalStreamCartographer : public DiscoveryInstrument
{
public:
    GalacticTidalStreamCartographer();
    ~GalacticTidalStreamCartographer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "GalacticTidalStreamCartographer"; }
    std::string getDescription() const override
    {
        return "Dark matter subhalo finder via stellar streams. Maps dark matter "
               "subhalo distribution by analyzing gravitational perturbations in "
               "stellar stream morphology.";
    }
    std::string getCategory() const override { return "Galactic"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

private:
    double computeStreamWidth(double distance, double velocityDispersion);
    double estimateSubhaloMass(double streamDeflection, double distance);
    bool detectGapStructure(const std::vector<double>& streamDensities);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GALACTIC_TIDAL_STREAM_CARTOGRAPHER_H