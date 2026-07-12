/**
 * @file FineStructureConstantDriftObservatory.cpp
 * @brief Implementation of the alpha-drift observatory
 */

#include "FineStructureConstantDriftObservatory.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

FineStructureConstantDriftObservatory::FineStructureConstantDriftObservatory()
{
    setParameter("drift_significance_threshold", 3.0);
    setParameter("min_points", 4.0);
}

std::vector<InstrumentFinding> FineStructureConstantDriftObservatory::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double sigThreshold = getParameter("drift_significance_threshold");

    // Extract (time, alpha) measurements. Convention: t = time, z = measured α.
    std::vector<double> times, alphas;
    for (const auto& ev : trajectory) {
        times.push_back(ev.t);
        alphas.push_back(ev.z);
    }
    size_t n = times.size();
    if (n < 3) return findings;

    // Least-squares linear fit: alpha = a0 + slope * t.
    double sumT = 0.0, sumA = 0.0, sumT2 = 0.0, sumTA = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sumT += times[i];
        sumA += alphas[i];
        sumT2 += times[i] * times[i];
        sumTA += times[i] * alphas[i];
    }
    double denom = static_cast<double>(n) * sumT2 - sumT * sumT;
    if (std::abs(denom) < 1e-30) return findings;

    double slope = (static_cast<double>(n) * sumTA - sumT * sumA) / denom;
    double a0 = (sumA - slope * sumT) / static_cast<double>(n);

    // Residuals and standard error of the slope.
    double sxx = sumT2 - sumT * sumT / static_cast<double>(n);
    double rss = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double pred = a0 + slope * times[i];
        double res = alphas[i] - pred;
        rss += res * res;
    }
    double dof = static_cast<double>(n) - 2.0;
    if (dof < 1.0) return findings;

    double sigmaRes = std::sqrt(rss / dof + 1e-30);
    double sigmaSlope = sigmaRes / std::sqrt(sxx + 1e-30);
    double significance = (sigmaSlope > 0.0) ? std::abs(slope) / sigmaSlope : 0.0;

    if (significance > sigThreshold) {
        double confidence = std::min(1.0, significance / (2.0 * sigThreshold));

        InstrumentFinding finding;
        finding.id = "FSCD_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Fine-structure constant drift detected: dα/dt = " +
            std::to_string(slope) + " with significance " + std::to_string(significance) +
            " sigma, suggesting a varying fundamental coupling (scalar-tensor / string).";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["drift_rate"] = slope;
        finding.parameters["alpha_0"] = a0;
        finding.parameters["significance_sigma"] = significance;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
