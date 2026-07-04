/**
 * @file RecombinationConstantVariationImager.cpp
 * @brief Implementation of the CMB recombination constant variation imager
 */

#include "RecombinationConstantVariationImager.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

RecombinationConstantVariationImager::RecombinationConstantVariationImager()
{
    setParameter("alpha_variation_limit", 1e-5);
    setParameter("electron_mass_variation_limit", 1e-5);
    setParameter("z_rec_nominal", 1100.0);
    setParameter("cmb_temperature", 2.7255);
    setParameter("spectral_distortion_threshold", 3.0);
}

std::vector<InstrumentFinding> RecombinationConstantVariationImager::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double alphaLimit = getParameter("alpha_variation_limit");
    double meLimit = getParameter("electron_mass_variation_limit");
    double zRecNom = getParameter("z_rec_nominal");
    double tcmb = getParameter("cmb_temperature");

    if (trajectory.size() < 4) return findings;

    // Simulate CMB power spectrum analysis along trajectory
    // Use trajectory points as proxy for multipole moments
    std::vector<double> powerSpectrum;
    std::vector<double> multipoles;

    for (size_t i = 0; i < trajectory.size(); ++i) {
        double l = 100.0 + i * 50.0; // multipole range ~100-2000
        double deltaAlpha = alphaLimit * std::sin(trajectory[i].t * 0.01);
        double deltaMe = meLimit * std::cos(trajectory[i].t * 0.01);

        // Recombination redshift shift
        double zRec = computeRecombinationRedshift(1.0 + deltaAlpha, 1.0 + deltaMe);
        double lShift = computeDMultipoleShift(zRec - zRecNom, l);

        // Power spectrum with potential distortion
        double power = std::exp(-l * (l + 1.0) / (2.0 * 3000.0 * 3000.0)) *
            (1.0 + 0.01 * lShift);
        power += 1e-10 * std::sin(l * 0.01); // small oscillatory distortion

        powerSpectrum.push_back(power);
        multipoles.push_back(l);
    }

    // Detect spectral distortion
    bool distortionDetected = detectSpectralDistortion(powerSpectrum, multipoles);

    if (distortionDetected) {
        // Compute chi-squared against standard model
        double chi2 = 0.0;
        int dof = 0;
        for (size_t i = 0; i < powerSpectrum.size(); ++i) {
            double expected = std::exp(-multipoles[i] * (multipoles[i] + 1.0) /
                (2.0 * 3000.0 * 3000.0));
            double sigma = expected * 0.01;
            if (sigma > 0) {
                chi2 += (powerSpectrum[i] - expected) * (powerSpectrum[i] - expected) / (sigma * sigma);
                dof++;
            }
        }

        double confidence = std::min(1.0, constrainConstantVariation(chi2, dof));

        InstrumentFinding finding;
        finding.id = "RCVI_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "CMB recombination constant variation detected: "
            "spectral distortion in damping tail, chi2/dof=" +
            std::to_string(dof > 0 ? chi2 / dof : 0.0) +
            ", possible time-varying alpha or electron mass at z~1100";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["chi2"] = chi2;
        finding.parameters["dof"] = static_cast<double>(dof);
        finding.parameters["z_rec_shift"] = powerSpectrum.size() > 0 ? 1.0 : 0.0;
        finding.parameters["alpha_variation"] = alphaLimit;
        finding.parameters["distortion_detected"] = 1.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double RecombinationConstantVariationImager::computeRecombinationRedshift(
    double alphaOverAlpha0, double meffOverMe0)
{
    // Recombination redshift depends on fundamental constants:
    // z_rec ~ (me * alpha^2 / (kB T_CMB))^(1/3) * const
    // Variation: delta_z/z ~ (1/3)(delta_me/me + 2*delta_alpha/alpha)
    double zRec0 = 1100.0;
    double deltaMe = meffOverMe0 - 1.0;
    double deltaAlpha = alphaOverAlpha0 - 1.0;

    double zRec = zRec0 * std::pow(meffOverMe0 * alphaOverAlpha0 * alphaOverAlpha0, 1.0 / 3.0);
    return zRec;
}

double RecombinationConstantVariationImager::computeDMultipoleShift(
    double deltaZrec, double lPeak)
{
    // Shift in peak positions: delta_l/l ~ delta_z/z
    if (lPeak <= 0 || std::abs(deltaZrec) < 1e-10) return 0.0;
    return deltaZrec / 1100.0 * lPeak;
}

bool RecombinationConstantVariationImager::detectSpectralDistortion(
    const std::vector<double>& powerSpectrum,
    const std::vector<double>& multipoles)
{
    if (powerSpectrum.size() < 5 || multipoles.size() < 5) return false;

    // Look for anomalous features in the damping tail (l > 1500)
    double highLSum = 0.0, lowLSum = 0.0;
    int highN = 0, lowN = 0;

    for (size_t i = 0; i < powerSpectrum.size(); ++i) {
        if (multipoles[i] > 1500) {
            highLSum += powerSpectrum[i];
            highN++;
        } else {
            lowLSum += powerSpectrum[i];
            lowN++;
        }
    }

    if (highN == 0 || lowN == 0) return false;

    double highLAve = highLSum / highN;
    double lowLAve = lowLSum / lowN;

    // Excess power in damping tail relative to low-l
    double ratio = highLAve / (lowLAve + 1e-30);

    // Standard model predicts ratio ~ 0.01-0.05; deviations > 3x indicate distortion
    return ratio > 0.15 || ratio < 0.001;
}

double RecombinationConstantVariationImager::constrainConstantVariation(
    double chi2, int dof)
{
    if (dof <= 0) return 0.0;

    // Convert chi2 to p-value, then to confidence of variation
    double chi2PerDof = chi2 / dof;

    // Simple approximation: excess chi2 indicates variation
    if (chi2PerDof < 1.5) return 0.0; // consistent with standard model

    // Map to [0, 1] confidence
    double confidence = std::min(1.0, (chi2PerDof - 1.5) / 5.0);
    return confidence;
}

} // namespace quantumverse