/**
 * @file GalacticRotationCurveScanner.cpp
 * @brief Implementation of the galactic rotation curve residual scanner
 */

#include "GalacticRotationCurveScanner.h"
#include <cmath>
#include <algorithm>
#include <utility>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

GalacticRotationCurveScanner::GalacticRotationCurveScanner()
{
    setParameter("kepler_fit_fraction", 0.15);
    setParameter("flattening_threshold", 0.2);
}

std::vector<InstrumentFinding> GalacticRotationCurveScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < 8) return findings;

    double innerFrac = getParameter("kepler_fit_fraction");
    double flatThreshold = getParameter("flattening_threshold");

    // Extract (radius, circular velocity) samples.
    // Convention: radius = sqrt(x^2 + y^2), circular velocity = z.
    std::vector<std::pair<double, double>> samples;
    for (const auto& ev : trajectory) {
        double r = std::sqrt(ev.x * ev.x + ev.y * ev.y);
        if (r > 1e-6) samples.emplace_back(r, ev.z);
    }
    if (samples.size() < 8) return findings;

    std::sort(samples.begin(), samples.end(),
        [](const std::pair<double, double>& a, const std::pair<double, double>& b) {
            return a.first < b.first;
        });

    size_t n = samples.size();
    size_t innerN = std::max<size_t>(3, static_cast<size_t>(innerFrac * n));
    if (innerN >= n) innerN = n / 2;

    // Fit a Keplerian model v^2 * r = GM (constant) over the inner region.
    double gmProxy = 0.0;
    for (size_t i = 0; i < innerN; ++i) {
        gmProxy += samples[i].second * samples[i].second * samples[i].first;
    }
    gmProxy /= innerN;
    if (gmProxy <= 0) return findings;

    // Scan the outer region for flattening: observed velocity well above the
    // Keplerian fall-off signals a dark-matter-dominated halo.
    double flattenRadius = 0.0;
    double maxRelResidual = 0.0;
    for (size_t i = innerN; i < n; ++i) {
        double r = samples[i].first;
        double vObs = samples[i].second;
        double vKep = std::sqrt(gmProxy / r);
        if (vKep <= 0) continue;
        double relResidual = (vObs - vKep) / vKep;
        if (relResidual > maxRelResidual) maxRelResidual = relResidual;
        if (relResidual > flatThreshold && flattenRadius == 0.0) {
            flattenRadius = r;
        }
    }

    if (flattenRadius > 0.0) {
        double confidence = std::min(1.0, maxRelResidual / (2.0 * flatThreshold));

        InstrumentFinding finding;
        finding.id = "GRC_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Rotation curve flattens at radius " +
            std::to_string(flattenRadius) + " with circular velocity exceeding the "
            "Keplerian fall-off by " + std::to_string(maxRelResidual * 100.0) +
            "%, indicating a dark matter halo signature.";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["flatten_radius"] = flattenRadius;
        finding.parameters["keplerian_gm"] = gmProxy;
        finding.parameters["max_relative_residual"] = maxRelResidual;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
