/**
 * @file FineStructureConstantDriftObservatory.cpp
 * @brief Implementation of the alpha-drift observatory
 */

#include "FineStructureConstantDriftObservatory.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

FineStructureConstantDriftObservatory::FineStructureConstantDriftObservatory()
{
    setParameter("alpha_variation_limit", 1e-6);
    setParameter("redshift_range_max", 5.0);
    setParameter("num_quasar_sightlines", 100);
    setParameter("spectral_resolution", 50000.0);
}

std::vector<InstrumentFinding> FineStructureConstantDriftObservatory::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double alphaLimit = getParameter("alpha_variation_limit");
    double zMax = getParameter("redshift_range_max");
    double resolution = getParameter("spectral_resolution");

    if (trajectory.size() < 3) return findings;

    // Simulate quasar absorption line measurements at different redshifts
    std::vector<double> alphaMeasurements;
    std::vector<double> redshifts;

    for (size_t i = 0; i < trajectory.size(); ++i) {
        double z = trajectory[i].t * 0.1; // Simplified redshift proxy
        if (z > zMax) break;

        // Simulate alpha measurement with potential drift
        double deltaAlpha = alphaLimit * (1.0 + 0.1 * std::sin(z * 2.0));
        double measuredAlpha = 1.0 / 137.035999084 + deltaAlpha;

        alphaMeasurements.push_back(measuredAlpha);
        redshifts.push_back(z);
    }

    if (alphaMeasurements.size() < 4) return findings;

    // Fit linear drift: alpha(z) = alpha0 + alpha0 * q * z
    double sumZ = 0.0, sumA = 0.0, sumZ2 = 0.0, sumZA = 0.0;
    size_t n = alphaMeasurements.size();
    for (size_t i = 0; i < n; ++i) {
        sumZ += redshifts[i];
        sumA += alphaMeasurements[i];
        sumZ2 += redshifts[i] * redshifts[i];
        sumZA += redshifts[i] * alphaMeasurements[i];
    }

    double denom = n * sumZ2 - sumZ * sumZ;
    if (std::abs(denom) < 1e-30) return findings;

    double slope = (n * sumZA - sumZ * sumA) / denom;
    double alpha0 = (sumA - slope * sumZ) / n;

    // Compute residuals and chi-squared
    double chi2 = 0.0;
    std::vector<double> residuals;
    for (size_t i = 0; i < n; ++i) {
        double expected = alpha0 * (1.0 + slope / alpha0 * redshifts[i]);
        double res = alphaMeasurements[i] - expected;
        residuals.push_back(res);
        chi2 += res * res;
    }

    if (isDriftSignificant(alphaMeasurements, redshifts)) {
        double scalarMass = constrainScalarFieldMass(residuals);
        double confidence = std::min(1.0, std::abs(slope) * 1e10);

        InstrumentFinding finding;
        finding.id = "FSCD_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Fine-structure constant drift detected: dα/dz = "
            + std::to_string(slope) + " suggesting scalar field coupling. "
            + "Scalar field mass constraint: " + std::to_string(scalarMass) + " eV";
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["alpha_slope"] = slope;
        finding.parameters["alpha_0"] = alpha0;
        finding.parameters["scalar_field_mass_eV"] = scalarMass;
        finding.parameters["chi_squared"] = chi2;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double FineStructureConstantDriftObservatory::computeAlphaVariation(
    double redshift, double deltaAlphaOverAlpha)
{
    // Webb et al. parametrization: α(z) = α0(1 + qα z)
    double alpha0 = 1.0 / 137.035999084;
    return alpha0 * (1.0 + deltaAlphaOverAlpha * redshift);
}

bool FineStructureConstantDriftObservatory::isDriftSignificant(
    const std::vector<double>& alphaMeasurements,
    const std::vector<double>& redshifts)
{
    if (alphaMeasurements.size() < 5) return false;

    // Compute weighted correlation between alpha and redshift
    double sumZ = 0.0, sumA = 0.0, sumZ2 = 0.0, sumZA = 0.0;
    size_t n = alphaMeasurements.size();
    for (size_t i = 0; i < n; ++i) {
        sumZ += redshifts[i];
        sumA += alphaMeasurements[i];
        sumZ2 += redshifts[i] * redshifts[i];
        sumZA += redshifts[i] * alphaMeasurements[i];
    }

    double r = (n * sumZA - sumZ * sumA) /
        std::sqrt((n * sumZ2 - sumZ * sumZ) * (n * sumA * sumA / n - sumA * sumA / n));

    return std::abs(r) > 0.7;
}

double FineStructureConstantDriftObservatory::constrainScalarFieldMass(
    const std::vector<double>& residuals)
{
    // From Bekenstein-type models: m_phi ~ Δα/α × H0
    double rmsResidual = 0.0;
    for (double r : residuals) rmsResidual += r * r;
    rmsResidual = std::sqrt(rmsResidual / residuals.size());

    double hubbleConstant = 67.4; // km/s/Mpc
    double mass_eV = rmsResidual * 1e10 * hubbleConstant;

    return mass_eV;
}

} // namespace quantumverse