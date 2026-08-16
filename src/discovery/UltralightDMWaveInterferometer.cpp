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
    (void)metric;
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < 10) return findings;

    double dmMass = getParameter("dm_mass_ev");
    double localDensity = getParameter("local_density_gev_cm3");
    double snrThreshold = getParameter("detection_snr_threshold");

    // 1. Extract the time series of the oscillatory observable (x coordinate).
    std::vector<double> times, xvals;
    for (const auto& ev : trajectory) {
        times.push_back(ev.t);
        xvals.push_back(ev.x);
    }
    size_t n = xvals.size();
    double dt = times[1] - times[0];
    if (dt <= 0) return findings;

    // 1b. Reject non-oscillatory input before any spectral analysis.
    //     A constant (pure-DC) series has zero variance, so every non-DC DFT bin
    //     holds nothing but floating-point round-off. The peak/mean ratio of pure
    //     round-off is meaningless and routinely exceeds any SNR threshold, which
    //     would report a detection for a flat signal. Detrend by the mean and
    //     require the residual RMS to clear a round-off floor scaled to the data.
    double mean = 0.0;
    double maxAbs = 0.0;
    for (double v : xvals) {
        mean += v;
        maxAbs = std::max(maxAbs, std::abs(v));
    }
    mean /= static_cast<double>(n);

    double variance = 0.0;
    for (double& v : xvals) {
        v -= mean;              // remove DC so it cannot leak into the spectrum
        variance += v * v;
    }
    variance /= static_cast<double>(n);
    const double rms = std::sqrt(variance);
    const double roundOffFloor = 1e-9 * std::max(1.0, maxAbs);
    if (rms <= roundOffFloor) return findings;

    // 2. Discrete Fourier transform over angular frequencies (rad/s).
    std::vector<double> freqs;
    std::vector<double> mags;
    for (size_t k = 0; k <= n / 2; ++k) {
        double freq = 2.0 * M_PI * static_cast<double>(k) /
                      (static_cast<double>(n) * dt);
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double ph = freq * times[i];
            re += xvals[i] * std::cos(ph);
            im += xvals[i] * std::sin(ph);
        }
        freqs.push_back(freq);
        mags.push_back(std::sqrt(re * re + im * im));
    }

    // 3. Locate the spectral peak (excluding the DC bin, k = 0).
    size_t peakIdx = 0;
    double peakMag = 0.0;
    double meanMag = 0.0;
    for (size_t i = 1; i < mags.size(); ++i) {
        meanMag += mags[i];
        if (mags[i] > peakMag) {
            peakMag = mags[i];
            peakIdx = i;
        }
    }
    meanMag /= std::max<size_t>(1, mags.size() - 1);

    double snr = (meanMag > 0.0) ? (peakMag / meanMag) : 0.0;
    if (snr <= snrThreshold) return findings;

    double peakFreq = freqs[peakIdx];
    double amplitude = 2.0 * peakMag / static_cast<double>(n);

    double dmOscFreq = computeOscillationFrequency(dmMass);
    double dmAmp = computeDMSignalAmplitude(dmMass, localDensity);

    double confidence = std::min(1.0, snr / (2.0 * snrThreshold));

    InstrumentFinding finding;
    finding.id = "ULDM_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Ultralight DM wave detected: oscillation_freq=" +
        std::to_string(peakFreq) + " rad/s, amplitude=" + std::to_string(amplitude) +
        ", SNR=" + std::to_string(snr) + ", dm_mass=" + std::to_string(dmMass) + " eV";
    finding.location = location;
    finding.timestamp = trajectory.back().t;
    finding.parameters["oscillation_freq_rad_s"] = peakFreq;
    finding.parameters["signal_amplitude"] = amplitude;
    finding.parameters["snr"] = snr;
    finding.parameters["dm_mass_ev"] = dmMass;
    finding.parameters["dm_oscillation_freq_hz"] = dmOscFreq;
    finding.parameters["dm_signal_amplitude"] = dmAmp;
    addFinding(finding);
    findings.push_back(finding);

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

} // namespace quantumverse