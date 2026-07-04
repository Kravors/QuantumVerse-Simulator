/**
 * @file GalacticTidalStreamCartographer.cpp
 * @brief Implementation of the tidal stream dark matter cartographer
 */

#include "GalacticTidalStreamCartographer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

GalacticTidalStreamCartographer::GalacticTidalStreamCartographer()
{
    setParameter("stream_distance_kpc", 10.0);
    setParameter("velocity_dispersion_kms", 10.0);
    setParameter("subhalo_mass_min_msun", 1e6);
    setParameter("subhalo_mass_max_msun", 1e9);
    setParameter("gap_detection_sigma", 3.0);
}

std::vector<InstrumentFinding> GalacticTidalStreamCartographer::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double streamDist = getParameter("stream_distance_kpc");
    double velDisp = getParameter("velocity_dispersion_kms");
    double minMass = getParameter("subhalo_mass_min_msun");
    double maxMass = getParameter("subhalo_mass_max_msun");

    if (trajectory.size() < 6) return findings;

    // Analyze trajectory for tidal stream perturbations
    std::vector<double> streamDensities;
    std::vector<double> distances;

    for (size_t i = 0; i < trajectory.size(); ++i) {
        double r = std::sqrt(trajectory[i].x * trajectory[i].x +
                            trajectory[i].y * trajectory[i].y +
                            trajectory[i].z * trajectory[i].z);
        distances.push_back(r);

        // Simulate stellar density along stream
        double width = computeStreamWidth(r, velDisp);
        double density = 1.0 / (width + 1e-10);

        // Add perturbation from potential subhalo
        double perturbation = 0.0;
        for (size_t j = 0; j < trajectory.size(); ++j) {
            if (i == j) continue;
            double dx = trajectory[i].x - trajectory[j].x;
            double dy = trajectory[i].y - trajectory[j].y;
            double dz = trajectory[i].z - trajectory[j].z;
            double d = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (d > 0.01 && d < 1.0) {
                perturbation += 1.0 / (d * d + 0.01);
            }
        }

        density *= (1.0 + 0.1 * perturbation);
        streamDensities.push_back(density);
    }

    // Detect gap structures indicating subhalo encounters
    bool gapDetected = detectGapStructure(streamDensities);

    if (gapDetected) {
        // Estimate subhalo mass from stream deflection
        double maxDeflection = 0.0;
        for (size_t i = 1; i < streamDensities.size(); ++i) {
            double defl = std::abs(streamDensities[i] - streamDensities[i - 1]);
            if (defl > maxDeflection) maxDeflection = defl;
        }

        double subhaloMass = estimateSubhaloMass(maxDeflection, streamDist);
        subhaloMass = std::max(minMass, std::min(maxMass, subhaloMass));

        double confidence = std::min(1.0, maxDeflection * 10.0);

        InstrumentFinding finding;
        finding.id = "GTSC_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Tidal stream gap detected: possible dark matter subhalo "
            "with estimated mass " + std::to_string(subhaloMass) + " Msun at "
            + std::to_string(streamDist) + " kpc, density perturbation="
            + std::to_string(maxDeflection);
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["subhalo_mass_msun"] = subhaloMass;
        finding.parameters["stream_distance_kpc"] = streamDist;
        finding.parameters["max_density_perturbation"] = maxDeflection;
        finding.parameters["gap_detected"] = 1.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double GalacticTidalStreamCartographer::computeStreamWidth(
    double distance, double velocityDispersion)
{
    // Stream width ~ velocity_dispersion * orbital_period / (2*pi*distance)
    double orbitalPeriod = 2.0 * M_PI * std::sqrt(distance * distance * distance); // simplified
    double width = velocityDispersion * orbitalPeriod / (2.0 * M_PI * distance + 1e-10);
    return width;
}

double GalacticTidalStreamCartographer::estimateSubhaloMass(
    double streamDeflection, double distance)
{
    // M_subhalo ~ deflection * distance^2 / G
    // Simplified: M ~ deflection * distance^2 * scaling
    double mass = streamDeflection * distance * distance * 1e6;
    return mass;
}

bool GalacticTidalStreamCartographer::detectGapStructure(
    const std::vector<double>& streamDensities)
{
    if (streamDensities.size() < 6) return false;

    double mean = 0.0;
    for (double d : streamDensities) mean += d;
    mean /= streamDensities.size();

    if (mean < 1e-10) return false;

    // Look for significant underdense regions (gaps)
    int gapCount = 0;
    for (double d : streamDensities) {
        if (d < mean * 0.5) gapCount++;
    }

    // Also check for consecutive underdense points
    int maxConsecutive = 0;
    int current = 0;
    for (double d : streamDensities) {
        if (d < mean * 0.6) {
            current++;
            if (current > maxConsecutive) maxConsecutive = current;
        } else {
            current = 0;
        }
    }

    return gapCount > static_cast<int>(streamDensities.size() * 0.2) || maxConsecutive >= 3;
}

} // namespace quantumverse