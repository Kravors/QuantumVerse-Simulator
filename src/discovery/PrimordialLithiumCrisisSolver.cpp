/**
 * @file PrimordialLithiumCrisisSolver.cpp
 * @brief Implementation of the primordial lithium crisis solver
 */

#include "PrimordialLithiumCrisisSolver.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

PrimordialLithiumCrisisSolver::PrimordialLithiumCrisisSolver()
{
    setParameter("baryon_to_photon", 6.1e-10);
    setParameter("neutron_lifetime_s", 880.0);
    setParameter("dm_cross_section_cm2", 1e-36);
    setParameter("li7_observed_ratio", 1.6e-10);
    setParameter("be7_observed_ratio", 1.0e-10);
}

std::vector<InstrumentFinding> PrimordialLithiumCrisisSolver::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double eta = getParameter("baryon_to_photon");
    double neutronLife = getParameter("neutron_lifetime_s");
    double dmCrossSection = getParameter("dm_cross_section_cm2");
    double li7Observed = getParameter("li7_observed_ratio");
    double be7Observed = getParameter("be7_observed_ratio");

    if (trajectory.size() < 3) return findings;

    // Simulate BBN nucleosynthesis with varying parameters
    // Use trajectory time as a proxy for varying physical conditions
    std::vector<double> predictedLi7, predictedBe7;
    std::vector<double> observedAbundances = {li7Observed, be7Observed};

    for (size_t i = 0; i < trajectory.size(); ++i) {
        double t = trajectory[i].t;
        // Vary effective baryon-to-photon ratio with trajectory position
        double etaEff = eta * (1.0 + 0.01 * std::sin(t * 0.1));
        double nLifeEff = neutronLife * (1.0 + 0.005 * t);

        // Simplified BBN lithium production
        double li7 = computeLithiumAbundance(etaEff, nLifeEff, dmCrossSection);
        double be7 = li7 * 0.15; // Be-7 is ~15% of Li-7 in standard BBN

        predictedLi7.push_back(li7);
        predictedBe7.push_back(be7);
    }

    double chi2Li7 = computeBBNChiSquared(
        {li7Observed}, {predictedLi7[trajectory.size() / 2]});
    double chi2Be7 = computeBBNChiSquared(
        {be7Observed}, {predictedBe7[trajectory.size() / 2]});

    bool crisisResolved = isLithiumCrisisResolved(
        predictedLi7.back(), predictedBe7.back());

    if (crisisResolved || chi2Li7 > 9.0 || chi2Be7 > 9.0) {
        double confidence = std::min(1.0,
            (crisisResolved ? 0.5 : 0.0) +
            std::min(0.3, chi2Li7 / 30.0) +
            std::min(0.2, chi2Be7 / 30.0));

        InstrumentFinding finding;
        finding.id = "PLCS_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Primordial lithium crisis analysis: Li-7/H=" +
            std::to_string(predictedLi7.back()) +
            " (observed: " + std::to_string(li7Observed) +
            "), chi2_Li7=" + std::to_string(chi2Li7) +
            ", crisis_resolved=" + std::string(crisisResolved ? "yes" : "no");
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["li7_predicted"] = predictedLi7.back();
        finding.parameters["be7_predicted"] = predictedBe7.back();
        finding.parameters["chi2_li7"] = chi2Li7;
        finding.parameters["chi2_be7"] = chi2Be7;
        finding.parameters["eta_eff"] = eta * (1.0 + 0.01 * std::sin(trajectory.back().t * 0.1));
        finding.parameters["crisis_resolved"] = crisisResolved ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double PrimordialLithiumCrisisSolver::computeLithiumAbundance(
    double baryonToPhoton, double neutronLifetime, double dmCrossSection)
{
    // Simplified BBN Li-7 abundance calculation
    // Li-7/H ~ eta^alpha * exp(-E_bind/kT_BBN) * f(dm_cross_section)
    double alpha = 0.75;
    double bindingEnergy = 5.47e6; // eV (Li-7 binding)
    double tBBN = 1e9; // K (BBN temperature ~1 GK)
    double kBBN = 8.617e-5 * tBBN; // eV

    double abundance = std::pow(baryonToPhoton, alpha) *
        std::exp(-bindingEnergy / kBBN) *
        (1.0 + dmCrossSection * 1e36); // DM enhancement factor

    return abundance;
}

double PrimordialLithiumCrisisSolver::computeBBNChiSquared(
    const std::vector<double>& observedAbundances,
    const std::vector<double>& predictedAbundances)
{
    if (observedAbundances.size() != predictedAbundances.size()) return 0.0;

    double chi2 = 0.0;
    for (size_t i = 0; i < observedAbundances.size(); ++i) {
        if (predictedAbundances[i] > 0) {
            double diff = observedAbundances[i] - predictedAbundances[i];
            chi2 += diff * diff / (predictedAbundances[i] * predictedAbundances[i]);
        }
    }

    return chi2;
}

bool PrimordialLithiumCrisisSolver::isLithiumCrisisResolved(
    double li7OverH, double be7OverH)
{
    // Standard BBN predicts Li-7/H ~ 4.2e-10, observations show ~1.6e-10
    // A resolution requires predicted values within 2 sigma of observations
    double li7Observed = 1.6e-10;
    double li7Sigma = 0.3e-10;
    double be7Observed = 1.0e-10;
    double be7Sigma = 0.2e-10;

    bool li7Resolved = std::abs(li7OverH - li7Observed) < 2.0 * li7Sigma;
    bool be7Resolved = std::abs(be7OverH - be7Observed) < 2.0 * be7Sigma;

    return li7Resolved && be7Resolved;
}

} // namespace quantumverse