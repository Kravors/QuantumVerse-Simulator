/**
 * @file test_dm_annihilation_analyzer.cpp
 * @brief TDD tests for the Dark Matter Annihilation Signal analyzer
 *
 * Verifies:
 *   1. A monochromatic line injected on top of a smooth power-law photon
 *      spectrum IS flagged as an anomaly with significance above threshold.
 *   2. A pure power-law background (no line) is correctly REJECTED (null).
 *   3. Robustness to NaN/Inf count values (skipped, findings stay finite).
 *   4. Too-few bins cannot crash or flag.
 *   5. A stronger annihilation line yields a higher significance.
 *   6. Parameter ranges are well-formed and finite.
 *   7. The DiscoveryInstrument::analyze() override scans the mass range and
 *      detects the line via the trajectory-encoded spectrum.
 *
 * The search fits a smooth power-law background (excluding a candidate line
 * region) and measures the excess counts in that region; the detection
 * significance is excess / sqrt(background) in the region (a Poisson-style
 * sigma), flagged when it exceeds the configured threshold (default 3.0).
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "discovery/DarkMatterAnnihilationAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {

// Build a smooth power-law photon spectrum N(E) = norm * E^(-index) over a
// grid of energies, then optionally inject a Gaussian monochromatic line at
// `lineEnergy` with the given per-bin amplitude (the annihilation signal).
std::vector<double> makeSpectrum(std::vector<double>& energies,
                                 double norm, double index, size_t nBins,
                                 double dE, double lineEnergy, double lineAmp,
                                 unsigned seed) {
    energies.resize(nBins);
    std::vector<double> counts(nBins);
    std::mt19937 gen(seed);
    std::normal_distribution<double> g(0.0, 1.0);
    const double sigmaLine = lineEnergy * 0.04; // narrow line
    for (size_t i = 0; i < nBins; ++i) {
        double E = 1.0 + static_cast<double>(i) * dE;
        energies[i] = E;
        double bg = norm * std::pow(E, -index);
        double c = bg + 0.03 * bg * g(gen); // small photon-count noise
        if (lineAmp > 0.0 && sigmaLine > 0.0) {
            c += lineAmp * std::exp(-0.5 * std::pow((E - lineEnergy) / sigmaLine, 2.0));
        }
        if (c < 0.0) c = 0.0;
        counts[i] = c;
    }
    return counts;
}

} // namespace

int main() {
    std::cout << "=== DarkMatterAnnihilationAnalyzerTest ===" << std::endl;
    MetricTensor metric;
    Event4D location(0.0, 0.0, 0.0, 0.0);

    const double norm = 1000.0, index = 1.5, dE = 0.5;
    const size_t nBins = 200;
    const double lineEnergy = 30.0;

    // 1. Injected DM line must be detected.
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> E, C;
        C = makeSpectrum(E, norm, index, nBins, dE, lineEnergy, 8.0, 12345);
        auto findings = dma.analyzeSpectrum(E, C, lineEnergy);
        std::cout << "  DM line findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "DM annihilation line must be detected");
        assert(findings.front().isAnomaly && "Detected line must be flagged as anomaly");
        double sig = findings.front().parameters.at("significance_sigma");
        double le = findings.front().parameters.at("line_energy");
        std::cout << "    significance=" << sig << " sigma, line_energy=" << le << std::endl;
        assert(sig > 3.0 && "Significance must exceed the 3-sigma threshold");
        assert(std::fabs(le - lineEnergy) < 5.0 && "Recovered line energy must be near injected value");
    }

    // 2. Pure power-law background must NOT be flagged (null case).
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> E, C;
        C = makeSpectrum(E, norm, index, nBins, dE, lineEnergy, 0.0, 999);
        auto findings = dma.analyzeSpectrum(E, C, lineEnergy);
        double nullSig = findings.empty() ? 0.0 : findings.front().parameters.at("significance_sigma");
        std::cout << "  Null background findings: " << findings.size() << " sig=" << nullSig << std::endl;
        if (!findings.empty()) {
            std::cerr << "FAIL: pure power-law background must NOT be flagged (sig=" << nullSig << ")\n";
            return 1;
        }
    }

    // 3. Robustness: NaN / Inf count values are skipped, findings stay finite.
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> E, C;
        C = makeSpectrum(E, norm, index, nBins, dE, lineEnergy, 8.0, 22222);
        C[20] = std::numeric_limits<double>::quiet_NaN();
        C[40] = std::numeric_limits<double>::infinity();
        auto findings = dma.analyzeSpectrum(E, C, lineEnergy);
        bool allFinite = true;
        for (const auto& f : findings) {
            allFinite = allFinite
                && std::isfinite(f.confidence)
                && std::isfinite(f.parameters.at("significance_sigma"));
        }
        assert(allFinite && "NaN/Inf counts must be skipped so findings stay finite");
        std::cout << "  NaN/Inf robustness findings: " << findings.size()
                  << " allFinite=" << (allFinite ? 1 : 0) << std::endl;
    }

    // 4. Too few bins cannot crash or flag.
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> E(5, 1.0), C(5, 2.0);
        auto findings = dma.analyzeSpectrum(E, C, 1.0);
        assert(findings.empty() && "Too-few-bin input must not flag");
    }

    // 5. Stronger annihilation line yields higher significance.
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> Ew, Cw, Es, Cs;
        Cw = makeSpectrum(Ew, norm, index, nBins, dE, lineEnergy, 4.0, 555);
        Cs = makeSpectrum(Es, norm, index, nBins, dE, lineEnergy, 14.0, 555);
        auto weak = dma.analyzeSpectrum(Ew, Cw, lineEnergy);
        auto strong = dma.analyzeSpectrum(Es, Cs, lineEnergy);
        double sw = weak.empty() ? 0.0 : weak.front().parameters.at("significance_sigma");
        double ss = strong.empty() ? 0.0 : strong.front().parameters.at("significance_sigma");
        std::cout << "  weak sig=" << sw << " strong sig=" << ss << std::endl;
        assert(ss > sw && "Stronger line must give higher significance");
    }

    // 6. Parameter ranges are well-formed and finite.
    {
        DarkMatterAnnihilationAnalyzer dma;
        auto ranges = dma.getParameterRanges();
        bool rangesOk = !ranges.empty();
        for (const auto& kv : ranges) {
            rangesOk = rangesOk
                && std::isfinite(kv.second.first) && std::isfinite(kv.second.second)
                && (kv.second.second > kv.second.first);
        }
        assert(rangesOk && "Parameter ranges must be well-formed and finite");
        std::cout << "  parameter ranges ok=" << (rangesOk ? 1 : 0) << std::endl;
    }

    // 7. analyze() override scans the mass range and detects the line.
    {
        DarkMatterAnnihilationAnalyzer dma;
        std::vector<double> E, C;
        C = makeSpectrum(E, norm, index, nBins, dE, lineEnergy, 8.0, 777);
        std::vector<Event4D> traj;
        traj.reserve(E.size());
        for (size_t i = 0; i < E.size(); ++i) traj.emplace_back(E[i], C[i], 0.0, 0.0); // t=E, x=counts
        auto findings = dma.analyze(metric, location, traj);
        std::cout << "  analyze(scan) findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "analyze() must detect the line via mass-range scan");
    }

    std::cout << "All DarkMatterAnnihilationAnalyzerTest checks passed." << std::endl;
    return 0;
}
