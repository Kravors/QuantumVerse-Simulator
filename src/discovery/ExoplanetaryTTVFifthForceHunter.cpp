/**
 * @file ExoplanetaryTTVFifthForceHunter.cpp
 * @brief Implementation of the TTV Fifth-Force Hunter
 *
 * Searches for deviations of observed transit times from a linear Keplerian
 * ephemeris (T_n = T0 + n*P) that could indicate a Yukawa-type fifth force
 * between the planet and its host star. A statistically significant residual
 * pattern (above the timing-noise floor by the configured sigma threshold)
 * produces a finding whose strength is estimated order-of-magnitude as the
 * timing-amplitude fraction of the orbital period: alpha ~ max|residual| / P.
 */

#include "ExoplanetaryTTVFifthForceHunter.h"
#include "PlanetaryGridGenerator.h"
#include <cmath>
#include <algorithm>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

ExoplanetaryTTVFifthForceHunter::ExoplanetaryTTVFifthForceHunter()
{
    setParameter("orbital_period", 10.0);    // days (synthetic default)
    setParameter("semi_major_axis", 0.1);    // AU
    setParameter("timing_noise", 1e-4);      // timing uncertainty floor (days)
    setParameter("detection_threshold_sigma", 3.0);
    setParameter("yukawa_range_au", 0.5);    // lambda of the Yukawa fifth force
}

std::vector<InstrumentFinding> ExoplanetaryTTVFifthForceHunter::analyzeGrid(
    const std::vector<PlanetaryGridResult>& gridResults)
{
    std::vector<InstrumentFinding> findings;
    if (gridResults.empty()) return findings;

    double strength = estimateStrengthFromGrid(gridResults);
    double yukawaRange = getParameter("yukawa_range_au");

    if (strength > 1e-6) {
        InstrumentFinding finding;
        finding.id = "TTV_5F_GRID_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(strength);
        finding.confidence = strength;
        finding.description = "Grid-based TTV analysis: fifth-force signature strength "
            + std::to_string(strength) + " across " + std::to_string(gridResults.size())
            + " orbital configurations";
        finding.location = gridResults.front().trajectory.empty()
                           ? Event4D() : gridResults.front().trajectory.front();
        finding.timestamp = 0.0;
        finding.parameters["fifth_force_strength"] = strength;
        finding.parameters["yukawa_range_au"] = yukawaRange;
        finding.parameters["grid_points"] = static_cast<double>(gridResults.size());
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double ExoplanetaryTTVFifthForceHunter::estimateStrengthFromGrid(
    const std::vector<PlanetaryGridResult>& gridResults)
{
    if (gridResults.empty()) return 0.0;

    double sumStrength = 0.0;
    size_t anomalyCount = 0;
    for (const auto& r : gridResults) {
        if (r.anomalyDetected) {
            sumStrength += r.fifthForceStrength;
            anomalyCount++;
        }
    }

    if (anomalyCount == 0) return 0.0;
    // Mean anomalous strength, down-weighted by the fraction of anomalous
    // configurations across the full grid.
    double meanAnomalous = sumStrength / static_cast<double>(anomalyCount);
    double fraction = static_cast<double>(anomalyCount) /
                      static_cast<double>(gridResults.size());
    return std::min(1.0, meanAnomalous * fraction);
}

std::vector<InstrumentFinding> ExoplanetaryTTVFifthForceHunter::analyze(
    const MetricTensor& /*metric*/, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < 4) return findings;

    double P = getParameter("orbital_period");
    double noise = getParameter("timing_noise");
    double sigmaThreshold = getParameter("detection_threshold_sigma");
    double yukawaRange = getParameter("yukawa_range_au");

    if (P <= 0.0) return findings;

    // Evaluate residuals against a linear Keplerian ephemeris.
    double t0 = trajectory.front().t;
    std::vector<double> residuals;
    std::vector<double> distances;
    for (size_t n = 0; n < trajectory.size(); ++n) {
        double observed = trajectory[n].t;
        double predicted = t0 + static_cast<double>(n) * P;
        residuals.push_back(computeTTVResidual(observed, predicted, P,
            std::sqrt(trajectory[n].x * trajectory[n].x +
                      trajectory[n].y * trajectory[n].y +
                      trajectory[n].z * trajectory[n].z)));
        distances.push_back(std::sqrt(trajectory[n].x * trajectory[n].x +
                                      trajectory[n].y * trajectory[n].y +
                                      trajectory[n].z * trajectory[n].z));
    }

    double rms = 0.0;
    for (double r : residuals) rms += r * r;
    rms = std::sqrt(rms / static_cast<double>(residuals.size()));

    double detectionFloor = sigmaThreshold * noise;
    if (rms <= detectionFloor) return findings;

    double strength = estimateFifthForceStrength(residuals, distances);
    double confidence = std::min(1.0, rms / detectionFloor);

    InstrumentFinding finding;
    finding.id = "TTV_5F_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Anomalous TTV residuals detected: possible Yukawa fifth force "
        "with strength " + std::to_string(strength) + " at range "
        + std::to_string(yukawaRange) + " AU (RMS residual " + std::to_string(rms)
        + " vs floor " + std::to_string(detectionFloor) + ")";
    finding.location = location;
    finding.timestamp = location.t;
    finding.parameters["fifth_force_strength"] = strength;
    finding.parameters["ttv_rms_residual"] = rms;
    finding.parameters["detection_floor"] = detectionFloor;
    finding.parameters["yukawa_range_au"] = yukawaRange;
    finding.parameters["orbital_period"] = P;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

double ExoplanetaryTTVFifthForceHunter::computeTTVResidual(
    double observedTransitTime, double predictedTransitTime,
    double /*orbitalPeriod*/, double /*semiMajorAxis*/)
{
    // Timing residual relative to the linear Keplerian ephemeris.
    return observedTransitTime - predictedTransitTime;
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
    double rms = std::sqrt(variance);

    // Treat `threshold` as the RMS residual floor (e.g. sigma_threshold * noise).
    return rms > threshold;
}

double ExoplanetaryTTVFifthForceHunter::estimateFifthForceStrength(
    const std::vector<double>& residuals,
    const std::vector<double>& distances)
{
    (void)distances;
    if (residuals.empty()) return 0.0;

    // Order-of-magnitude Yukawa strength: timing amplitude / orbital period.
    double maxAbs = 0.0;
    for (double r : residuals) {
        maxAbs = std::max(maxAbs, std::abs(r));
    }
    double P = getParameter("orbital_period");
    if (P <= 0.0) return 0.0;
    double strength = maxAbs / P;
    return std::min(1.0, strength);
}

} // namespace quantumverse
