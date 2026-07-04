/**
 * @file BosonStarCollisionPredictor.cpp
 * @brief Implementation of the boson star collision predictor
 */

#include "BosonStarCollisionPredictor.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

BosonStarCollisionPredictor::BosonStarCollisionPredictor()
{
    setParameter("boson_mass_ev", 1e-12);
    setParameter("compactness", 0.15);
    setParameter("tidal_deformability_threshold", 500.0);
    setParameter("mass_gap_lower_msun", 2.5);
    setParameter("mass_gap_upper_msun", 5.0);
}

std::vector<InstrumentFinding> BosonStarCollisionPredictor::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double bosonMass = getParameter("boson_mass_ev");
    double compactness = getParameter("compactness");
    double tidalThreshold = getParameter("tidal_deformability_threshold");
    double massGapLower = getParameter("mass_gap_lower_msun");
    double massGapUpper = getParameter("mass_gap_upper_msun");

    if (trajectory.size() < 4) return findings;

    // Analyze trajectory for binary inspiral signatures
    std::vector<double> separations;
    std::vector<double> orbitalFreqs;

    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dx = trajectory[i].x - trajectory[i - 1].x;
        double dy = trajectory[i].y - trajectory[i - 1].y;
        double dz = trajectory[i].z - trajectory[i - 1].z;
        double sep = std::sqrt(dx * dx + dy * dy + dz * dz);
        double dt = trajectory[i].t - trajectory[i - 1].t;

        if (sep > 0 && dt > 0) {
            separations.push_back(sep);
            double omega = 2.0 * M_PI / dt;
            orbitalFreqs.push_back(omega);
        }
    }

    if (separations.size() < 3) return findings;

    // Check for chirp mass evolution (frequency increasing with decreasing separation)
    bool chirpDetected = false;
    double chirpRate = 0.0;
    for (size_t i = 2; i < separations.size(); ++i) {
        if (separations[i] < separations[i - 1] && orbitalFreqs[i] > orbitalFreqs[i - 1]) {
            chirpDetected = true;
            double df = orbitalFreqs[i] - orbitalFreqs[i - 1];
            double dt_sep = separations[i - 1] - separations[i];
            if (dt_sep > 0) chirpRate += df / dt_sep;
        }
    }

    // Compute tidal deformability
    double tidalDeform = computeTidalDeformability(compactness, bosonMass);

    // Check for mass gap objects
    double totalMass = 0.0;
    for (size_t i = 0; i < trajectory.size(); ++i) {
        double r = std::sqrt(trajectory[i].x * trajectory[i].x +
                            trajectory[i].y * trajectory[i].y +
                            trajectory[i].z * trajectory[i].z);
        if (r > 0) totalMass += r * 0.5; // Proxy mass estimate
    }
    bool massGap = isMassGapObject(totalMass * 0.6, totalMass * 0.4);

    if (chirpDetected || tidalDeform > tidalThreshold || massGap) {
        double confidence = std::min(1.0, (chirpRate * 0.1 +
            (tidalDeform > tidalThreshold ? 0.3 : 0.0) +
            (massGap ? 0.4 : 0.0)));

        InstrumentFinding finding;
        finding.id = "BSCP_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Boson star collision candidate detected: "
            "chirp=" + std::string(chirpDetected ? "yes" : "no") +
            ", tidal_deformability=" + std::to_string(tidalDeform) +
            ", mass_gap_object=" + std::string(massGap ? "yes" : "no");
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["tidal_deformability"] = tidalDeform;
        finding.parameters["chirp_rate"] = chirpRate;
        finding.parameters["boson_mass_ev"] = bosonMass;
        finding.parameters["compactness"] = compactness;
        finding.parameters["is_mass_gap"] = massGap ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double BosonStarCollisionPredictor::computeTidalDeformability(
    double compactness, double bosonMass)
{
    // Tidal deformability Lambda ~ (2/3) * k2 * (R/M)^5
    // For boson stars: k2 ~ compactness-dependent
    double k2 = 0.75 * compactness * (1.0 - 2.0 * compactness) /
        (1.0 - compactness);
    double rOverM = 1.0 / compactness;
    double lambda = (2.0 / 3.0) * k2 * std::pow(rOverM, 5);

    // Scale by boson mass
    return lambda * std::pow(bosonMass, 2.0);
}

double BosonStarCollisionPredictor::estimateBosonMassFromRingdown(
    double fRing, double fDamp)
{
    // f_ring ~ c^3 / (G * M * 2pi) for fundamental mode
    // M = c^3 / (2pi * G * f_ring)
    // In natural units with boson mass correction
    double massEstimate = 1.0 / (2.0 * M_PI * fRing);
    double dampingCorrection = fDamp / fRing;

    return massEstimate * (1.0 + 0.5 * dampingCorrection);
}

bool BosonStarCollisionPredictor::isMassGapObject(double mass1, double mass2)
{
    // Neutron star upper limit ~ 2.5 Msun, BH lower limit ~ 5 Msun
    // Mass gap: 2.5 - 5 Msun
    double massGapLower = getParameter("mass_gap_lower_msun");
    double massGapUpper = getParameter("mass_gap_upper_msun");

    return (mass1 > massGapLower && mass1 < massGapUpper) ||
           (mass2 > massGapLower && mass2 < massGapUpper);
}

} // namespace quantumverse