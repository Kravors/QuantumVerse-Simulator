/**
 * @file HawkingRadiationDetector.cpp
 * @brief Implementation of the Hawking radiation signature detector
 */

#include "HawkingRadiationDetector.h"

#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

HawkingRadiationDetector::HawkingRadiationDetector() {
    setParameter("mass", 1.0);       // geometric mass (length)
    setParameter("min_mass", 1e-6);  // plausibility lower bound
    setParameter("max_mass", 1e6);   // plausibility upper bound
}

std::string HawkingRadiationDetector::getDescription() const {
    return "Hawking radiation signature detector. Computes the Hawking "
           "temperature T = 1/(8πM), Bekenstein-Hawking entropy S = A/4, "
           "luminosity, and the 2D linear-dilaton asymptotic flux "
           "⟨T₋₋⟩ = 1/48 for a black hole of (geometric) mass M.";
}

std::map<std::string, std::pair<double, double>> HawkingRadiationDetector::getParameterRanges() const {
    return {
        {"mass", {getParameter("min_mass"), getParameter("max_mass")}}
    };
}

double HawkingRadiationDetector::hawkingTemperature(double M) {
    if (M <= 0.0) return 0.0;
    return 1.0 / (8.0 * M_PI * M);
}

double HawkingRadiationDetector::hawkingEntropy(double M) {
    if (M <= 0.0) return 0.0;
    // A = 4π r_s² = 4π (2M)² = 16π M² ; S = A/4 = 4π M²
    return 4.0 * M_PI * M * M;
}

double HawkingRadiationDetector::hawkingLuminosity(double M) {
    if (M <= 0.0) return 0.0;
    double rs = 2.0 * M;
    double sigma = M_PI * M_PI / 60.0;  // geometric Stefan–Boltzmann constant
    double T = hawkingTemperature(M);
    double area = 4.0 * M_PI * rs * rs;
    return area * sigma * T * T * T * T;
}

double HawkingRadiationDetector::hawkingFlux2D() {
    // Giddings (1994), linear-dilaton vacuum: asymptotic ⟨T₋₋⟩ = 1/48
    // (2D units, λ = 1).
    return 1.0 / 48.0;
}

std::vector<InstrumentFinding> HawkingRadiationDetector::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory) {
    (void)metric;
    (void)trajectory;

    double M = getParameter("mass");
    if (M <= 0.0) return {};

    double T = hawkingTemperature(M);
    double S = hawkingEntropy(M);
    double L = hawkingLuminosity(M);
    double f2d = hawkingFlux2D();
    double rs = 2.0 * M;

    InstrumentFinding finding;
    finding.id = "HRD_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(0.95);
    finding.confidence = 0.95;
    finding.description = "Hawking radiation signature for black hole of geometric "
        "mass " + std::to_string(M) + ": T = 1/(8πM), S = A/4, ⟨T₋₋⟩(2D) = 1/48.";
    finding.location = location;
    finding.timestamp = location.t;
    finding.isAnomaly = true;
    finding.parameters["mass_geometric"] = M;
    finding.parameters["horizon_radius"] = rs;
    finding.parameters["temperature"] = T;
    finding.parameters["entropy"] = S;
    finding.parameters["luminosity"] = L;
    finding.parameters["flux_2d"] = f2d;

    addFinding(finding);
    return {finding};
}

} // namespace quantumverse
