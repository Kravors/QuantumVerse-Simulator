/**
 * @file FRBDispersionScanner.cpp
 * @brief Implementation of the Fast Radio Burst dispersion measure scanner
 */

#include "FRBDispersionScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

FRBDispersionScanner::FRBDispersionScanner()
{
    setParameter("chi2_threshold", 4.0);
    setParameter("min_points", 3.0);
}

std::vector<InstrumentFinding> FRBDispersionScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double threshold = getParameter("chi2_threshold");

    double chi2 = 0.0;
    int valid = 0;
    double maxExcess = 0.0;

    for (const auto& ev : trajectory) {
        double observedDM = ev.x;
        double uncertainty = ev.y;
        double expectedDM = ev.z;

        if (std::isnan(observedDM) || std::isnan(uncertainty) || std::isnan(expectedDM) ||
            std::isinf(observedDM) || std::isinf(uncertainty) || std::isinf(expectedDM)) {
            return findings;
        }

        if (uncertainty <= 0.0) continue;

        double residual = observedDM - expectedDM;
        double sigma = residual / uncertainty;
        chi2 += sigma * sigma;
        maxExcess = std::max(maxExcess, std::abs(sigma));
        valid++;
    }

    if (valid < static_cast<int>(minPoints)) return findings;

    double reducedChi2 = chi2 / valid;

    if (reducedChi2 > threshold) {
        double confidence = std::min(1.0, reducedChi2 / (2.0 * threshold));

        InstrumentFinding finding;
        finding.id = "FRB_DM_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "FRB dispersion anomaly: reduced chi-squared = "
            + std::to_string(reducedChi2) +
            " (threshold = " + std::to_string(threshold) +
            "). Max DM excess = " + std::to_string(maxExcess) +
            " sigma. Possible new plasma structure or modified dispersion physics.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["reduced_chi2"] = reducedChi2;
        finding.parameters["max_excess_sigma"] = maxExcess;
        finding.parameters["threshold"] = threshold;
        finding.parameters["valid_points"] = static_cast<double>(valid);

        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double FRBDispersionScanner::expectedGalacticDM(double gl, double gb)
{
    if (std::isnan(gl) || std::isnan(gb) || std::isinf(gl) || std::isinf(gb)) {
        return 0.0;
    }

    double lat = std::abs(gb) * 180.0 / M_PI;
    double baseDM = 100.0 * std::exp(-lat / 15.0);
    double lonFactor = 1.0 + 0.3 * std::cos(gl);
    return std::max(0.0, baseDM * lonFactor);
}

} // namespace quantumverse
