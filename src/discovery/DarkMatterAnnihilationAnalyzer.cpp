/**
 * @file DarkMatterAnnihilationAnalyzer.cpp
 * @brief Implementation of the Dark Matter Annihilation Signal analyzer
 */

#include "DarkMatterAnnihilationAnalyzer.h"

#include <cmath>
#include <algorithm>
#include <vector>

namespace quantumverse {

namespace {

// Linear least-squares fit of y = b + m*x. Returns false if ill-conditioned.
bool fitLine(const std::vector<double>& x, const std::vector<double>& y,
             double& b, double& m) {
    const size_t n = x.size();
    if (n < 2) return false;
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sx += x[i];
        sy += y[i];
        sxx += x[i] * x[i];
        sxy += x[i] * y[i];
    }
    const double denom = static_cast<double>(n) * sxx - sx * sx;
    if (std::fabs(denom) < 1e-12) return false;
    m = (static_cast<double>(n) * sxy - sx * sy) / denom;
    b = (sy - m * sx) / static_cast<double>(n);
    return true;
}

} // anonymous namespace

DarkMatterAnnihilationAnalyzer::DarkMatterAnnihilationAnalyzer() {
    setParameter("significance_threshold", 3.0); // detection sigma
    setParameter("line_width_fraction", 0.15);    // line region = +/- 15% in energy
    setParameter("mass_min", 1.0);                // scan lower bound (GeV)
    setParameter("mass_max", 100.0);              // scan upper bound (GeV)
    setParameter("annihilation_cross_section", 1.0); // model amplitude (informational)
}

double DarkMatterAnnihilationAnalyzer::computeLineSignificance(
        const std::vector<double>& energies, const std::vector<double>& counts,
        double lineEnergy, double widthFrac) const {
    const size_t N = std::min(energies.size(), counts.size());
    if (N < kMinBins || !(lineEnergy > 0.0) || !(widthFrac > 0.0)) return 0.0;

    // Collect valid bins (finite, positive energy, non-negative counts), and a
    // separate set for the background fit that excludes the candidate line.
    std::vector<double> fx, fy; // background-fit points (log-log)
    std::vector<size_t> lineIdx;
    for (size_t i = 0; i < N; ++i) {
        const double E = energies[i];
        const double C = counts[i];
        if (!std::isfinite(E) || !std::isfinite(C) || E <= 0.0 || C < 0.0) continue;
        const bool inLine = std::fabs(E - lineEnergy) / lineEnergy <= widthFrac;
        if (inLine) {
            lineIdx.push_back(i);
        } else if (C > 0.0) {
            fx.push_back(std::log(E));
            fy.push_back(std::log(C + 1e-9));
        }
    }
    if (lineIdx.empty()) return 0.0;
    if (fx.size() < 2) return 0.0;

    double b = 0.0, m = 0.0;
    if (!fitLine(fx, fy, b, m)) return 0.0;
    const double A = std::exp(b);
    const double gamma = -m; // model: C = A * E^(-gamma)

    double excess = 0.0, varExp = 0.0;
    for (size_t i : lineIdx) {
        const double E = energies[i];
        const double exp_i = A * std::pow(E, -gamma);
        if (!std::isfinite(exp_i) || exp_i < 0.0) continue;
        excess += counts[i] - exp_i;
        varExp += exp_i;
    }
    if (!(varExp > 0.0) || !std::isfinite(excess) || !std::isfinite(varExp)) return 0.0;

    const double sigma = excess / std::sqrt(varExp);
    return sigma > 0.0 ? sigma : 0.0;
}

std::vector<InstrumentFinding> DarkMatterAnnihilationAnalyzer::analyzeSpectrum(
        const std::vector<double>& energies, const std::vector<double>& counts,
        double lineEnergy) {
    const double widthFrac = getParameter("line_width_fraction");
    const double threshold = getParameter("significance_threshold");
    const double sigma = computeLineSignificance(energies, counts, lineEnergy, widthFrac);
    if (sigma < threshold) return {};

    InstrumentFinding finding;
    finding.id = "DMA_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    const double confidence = std::min(1.0, sigma / 6.0);
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.isAnomaly = true;
    finding.description = "Dark-matter annihilation line detected at E=" +
                          std::to_string(lineEnergy) + " GeV: excess counts above the "
                          "fitted power-law background at " + std::to_string(sigma) +
                          " sigma, exceeding the " + std::to_string(threshold) +
                          "-sigma threshold. Consistent with a monochromatic gamma-ray "
                          "line from WIMP annihilation (Fermi-LAT / CTA style search).";
    finding.parameters["line_energy"] = lineEnergy;
    finding.parameters["significance_sigma"] = sigma;
    finding.parameters["threshold"] = threshold;
    finding.parameters["line_width_fraction"] = widthFrac;
    finding.parameters["n_bins"] = static_cast<double>(std::min(energies.size(), counts.size()));
    addFinding(finding);
    return {finding};
}

std::vector<InstrumentFinding> DarkMatterAnnihilationAnalyzer::analyze(
        const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory) {
    (void)metric;
    std::vector<double> E, C;
    E.reserve(trajectory.size());
    C.reserve(trajectory.size());
    for (const auto& pt : trajectory) {
        if (std::isfinite(pt.t) && std::isfinite(pt.x) && pt.t > 0.0 && pt.x >= 0.0) {
            E.push_back(pt.t); // energy
            C.push_back(pt.x);  // photon counts
        }
    }
    if (E.size() < kMinBins) return {};

    const double massMin = getParameter("mass_min");
    const double massMax = getParameter("mass_max");
    const double threshold = getParameter("significance_threshold");
    const double widthFrac = getParameter("line_width_fraction");

    // Scan candidate line energies across the mass range (use the actual
    // energy grid points that fall inside it) and keep the most significant.
    double bestSigma = 0.0, bestEnergy = massMin;
    for (size_t i = 0; i < E.size(); ++i) {
        const double cand = E[i];
        if (cand < massMin || cand > massMax) continue;
        const double s = computeLineSignificance(E, C, cand, widthFrac);
        if (s > bestSigma) {
            bestSigma = s;
            bestEnergy = cand;
        }
    }
    (void)location;
    if (bestSigma < threshold) return {};

    InstrumentFinding finding;
    finding.id = "DMA_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    const double confidence = std::min(1.0, bestSigma / 6.0);
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.isAnomaly = true;
    finding.description = "Dark-matter annihilation line detected at E=" +
                          std::to_string(bestEnergy) + " GeV via mass-range scan: excess "
                          "counts above the fitted power-law background at " +
                          std::to_string(bestSigma) + " sigma, exceeding the " +
                          std::to_string(threshold) + "-sigma threshold.";
    finding.parameters["line_energy"] = bestEnergy;
    finding.parameters["significance_sigma"] = bestSigma;
    finding.parameters["threshold"] = threshold;
    finding.parameters["mass_min"] = massMin;
    finding.parameters["mass_max"] = massMax;
    finding.parameters["n_bins"] = static_cast<double>(E.size());
    addFinding(finding);
    return {finding};
}

std::map<std::string, std::pair<double, double>> DarkMatterAnnihilationAnalyzer::getParameterRanges() const {
    return {
        {"significance_threshold", {1.0, 10.0}},
        {"line_width_fraction", {0.02, 0.5}},
        {"mass_min", {0.1, 50.0}},
        {"mass_max", {50.0, 1000.0}},
        {"annihilation_cross_section", {0.01, 100.0}}
    };
}

} // namespace quantumverse
