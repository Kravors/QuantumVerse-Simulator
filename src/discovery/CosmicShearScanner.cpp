/**
 * @file CosmicShearScanner.cpp
 * @brief Implementation of the cosmic shear weak lensing anomaly detector
 */

#include "CosmicShearScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

CosmicShearScanner::CosmicShearScanner()
{
    setParameter("chi2_threshold", 2.0);
    setParameter("min_theta", 1.0);
    setParameter("max_theta", 1000.0);
    setParameter("min_points", 3.0);
}

std::vector<InstrumentFinding> CosmicShearScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double threshold = getParameter("chi2_threshold");
    double minTheta = getParameter("min_theta");
    double maxTheta = getParameter("max_theta");

    std::vector<double> thetas, obsShears, errors, theoryShears;
    for (const auto& ev : trajectory) {
        double theta = ev.t;
        if (theta < minTheta || theta > maxTheta) continue;
        thetas.push_back(theta);
        obsShears.push_back(ev.x);
        errors.push_back(ev.y);
        theoryShears.push_back(ev.z);
    }

    size_t n = thetas.size();
    if (n < static_cast<size_t>(minPoints)) return findings;

    double chi2 = 0.0;
    double maxExcessSigma = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double residual = obsShears[i] - theoryShears[i];
        double sigma = errors[i];
        if (sigma <= 0.0) continue;
        double z = residual / sigma;
        chi2 += z * z;
        maxExcessSigma = std::max(maxExcessSigma, std::abs(z));
    }

    double dof = static_cast<double>(n);
    if (dof < 1.0) return findings;

    double reducedChi2 = chi2 / dof;

    if (reducedChi2 > threshold) {
        double confidence = std::min(1.0, (reducedChi2 - 1.0) / (threshold - 1.0));

        InstrumentFinding finding;
        finding.id = "COSMIC_SHEAR_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Cosmic shear anomaly detected: reduced chi-squared = "
            + std::to_string(reducedChi2) +
            " exceeds threshold " + std::to_string(threshold) +
            ". Max shear excess = " + std::to_string(maxExcessSigma) +
            " sigma. Possible modified gravity or dark-matter substructure.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["reduced_chi2"] = reducedChi2;
        finding.parameters["max_excess_sigma"] = maxExcessSigma;
        finding.parameters["threshold"] = threshold;
        finding.parameters["n_scales"] = static_cast<double>(n);

        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double CosmicShearScanner::theoreticalShear(double theta)
{
    if (theta <= 0.0) return 0.0;
    return 1e-4 / (theta * theta);
}

} // namespace quantumverse
