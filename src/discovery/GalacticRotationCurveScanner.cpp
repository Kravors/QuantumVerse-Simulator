/**
 * @file GalacticRotationCurveScanner.cpp
 * @brief Implementation of the universal rotation curve residual scanner
 */

#include "GalacticRotationCurveScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

GalacticRotationCurveScanner::GalacticRotationCurveScanner()
{
    setParameter("nfw_virial_mass_solar", 1e12);
    setParameter("nfw_concentration", 10.0);
    setParameter("mond_acceleration_threshold", 1e-10);
    setParameter("residual_detection_sigma", 3.0);
}

std::vector<InstrumentFinding> GalacticRotationCurveScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double virialMass = getParameter("nfw_virial_mass_solar");
    double concentration = getParameter("nfw_concentration");
    double mondThreshold = getParameter("mond_acceleration_threshold");
    double residualSigma = getParameter("residual_detection_sigma");

    if (trajectory.size() < 8) {
        return findings;
    }

    // Sample radii from trajectory
    std::vector<double> radii;
    for (const auto& event : trajectory) {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        if (r > 0.01) radii.push_back(r);
    }

    if (radii.size() < 4) return findings;

    // Compute residuals between NFW prediction and MOND
    std::vector<double> residuals;
    for (double r : radii) {
        double nfwAccel = computeNFWPrediction(r, virialMass, concentration);
        double mondAccel = computeMONDAcceleration(nfwAccel, mondThreshold);
        residuals.push_back(std::abs(nfwAccel - mondAccel));
    }

    if (detectSystematicResidual(residuals)) {
        double maxResidual = *std::max_element(residuals.begin(), residuals.end());
        double meanResidual = 0.0;
        for (double r : residuals) meanResidual += r;
        meanResidual /= residuals.size();

        double confidence = std::min(1.0, meanResidual * 1e8);

        InstrumentFinding finding;
        finding.id = "GRC_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Modified gravity residual detected: NFW vs MOND discrepancy "
            "at galactocentric radii with mean residual " + std::to_string(meanResidual)
            + " suggesting f(R) or MOND-like modification";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["mean_residual"] = meanResidual;
        finding.parameters["max_residual"] = maxResidual;
        finding.parameters["virial_mass"] = virialMass;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double GalacticRotationCurveScanner::computeNFWPrediction(
    double radius, double virialMass, double concentration)
{
    // NFW profile: rho(r) = rho_s / [(r/r_s)(1 + r/r_s)^2]
    double rScale = virialMass / (4.0 * M_PI * 200.0 * concentration * concentration * concentration);
    double rho_s = virialMass / (4.0 * M_PI * rScale * rScale * rScale *
        (std::log(1.0 + concentration) - concentration / (1.0 + concentration)));

    double x = radius / rScale;
    if (x <= 0) return 0.0;

    // Enclosed mass
    double massEnclosed = 4.0 * M_PI * rho_s * rScale * rScale * rScale *
        (std::log(1.0 + x) - x / (1.0 + x));

    // Circular velocity
    return std::sqrt(massEnclosed / radius);
}

double GalacticRotationCurveScanner::computeMONDAcceleration(
    double newtonianAccel, double a0)
{
    // MOND interpolation function: mu(g/a0) * g = g_N
    // Simple form: g = sqrt(g_N * a0) for g_N << a0
    if (newtonianAccel <= 0) return 0.0;
    if (newtonianAccel >= a0) return newtonianAccel;

    return std::sqrt(newtonianAccel * a0);
}

bool GalacticRotationCurveScanner::detectSystematicResidual(
    const std::vector<double>& residuals)
{
    if (residuals.size() < 4) return false;

    double sum = 0.0;
    for (double r : residuals) sum += r;
    double mean = sum / residuals.size();

    if (mean < 1e-15) return false;

    // Check for monotonic trend (systematic, not random)
    int increasing = 0, decreasing = 0;
    for (size_t i = 1; i < residuals.size(); ++i) {
        if (residuals[i] > residuals[i - 1]) increasing++;
        else if (residuals[i] < residuals[i - 1]) decreasing++;
    }

    double trendRatio = static_cast<double>(std::max(increasing, decreasing)) / residuals.size();
    return trendRatio > 0.7 && mean > 1e-12;
}

} // namespace quantumverse