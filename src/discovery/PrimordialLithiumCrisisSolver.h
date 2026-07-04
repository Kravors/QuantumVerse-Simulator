/**
 * @file PrimordialLithiumCrisisSolver.h
 * @brief Exotic BBN nucleosynthesis explorer
 */

#ifndef QUANTUMVERSE_PRIMORDIAL_LITHIUM_CRISIS_SOLVER_H
#define QUANTUMVERSE_PRIMORDIAL_LITHIUM_CRISIS_SOLVER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Searches for solutions to the primordial lithium crisis
 *
 * Explores exotic BBN nucleosynthesis scenarios including
 * dark matter interactions, varying constants, and non-standard
 * neutrino physics to resolve the 7Li problem.
 */
class PrimordialLithiumCrisisSolver : public DiscoveryInstrument
{
public:
    PrimordialLithiumCrisisSolver();
    ~PrimordialLithiumCrisisSolver() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "PrimordialLithiumCrisisSolver"; }
    std::string getDescription() const override
    {
        return "Exotic BBN nucleosynthesis explorer. Searches for solutions to "
               "the primordial lithium-7 problem through dark matter interactions, "
               "varying fundamental constants, and non-standard neutrino physics.";
    }
    std::string getCategory() const override { return "Cosmology"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::MEDIUM; }

private:
    double computeLithiumAbundance(double baryonToPhoton, double neutronLifetime, double dmCrossSection);
    double computeBBNChiSquared(const std::vector<double>& observedAbundances,
        const std::vector<double>& predictedAbundances);
    bool isLithiumCrisisResolved(double li7OverH, double be7OverH);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PRIMORDIAL_LITHIUM_CRISIS_SOLVER_H