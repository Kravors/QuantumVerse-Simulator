/**
 * @file PTAScanner.cpp
 * @brief Implementation of the Pulsar Timing Array anomaly detector
 */

#include "PTAScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

PTAScanner::PTAScanner()
{
    setParameter("sigma_threshold", 3.0);
    setParameter("min_points", 2.0);
}

std::vector<InstrumentFinding> PTAScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double threshold = getParameter("sigma_threshold");

    double maxAbsResidual = 0.0;
    for (const auto& ev : trajectory) {
        double observed = ev.x;
        double expected = ev.z;
        double uncertainty = ev.y;

        if (std::isnan(observed) || std::isnan(expected) || std::isnan(uncertainty) ||
            std::isinf(observed) || std::isinf(expected) || std::isinf(uncertainty)) {
            return findings;
        }

        if (uncertainty <= 0.0) continue;

        double residual = std::abs(observed - expected) / uncertainty;
        maxAbsResidual = std::max(maxAbsResidual, residual);
    }

    if (maxAbsResidual > threshold) {
        double confidence = std::min(1.0, maxAbsResidual / (2.0 * threshold));

        InstrumentFinding finding;
        finding.id = "PTA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "PTA anomaly detected: correlation deviates from "
            "Hellings–Downs by " + std::to_string(maxAbsResidual) +
            " sigma (threshold = " + std::to_string(threshold) +
            "). Potential nanohertz GW signal from supermassive black-hole binaries.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["max_residual_sigma"] = maxAbsResidual;
        finding.parameters["threshold_sigma"] = threshold;

        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double PTAScanner::hellingsDowns(double theta)
{
    if (theta <= 0.0) return 0.5;
    if (theta >= M_PI) return 0.0;

    double x = 0.5 * (1.0 - std::cos(theta));
    double term1 = x * std::log(x);
    double term2 = (1.0 - x) * std::log(1.0 - x);
    return 0.5 * (term1 + term2) - 0.25;
}

} // namespace quantumverse
