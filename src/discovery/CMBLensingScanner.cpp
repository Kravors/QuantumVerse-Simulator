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

    // Significance is the maximum *fractional* deviation of the observed C_l from
    // the ΛCDM prediction. Normalising by the residual RMS (as before) was
    // degenerate: because both the clean noise and a true excess scale with C_l,
    // the resulting sigma was ~sqrt(N) for any input and a 900% excess could not
    // be distinguished from the clean trajectory. Using the fractional deviation
    // makes the detection sensitive to the actual anomaly amplitude.
    double maxFracRes = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double predicted = theoreticalCl(ells[i]);
        if (predicted <= 0.0) continue;
        double frac = std::abs(cls[i] - predicted) / predicted;
        if (std::isnan(frac) || std::isinf(frac)) continue;
        maxFracRes = std::max(maxFracRes, frac);
    }

    double significance = maxFracRes;
    if (significance < 1e-30) return findings;

    if (significance > sigThreshold) {
        double confidence = std::min(1.0, significance / (2.0 * sigThreshold));

        InstrumentFinding finding;
        finding.id = "CMBLENS_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "CMB lensing anomaly detected: max fractional "
            "deviation from ΛCDM C_l = " + std::to_string(significance) +
            " (threshold = " + std::to_string(sigThreshold) + "). Potential "
            "dark-matter / neutrino-mass deviation from ΛCDM.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["max_fractional_deviation"] = significance;
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
