/**
 * @file BlackHoleJetAnomalyRecogniser.h
 * @brief Non-Kerr jet launching anomaly detection
 */

#ifndef QUANTUMVERSE_BLACK_HOLE_JET_ANOMALY_RECOGNISER_H
#define QUANTUMVERSE_BLACK_HOLE_JET_ANOMALY_RECOGNISER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects non-Kerr jet launching anomalies in black hole systems
 *
 * Analyzes jet morphology and launching parameters for deviations
 * from Kerr metric predictions, indicating exotic compact objects
 * or modified gravity effects near the horizon.
 */
class BlackHoleJetAnomalyRecogniser : public DiscoveryInstrument
{
public:
    BlackHoleJetAnomalyRecogniser();
    ~BlackHoleJetAnomalyRecogniser() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "BlackHoleJetAnomalyRecogniser"; }
    std::string getDescription() const override
    {
        return "Non-Kerr jet launching analysis. Detects anomalies in black hole "
               "jet morphology and launching parameters that deviate from Kerr "
               "metric predictions, indicating exotic compact objects or modified "
               "gravity near the horizon.";
    }
    std::string getCategory() const override { return "Black Holes"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    double computeBlandfordZnajekPower(double spin, double magneticField, double mass);
    double computeJetCollimationAngle(double spinParameter);
    bool detectJetPrecession(const std::vector<double>& jetAngles);
    double estimateSpinFromJet(double jetPower, double mass);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_BLACK_HOLE_JET_ANOMALY_RECOGNISER_H