/**
 * @file CMBLensingScanner.cpp
 * @brief Implementation of the CMB lensing anomaly detector
 */

#include "CMBLensingScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

CMBLensingScanner::CMBLensingScanner()
{
    setParameter("cl_threshold_sigma", 5.0);
    setParameter("min_ell", 2.0);
    setParameter("max_ell", 2500.0);
    setParameter("min_points", 4.0);
}

std::vector<InstrumentFinding> CMBLensingScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double sigThreshold = getParameter("cl_threshold_sigma");
    double minEll = getParameter("min_ell");
    double maxEll = getParameter("max_ell");

    std::vector<double> ells, cls;
    for (const auto& ev : trajectory) {
        double ell = ev.t;
        if (ell < minEll || ell > maxEll) continue;
        ells.push_back(ell);
        cls.push_back(ev.x);
    }

    size_t n = ells.size();
    if (n < static_cast<size_t>(minPoints)) return findings;

    std::vector<double> residuals;
    residuals.reserve(n);
    double rss = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double predicted = theoreticalCl(ells[i]);
        double residual = cls[i] - predicted;
        residuals.push_back(residual);
        rss += residual * residual;
    }

    double dof = static_cast<double>(n);
    if (dof < 1.0) return findings;

    double sigmaRes = std::sqrt(rss / dof + 1e-30);
    if (sigmaRes < 1e-30) return findings;

    double maxAbsRes = 0.0;
    for (double r : residuals) {
        maxAbsRes = std::max(maxAbsRes, std::abs(r));
    }
    double significance = maxAbsRes / sigmaRes;

    if (significance > sigThreshold) {
        double confidence = std::min(1.0, significance / (2.0 * sigThreshold));

        InstrumentFinding finding;
        finding.id = "CMBLENS_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "CMB lensing anomaly detected: residual RMS = " +
            std::to_string(sigmaRes) + " uK^2, max|residual| = " +
            std::to_string(maxAbsRes) + " uK^2, significance = " +
            std::to_string(significance) + " sigma (threshold = " +
            std::to_string(sigThreshold) + "). Potential dark-matter / "
            "neutrino-mass deviation from ΛCDM.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["sigma_residual"] = sigmaRes;
        finding.parameters["max_abs_residual"] = maxAbsRes;
        finding.parameters["significance_sigma"] = significance;
        finding.parameters["n_multipoles"] = static_cast<double>(n);

        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double CMBLensingScanner::theoreticalCl(double ell)
{
    if (ell <= 1.0) return 0.0;
    return 1000.0 * 200.0 * 201.0 / (ell * (ell + 1.0));
}

double CMBLensingScanner::lensingKernel(double ell)
{
    if (ell <= 1.0) return 0.0;
    return 1.0 / (ell * (ell + 1.0));
}

} // namespace quantumverse
