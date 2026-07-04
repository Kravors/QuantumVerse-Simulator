/**
 * @file BosonStarCollisionPredictor.h
 * @brief Multi-messenger boson star collision analysis
 */

#ifndef QUANTUMVERSE_BOSON_STAR_COLLISION_PREDICTOR_H
#define QUANTUMVERSE_BOSON_STAR_COLLISION_PREDICTOR_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects boson star collision signatures in multi-messenger data
 *
 * Searches for gravitational-wave and electromagnetic signatures
 * consistent with boson star mergers, including mass-gap objects
 * and anomalous tidal deformability.
 */
class BosonStarCollisionPredictor : public DiscoveryInstrument
{
public:
    BosonStarCollisionPredictor();
    ~BosonStarCollisionPredictor() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "BosonStarCollisionPredictor"; }
    std::string getDescription() const override
    {
        return "Multi-messenger boson star collision analysis. Searches for "
               "gravitational-wave and electromagnetic signatures consistent with "
               "boson star mergers, including mass-gap objects and anomalous "
               "tidal deformability.";
    }
    std::string getCategory() const override { return "Compact Objects"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    double computeTidalDeformability(double compactness, double bosonMass);
    double estimateBosonMassFromRingdown(double fRing, double fDamp);
    bool isMassGapObject(double mass1, double mass2);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_BOSON_STAR_COLLISION_PREDICTOR_H