/**
 * @file ExoplanetaryTTVFifthForceHunter.cpp
 * @brief Implementation of the TTV Fifth-Force Hunter
 */

#include "ExoplanetaryTTVFifthForceHunter.h"
#include <cmath>
#include <algorithm>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

ExoplanetaryTTVFifthForceHunter::ExoplanetaryTTVFifthForceHunter()
{
    setParameter("yukawa_range_au", 0.1);
    setParameter("yukawa_strength", 1e-6);
    setParameter("detection_threshold_sigma", 3.0);
    setParameter("min_planet_mass_earth", 0.5);
}

std::vector<InstrumentFinding> ExoplanetaryTTVFifthForceHunter::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    // Simulate TTV analysis: compute residuals from Keplerian prediction
    double yukawaRange = getParameter("yukawa_range_au");
    double yukawaStrength = getParameter("yukawa_strength");
    double thresholdSigma = getParameter("detection_threshold_sigma");

    if (trajectory.size() < 4) {
        return findings;
    }

    // Compute timing residuals along trajectory
    std::vector<double> residuals;
    std::vector<double> distances;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        double dx = trajectory[i].x - trajectory[i - 1].x;
        double dy = trajectory[i].y - trajectory[i - 1].y;
        double dz = trajectory[i].z - trajectory[i - 1].z;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        distances.push_back(dist);

        // Expected Keplerian transit time vs observed
        double expectedDt = dt * (1.0 + yukawaStrength * std::exp(-dist / yukawaRange));
        residuals.push_back(std::abs(dt - expectedDt));
    }

    if (isAnomalousPattern(residuals, thresholdSigma)) {
        double strength = estimateFifthForceStrength(residuals, distances);

        InstrumentFinding finding;
        finding.id = "TTV_5F_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(strength);
        finding.confidence = strength;
        finding.description = "Anomalous TTV residuals detected: possible Yukawa fifth force "
            "with strength " + std::to_string(strength) + " at range "
            + std::to_string(yukawaRange) + " AU";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["yukawa_strength"] = strength;
        finding.parameters["yukawa_range_au"] = yukawaRange;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double ExoplanetaryTTVFifthForceHunter::computeTTVResidual(
    double observedTransitTime, double predictedTransitTime,
    double orbitalPeriod, double semiMajorAxis)
{
    // Simple model: residual = observed - predicted
    // with Yukawa correction factor
    double correction = std::exp(-semiMajorAxis / getParameter("yukawa_range_au"));
    double predictedWithCorrection = predictedTransitTime * (1.0 + getParameter("yukawa_strength") * correction);
    return std::abs(observedTransitTime - predictedWithCorrection);
}

bool ExoplanetaryTTVFifthForceHunter::isAnomalousPattern(
    const std::vector<double>& residuals, double threshold)
{
    if (residuals.size() < 3) return false;

    double sum = 0.0;
    for (double r : residuals) sum += r;
    double mean = sum / residuals.size();

    double variance = 0.0;
    for (double r : residuals) variance += (r - mean) * (r - mean);
    variance /= residuals.size();
    double stddev = std::sqrt(variance);

    // Check if residuals exceed threshold sigma
    int aboveThreshold = 0;
    for (double r : residuals) {
        if (stddev > 0 && std::abs(r - mean) / stddev > threshold) {
            aboveThreshold++;
        }
    }

    return aboveThreshold > static_cast<int>(residuals.size() * 0.3);
}

double ExoplanetaryTTVFifthForceHunter::estimateFifthForceStrength(
    const std::vector<double>& residuals,
    const std::vector<double>& distances)
{
    if (distances.empty()) return 0.0;

    double sumWeighted = 0.0;
    double sumWeights = 0.0;
    for (size_t i = 0; i < residuals.size() && i < distances.size(); ++i) {
        double w = 1.0 / (distances[i] + 1e-10);
        sumWeighted += residuals[i] * w;
        sumWeights += w;
    }

    double weightedMean = sumWeights > 0 ? sumWeighted / sumWeights : 0.0;
    // Normalize to [0, 1] confidence
    return std::min(1.0, weightedMean * 100.0);
}

} // namespace quantumverse