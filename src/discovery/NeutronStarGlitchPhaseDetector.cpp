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

    double criticalSpin = getParameter("critical_spin_rate");
    double pinEnergy = getParameter("vortex_pin_energy");
    double gap0 = getParameter("superfluid_gap_0");
    double glitchThreshold = getParameter("glitch_size_threshold");
    double phaseSigma = getParameter("phase_transition_sigma");

    if (trajectory.size() < 6) return findings;

    // Extract spin-down rate and glitch candidates from trajectory
    std::vector<double> spinRates;
    std::vector<double> glitchSizes;
    std::vector<double> timingResiduals;

    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        if (dt <= 0) continue;

        // Angular velocity proxy from spatial displacement
        double omega = std::sqrt(trajectory[i].x * trajectory[i].x +
                                trajectory[i].y * trajectory[i].y) / dt;
        spinRates.push_back(omega);

        // Glitch size from sudden spin-up events
        if (i >= 2) {
            double prevDt = trajectory[i - 1].t - trajectory[i - 2].t;
            if (prevDt > 0) {
                double prevOmega = std::sqrt(trajectory[i - 1].x * trajectory[i - 1].x +
                                             trajectory[i - 1].y * trajectory[i - 1].y) / prevDt;
                double glitch = (omega - prevOmega) / prevOmega;
                glitchSizes.push_back(glitch);
            }
        }
    }

    if (spinRates.size() < 4 || glitchSizes.size() < 3) return findings;

    // Compute vortex unpinning probability
    double meanSpin = 0.0;
    for (double s : spinRates) meanSpin += s;
    meanSpin /= spinRates.size();

    double unpinningProb = computeVortexUnpinningProbability(
        std::abs(meanSpin - criticalSpin) / criticalSpin,
        *std::max_element(glitchSizes.begin(), glitchSizes.end()));

    // Estimate superfluid gap from glitch recurrence
    double meanGlitchInterval = 0.0;
    for (size_t i = 1; i < glitchSizes.size(); ++i) {
        meanGlitchInterval += std::abs(glitchSizes[i] - glitchSizes[i - 1]);
    }
    meanGlitchInterval /= (glitchSizes.size() - 1);
    double superfluidGap = estimateSuperfluidGap(meanGlitchInterval);

    // Detect phase transition in glitch size distribution
    bool phaseTransition = detectPhaseTransition(glitchSizes);

    if (unpinningProb > 0.5 || phaseTransition || superfluidGap > gap0 * 0.8) {
        double confidence = std::min(1.0, unpinningProb * 0.5 +
            (phaseTransition ? 0.3 : 0.0) +
            (superfluidGap > gap0 * 0.8 ? 0.2 : 0.0));

        InstrumentFinding finding;
        finding.id = "NSGP_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Neutron star glitch phase transition detected: "
            "unpinning_prob=" + std::to_string(unpinningProb) +
            ", superfluid_gap=" + std::to_string(superfluidGap) +
            " MeV, phase_transition=" + std::string(phaseTransition ? "yes" : "no");
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["unpinning_probability"] = unpinningProb;
        finding.parameters["superfluid_gap_MeV"] = superfluidGap;
        finding.parameters["mean_spin_rate"] = meanSpin;
        finding.parameters["critical_spin_rate"] = criticalSpin;
        finding.parameters["phase_transition"] = phaseTransition ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

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