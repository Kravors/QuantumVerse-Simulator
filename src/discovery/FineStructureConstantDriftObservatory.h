/**
 * @file FineStructureConstantDriftObservatory.h
 * @brief Quasar absorption spectrography for alpha-drift detection
 */

#ifndef QUANTUMVERSE_FINE_STRUCTURE_CONSTANT_DRIFT_OBSERVATORY_H
#define QUANTUMVERSE_FINE_STRUCTURE_CONSTANT_DRIFT_OBSERVATORY_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Searches for spatial/temporal drift in the fine-structure constant
 *
 * Uses quasar absorption line measurements at various redshifts to
 * constrain variations in alpha over cosmic time, as predicted by
 * scalar-tensor theories and string-inspired models.
 */
class FineStructureConstantDriftObservatory : public DiscoveryInstrument
{
public:
    FineStructureConstantDriftObservatory();
    ~FineStructureConstantDriftObservatory() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "FineStructureConstantDriftObservatory"; }
    std::string getDescription() const override
    {
        return "Quasar absorption spectrography for alpha-drift detection. "
               "Searches for spatial/temporal variation in the fine-structure "
               "constant using absorption line measurements at various redshifts.";
    }
    std::string getCategory() const override { return "Fundamental Constants"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::CRITICAL; }

private:
    // (no private helpers; detection is performed inline in analyze)
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FINE_STRUCTURE_CONSTANT_DRIFT_OBSERVATORY_H