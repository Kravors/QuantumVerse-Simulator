/**
 * @file RecombinationConstantVariationImager.cpp
 * @brief Implementation of the CMB recombination constant variation imager
 */

#include "RecombinationConstantVariationImager.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

RecombinationConstantVariationImager::RecombinationConstantVariationImager()
{
    setParameter("alpha_sensitivity", 1.0);    // C_l response coefficient to Δα/α
    setParameter("confidence_threshold", 1e-5); // minimum |Δα/α| to flag a finding
    setParameter("min_points", 10.0);           // minimum number of (ℓ, C_l) samples
}

double RecombinationConstantVariationImager::expectedPowerSpectrum(double l)
{
    // Simplified smooth ΛCDM proxy shared with the validation harness:
    // C_l ~ A / (ℓ (ℓ+1)). Guard against division by (or near) zero.
    if (l < 1.0) return 0.0;
    return 1000.0 / (l * (l + 1.0));
}

std::vector<InstrumentFinding> RecombinationConstantVariationImager::analyze(
    const MetricTensor& /*metric*/, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double alphaSensitivity = getParameter("alpha_sensitivity");
    if (alphaSensitivity == 0.0) alphaSensitivity = 1.0;
    double threshold = getParameter("confidence_threshold");

    // Extract per-point ratios of observed to expected C_l.
    // Convention: t = multipole ℓ, x = observed temperature power C_l.
    std::vector<double> ratios;
    ratios.reserve(trajectory.size());
    for (const auto& ev : trajectory) {
        double expected = expectedPowerSpectrum(ev.t);
        if (expected > 0.0) ratios.push_back(ev.x / expected);
    }
    if (ratios.size() < static_cast<size_t>(minPoints)) return findings;

    // Mean ratio and standard error of the mean across multipoles.
    double sum = 0.0;
    for (double r : ratios) sum += r;
    double meanRatio = sum / ratios.size();

    double var = 0.0;
    for (double r : ratios) var += (r - meanRatio) * (r - meanRatio);
    var /= std::max(1.0, static_cast<double>(ratios.size()) - 1.0);
    double sem = std::sqrt(var / ratios.size() + 1e-300);

    // Infer variation in alpha from the mean ratio:
    // C_l ∝ (1 + alpha_sensitivity * Δα/α)  =>  Δα/α = (meanRatio - 1) / sensitivity.
    double deltaAlpha = (meanRatio - 1.0) / alphaSensitivity;

    if (std::abs(deltaAlpha) <= threshold) return findings;

    // Significance of the deviation in Δα/α relative to its measurement uncertainty.
    double sigmaDelta = sem / alphaSensitivity;
    double significance = (sigmaDelta > 0.0) ? std::abs(deltaAlpha) / sigmaDelta : 50.0;
    if (!(significance > 0.0) || significance > 50.0) significance = 50.0;

    double confidence = std::min(1.0, std::abs(deltaAlpha) / threshold);

    InstrumentFinding finding;
    finding.id = "RCVI_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Detected variation in the fine-structure constant (alpha) "
        "at recombination: Δα/α = " + std::to_string(deltaAlpha) +
        " (significance " + std::to_string(significance) + " sigma), inferred "
        "from a systematic scaling of the CMB power spectrum.";
    finding.location = location;
    finding.timestamp = location.t;
    finding.parameters["delta_alpha_over_alpha"] = deltaAlpha;
    finding.parameters["mean_ratio"] = meanRatio;
    finding.parameters["significance_sigma"] = significance;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

} // namespace quantumverse
