/**
 * @file UltralightDMWaveInterferometer.cpp
 * @brief Implementation of the ultralight DM wave interferometer
 */

#include "UltralightDMWaveInterferometer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

UltralightDMWaveInterferometer::UltralightDMWaveInterferometer()
{
    setParameter("dm_mass_ev", 1e-22);
    setParameter("local_density_gev_cm3", 0.3);
    setParameter("coherence_time_s", 1e6);
    setParameter("detection_snr_threshold", 5.0);
}

std::vector<InstrumentFinding> UltralightDMWaveInterferometer::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double dmMass = getParameter("dm_mass_ev");
    double localDensity = getParameter("local_density_gev_cm3");
    double coherenceTime = getParameter("coherence_time_s");
    double snrThreshold = getParameter("detection_snr_threshold");

    if (trajectory.size() < 4) return findings;

    // Compute DM oscillation frequency
    double oscFreq = computeOscillationFrequency(dmMass);

    // Analyze trajectory for oscillatory perturbations
    std::vector<double> residuals;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        double dx = trajectory[i].x - trajectory[i - 1].x;
        double dy = trajectory[i].y - trajectory[i - 1].y;
        double dz = trajectory[i].z - trajectory[i - 1].z;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        // Expected geodesic distance vs observed
        double expectedDist = dist * (1.0 + 0.0); // GR baseline
        double residual = std::abs(dist - expectedDist);
        residuals.push_back(residual);
    }

    // Signal amplitude from DM wave
    double signalAmp = computeDMSignalAmplitude(dmMass, localDensity);

    // Check for coherent oscillation at DM frequency
    bool coherentSignal = detectCoherentOscillation(residuals, oscFreq);

    // SNR estimate
    double rmsNoise = 0.0;
    if (!residuals.empty()) {
        for (double r : residuals) rmsNoise += r * r;
        rmsNoise = std::sqrt(rmsNoise / residuals.size());
    }
    double snr = rmsNoise > 0 ? signalAmp / rmsNoise : 0.0;

    if (coherentSignal || snr > snrThreshold) {
        double confidence = std::min(1.0, snr / (snrThreshold * 2.0));

        InstrumentFinding finding;
        finding.id = "ULDM_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Ultralight DM wave detected: mass=" + std::to_string(dmMass)
            + " eV, frequency=" + std::to_string(oscFreq) + " Hz, SNR="
            + std::to_string(snr) + ", coherent=" + std::string(coherentSignal ? "yes" : "no");
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["dm_mass_ev"] = dmMass;
        finding.parameters["oscillation_freq_hz"] = oscFreq;
        finding.parameters["signal_amplitude"] = signalAmp;
        finding.parameters["snr"] = snr;
        finding.parameters["local_density_gev_cm3"] = localDensity;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double UltralightDMWaveInterferometer::computeDMSignalAmplitude(
    double dmMass, double localDensity)
{
    // Axion-photon coupling: g_aγγ ~ 10^-10 GeV^-1
    // Signal amplitude ~ g_aγγ * B * sqrt(ρ_DM / m_a)
    double coupling = 1e-10; // GeV^-1
    double magneticField = 1e-5; // T (interstellar)
    double evToGeV = 1e-9;

    double amplitude = coupling * magneticField *
        std::sqrt(localDensity / (dmMass * evToGeV));

    return amplitude;
}

double UltralightDMWaveInterferometer::computeOscillationFrequency(double dmMass)
{
    // f = m_a c^2 / h = m_a / (2π ħ) in natural units
    // m_a in eV -> f in Hz
    double hbar_eV_s = 6.582e-16; // eV·s
    return dmMass / hbar_eV_s;
}

bool UltralightDMWaveInterferometer::detectCoherentOscillation(
    const std::vector<double>& residuals, double frequency)
{
    if (residuals.size() < 10 || frequency <= 0) return false;

    // Simple Fourier analysis: look for peak at expected frequency
    double sumCos = 0.0, sumSin = 0.0;
    for (size_t i = 0; i < residuals.size(); ++i) {
        double phase = 2.0 * M_PI * frequency * i;
        sumCos += residuals[i] * std::cos(phase);
        sumSin += residuals[i] * std::sin(phase);
    }

    double power = std::sqrt(sumCos * sumCos + sumSin * sumSin) / residuals.size();
    double meanResidual = 0.0;
    for (double r : residuals) meanResidual += r;
    meanResidual /= residuals.size();

    // Coherent if power exceeds 3x mean
    return meanResidual > 0 && power / meanResidual > 3.0;
}

} // namespace quantumverse