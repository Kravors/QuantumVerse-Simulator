/**
 * @file GWMemoryDetector.cpp
 * @brief Implementation of the GW memory detector
 */

#include "GWMemoryDetector.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

GWMemoryDetector::GWMemoryDetector()
{
    setParameter("sigma_threshold", kSigmaThreshold);
}

std::vector<InstrumentFinding> GWMemoryDetector::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < kMinTrajectorySize) {
        return findings;
    }

    double maxDeviation = 0.0;
    for (const auto& pt : trajectory) {
        double time     = pt.t;  // time (s)
        double observed = pt.x;  // observed strain memory
        double err      = pt.y;  // uncertainty
        double theory   = pt.z;  // predicted memory amplitude

        (void)time;

        if (std::isnan(observed) || std::isnan(theory) || std::isnan(err) ||
            std::isinf(observed) || std::isinf(theory) || std::isinf(err)) {
            return findings;
        }

        if (err <= 0.0) continue;

        double deviation = std::abs(observed - theory) / err;
        if (deviation > maxDeviation) maxDeviation = deviation;
    }

    double threshold = getParameter("sigma_threshold");

    if (maxDeviation > threshold) {
        InstrumentFinding finding;
        finding.id = "GWMEM_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(std::min(1.0, (maxDeviation - threshold) / (2.0 * threshold)));
        finding.confidence = std::min(1.0, (maxDeviation - threshold) / (2.0 * threshold));
        finding.isAnomaly = true;
        finding.description = "GW memory anomaly: excess permanent displacement of " +
                             std::to_string(maxDeviation) + " sigma detected.";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["max_deviation_sigma"] = maxDeviation;
        finding.parameters["threshold_sigma"] = threshold;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
