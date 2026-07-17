/**
 * @file PTAScanner.h
 * @brief Pulsar Timing Array nanohertz gravitational-wave detector
 */

#ifndef QUANTUMVERSE_PTA_SCANNER_H
#define QUANTUMVERSE_PTA_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Pulsar Timing Array anomaly scanner
 *
 * Analyzes timing-residual cross-correlations between pulsar pairs and
 * compares them to the Hellings–Downs curve expected in general relativity.
 * Flags excess correlations as potential nanohertz gravitational-wave signals
 * from supermassive black-hole binaries or a stochastic GW background.
 */
class PTAScanner : public DiscoveryInstrument {
public:
    PTAScanner();
    ~PTAScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "PTAScanner"; }
    std::string getDescription() const override
    {
        return "Pulsar Timing Array nanohertz GW detector. "
               "Compares pulsar-timing cross-correlations to the Hellings–Downs "
               "curve and flags deviations as potential GW anomalies.";
    }
    std::string getCategory() const override { return "Multi-Messenger"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    static double hellingsDowns(double theta);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PTA_SCANNER_H
