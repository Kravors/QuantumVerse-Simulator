/**
 * @file UltralightDMWaveInterferometer.h
 * @brief Dark matter wave detection in the Solar System
 */

#ifndef QUANTUMVERSE_ULTRALIGHT_DM_WAVE_INTERFEROMETER_H
#define QUANTUMVERSE_ULTRALIGHT_DM_WAVE_INTERFEROMETER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Detects ultralight dark matter wave signatures
 *
 * Searches for oscillatory perturbations in the metric caused by
 * ultralight axion-like dark matter fields (m ~ 10^-22 eV).
 * Uses Solar System probes as interferometric detectors.
 */
class UltralightDMWaveInterferometer : public DiscoveryInstrument
{
public:
    UltralightDMWaveInterferometer();
    ~UltralightDMWaveInterferometer() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "UltralightDMWaveInterferometer"; }
    std::string getDescription() const override
    {
        return "Dark matter wave interferometer. Searches for oscillatory metric "
               "perturbations from ultralight axion-like dark matter fields "
               "(m ~ 1e-22 eV) using Solar System probes.";
    }
    std::string getCategory() const override { return "Dark Matter"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

private:
    double computeDMSignalAmplitude(double dmMass, double localDensity);
    double computeOscillationFrequency(double dmMass);
    bool detectCoherentOscillation(const std::vector<double>& residuals, double frequency);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ULTRALIGHT_DM_WAVE_INTERFEROMETER_H