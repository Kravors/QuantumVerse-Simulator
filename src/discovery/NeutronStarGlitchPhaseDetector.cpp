/**
 * @file NeutronStarGlitchPhaseDetector.cpp
 * @brief Implementation of the quantum-vortex glitch phase detector
 */

#include "NeutronStarGlitchPhaseDetector.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

NeutronStarGlitchPhaseDetector::NeutronStarGlitchPhaseDetector()
{
    setParameter("critical_spin_rate", 200.0);     // Hz
    setParameter("vortex_pin_energy", 1e-10);       // J
    setParameter("superfluid_gap_0", 1.0);          // MeV
    setParameter("glitch_size_threshold", 1e-7);
    setParameter("phase_transition_sigma", 3.0);
}

std::vector<InstrumentFinding> NeutronStarGlitchPhaseDetector::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < 6) return findings;

    double glitchThreshold = getParameter("glitch_size_threshold");

    // 1. Build a spin-rate proxy from the trajectory's spatial projection.
    //    omega_i = sqrt(x_i^2 + y_i^2) / dt_i  (rad/s analogue).
    std::vector<double> omega;
    std::vector<double> omegaTime;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        if (dt <= 0) continue;
        double r = std::sqrt(trajectory[i].x * trajectory[i].x +
                             trajectory[i].y * trajectory[i].y);
        omega.push_back(r / dt);
        omegaTime.push_back(trajectory[i].t);
    }

    if (omega.size() < 3) return findings;

    // 2. Detect a sudden spin-up (glitch) as a relative change in spin rate.
    double glitchTime = 0.0;
    double deltaRate = 0.0;
    bool glitchFound = false;
    for (size_t i = 1; i < omega.size(); ++i) {
        double denom = std::abs(omega[i - 1]) + 1e-30;
        double relChange = (omega[i] - omega[i - 1]) / denom;
        if (relChange > glitchThreshold && !glitchFound) {
            glitchFound = true;
            glitchTime = omegaTime[i];
            deltaRate = omega[i] - omega[i - 1];
        }
    }

    if (!glitchFound) return findings;

    // 3. Characterise the glitch via the quantum-vortex unpinning model.
    double meanSpin = 0.0;
    for (double w : omega) meanSpin += w;
    meanSpin /= omega.size();

    double criticalSpin = getParameter("critical_spin_rate");
    double unpinningProb = computeVortexUnpinningProbability(
        std::abs(meanSpin - criticalSpin) / criticalSpin, deltaRate);
    double superfluidGap = estimateSuperfluidGap(glitchTime + 1.0);

    double confidence = std::min(1.0, 0.5 + 0.5 * std::min(1.0,
        deltaRate / (std::abs(meanSpin) + 1e-30)));

    InstrumentFinding finding;
    finding.id = "NSGP_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Neutron star glitch detected at t=" +
        std::to_string(glitchTime) + " s: delta_spin_rate=" +
        std::to_string(deltaRate) + ", unpinning_prob=" +
        std::to_string(unpinningProb) + ", superfluid_gap=" +
        std::to_string(superfluidGap) + " MeV";
    finding.location = location;
    finding.timestamp = glitchTime;
    finding.parameters["glitch_time"] = glitchTime;
    finding.parameters["delta_rate"] = deltaRate;
    finding.parameters["mean_spin_rate"] = meanSpin;
    finding.parameters["critical_spin_rate"] = criticalSpin;
    finding.parameters["unpinning_probability"] = unpinningProb;
    finding.parameters["superfluid_gap_MeV"] = superfluidGap;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

double NeutronStarGlitchPhaseDetector::computeVortexUnpinningProbability(
    double spinDownRate, double glitchSize)
{
    // Exponential activation model: P ~ exp(-E_pin / (k_B T_eff))
    // T_eff ~ hbar * Omega_critical
    double omegaRatio = spinDownRate;
    double effectiveTemp = 1.0e6 * omegaRatio; // K (effective temperature)

    if (effectiveTemp < 1e-10) return 0.0;

    double boltzmann = 8.617e-5; // eV/K
    double activationEnergy = getParameter("vortex_pin_energy");

    double prob = 1.0 - std::exp(-activationEnergy / (boltzmann * effectiveTemp));
    return std::min(1.0, prob);
}

double NeutronStarGlitchPhaseDetector::estimateSuperfluidGap(
    double glitchRecurrenceTime)
{
    // Gap ~ hbar / tau_recurrence
    // Scaled to MeV
    double hbarMeV_s = 6.582e-22; // MeV·s
    double gap = hbarMeV_s / (glitchRecurrenceTime + 1e-30);

    return gap;
}

bool NeutronStarGlitchPhaseDetector::detectPhaseTransition(
    const std::vector<double>& glitchSizes)
{
    if (glitchSizes.size() < 5) return false;

    // Look for bimodal distribution in glitch sizes
    double mean = 0.0;
    for (double g : glitchSizes) mean += g;
    mean /= glitchSizes.size();

    double variance = 0.0;
    for (double g : glitchSizes) variance += (g - mean) * (g - mean);
    variance /= glitchSizes.size();
    double stddev = std::sqrt(variance);

    // Count outliers
    int largeGlitches = 0, smallGlitches = 0;
    for (double g : glitchSizes) {
        if (g > mean + stddev) largeGlitches++;
        else if (g < mean - stddev) smallGlitches++;
    }

    // Phase transition indicated by bimodal distribution
    double bimodalRatio = static_cast<double>(std::min(largeGlitches, smallGlitches)) /
        std::max(1, std::max(largeGlitches, smallGlitches));

    return bimodalRatio > 0.2 && glitchSizes.size() > 4;
}

} // namespace quantumverse