/**
 * @file test_sgwb_analyzer.cpp
 * @brief TDD tests for the Stochastic Gravitational-Wave Background (SGWB)
 *        analyzer
 *
 * Verifies:
 *   1. An injected SGWB signal (common to two detectors) IS flagged as an
 *      anomaly with significance above the sigma threshold.
 *   2. Pure, uncorrelated detector noise is correctly REJECTED (null case).
 *   3. Robustness to NaN/Inf strain values (skipped, never NaN findings).
 *   4. Too-few samples cannot crash or flag.
 *   5. A stronger SGWB amplitude yields a higher significance.
 *   6. Parameter ranges are well-formed and finite.
 *   7. The DiscoveryInstrument::analyze() override detects via a configured
 *      reference strain, and its single-detector fallback works.
 *
 * The underlying search is a two-detector cross-correlation (the standard
 * LIGO/Virgo/KAGRA stochastic-background statistic): the normalized
 * zero-lag cross-power r0 = C / sqrt(Pa Pb), whose significance under
 * independent noise has sigma = r0 * sqrt(N).
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "discovery/SGWBBackgroundAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {

constexpr double kPi = 3.14159265358979323846;

// Inject a stochastic background shared by two detectors, plus independent
// detector noise.  a and b thus share `amp * g_signal` but have uncorrelated
// noise `noise * g_noise`, so their cross-correlation reveals the common SGWB.
void fillSGWB(std::vector<double>& a, std::vector<double>& b,
              double amp, double noise, size_t n, unsigned seed) {
    std::mt19937 gs(seed), ga(seed + 1), gb(seed + 2);
    std::normal_distribution<double> g(0.0, 1.0);
    a.resize(n);
    b.resize(n);
    for (size_t i = 0; i < n; ++i) {
        double sig = amp * g(gs);
        a[i] = sig + noise * g(ga);
        b[i] = sig + noise * g(gb);
    }
}

// Two independent noise streams (no shared signal) -> must not correlate.
void fillNoise(std::vector<double>& a, std::vector<double>& b,
               double noise, size_t n, unsigned seed) {
    std::mt19937 ga(seed), gb(seed + 7);
    std::normal_distribution<double> g(0.0, 1.0);
    a.resize(n);
    b.resize(n);
    for (size_t i = 0; i < n; ++i) {
        a[i] = noise * g(ga);
        b[i] = noise * g(gb);
    }
}

// A slowly-varying common signal across the whole timeline, used to validate
// the single-detector (split-half) fallback path.
std::vector<Event4D> makeSlowCorrelatedTrajectory(size_t n = 2000, double dt = 0.1) {
    std::mt19937 gen(424242);
    std::normal_distribution<double> g(0.0, 1.0);
    std::vector<Event4D> traj;
    traj.reserve(n);
    const double period = 400.0; // samples; persists across the mid-point split
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double common = 0.8 * std::sin(2.0 * kPi * t / (period * dt));
        double x = common + 0.15 * g(gen);
        traj.emplace_back(t, x, 0.0, 0.0);
    }
    return traj;
}

} // namespace

int main() {
    std::cout << "=== SGWBBackgroundAnalyzerTest ===" << std::endl;
    MetricTensor metric;
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Injected SGWB (common to both detectors) must be detected.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> a, b;
        fillSGWB(a, b, 1.0, 0.3, 4000, 12345);
        auto findings = sgwb.analyzeStrains(a, b);
        std::cout << "  SGWB signal findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "SGWB signal must be detected");
        assert(findings.front().isAnomaly && "Detected SGWB must be flagged as anomaly");
        double sig = findings.front().parameters.at("significance_sigma");
        double r0 = findings.front().parameters.at("cross_correlation");
        std::cout << "    significance=" << sig << " sigma, r0=" << r0 << std::endl;
        assert(sig > 3.0 && "Significance must exceed the 3-sigma threshold");
        assert(r0 > 0.5 && "Normalized cross-power must be substantial");
    }

    // 2. Pure, uncorrelated noise must NOT be flagged.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> a, b;
        fillNoise(a, b, 1.0, 8000, 999);
        auto findings = sgwb.analyzeStrains(a, b);
        double nullSig = findings.empty() ? 0.0 : findings.front().parameters.at("significance_sigma");
        std::cout << "  Null noise findings: " << findings.size() << " sig=" << nullSig << std::endl;
        if (!findings.empty()) {
            std::cerr << "FAIL: uncorrelated noise must NOT be flagged (sig=" << nullSig << ")\n";
            return 1;
        }
    }

    // 3. Robustness: NaN / Inf strain values are skipped, findings stay finite.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> a, b;
        fillSGWB(a, b, 1.0, 0.3, 4000, 22222);
        a[3] = std::numeric_limits<double>::quiet_NaN();
        b[7] = std::numeric_limits<double>::infinity();
        auto findings = sgwb.analyzeStrains(a, b);
        for ([[maybe_unused]] const auto& f : findings) {
            assert(std::isfinite(f.confidence));
            assert(std::isfinite(f.parameters.at("significance_sigma")));
        }
        std::cout << "  NaN/Inf robustness findings: " << findings.size() << std::endl;
    }

    // 4. Too few samples cannot crash or flag.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> a(5, 0.1), b(5, 0.1);
        auto findings = sgwb.analyzeStrains(a, b);
        assert(findings.empty() && "Too-few-sample input must not flag");
    }

    // 5. Stronger SGWB amplitude yields higher significance.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> aw, bw, as, bs;
        fillSGWB(aw, bw, 0.5, 0.3, 4000, 555);
        fillSGWB(as, bs, 2.0, 0.3, 4000, 555);
        auto weak = sgwb.analyzeStrains(aw, bw);
        auto strong = sgwb.analyzeStrains(as, bs);
        double sw = weak.empty() ? 0.0 : weak.front().parameters.at("significance_sigma");
        double ss = strong.empty() ? 0.0 : strong.front().parameters.at("significance_sigma");
        std::cout << "  weak sig=" << sw << " strong sig=" << ss << std::endl;
        assert(ss > sw && "Stronger SGWB must give higher significance");
    }

    // 6. Parameter ranges are well-formed and finite.
    {
        SGWBBackgroundAnalyzer sgwb;
        auto ranges = sgwb.getParameterRanges();
        assert(!ranges.empty() && "Parameter ranges must be defined");
        for ([[maybe_unused]] const auto& kv : ranges) {
            assert(std::isfinite(kv.second.first) && std::isfinite(kv.second.second));
            assert(kv.second.second > kv.second.first && "Range max must exceed min");
        }
    }

    // 7a. analyze() override detects via a configured reference strain.
    {
        SGWBBackgroundAnalyzer sgwb;
        std::vector<double> a, b;
        fillSGWB(a, b, 1.0, 0.3, 4000, 777);
        std::vector<Event4D> traj;
        traj.reserve(a.size());
        for (size_t i = 0; i < a.size(); ++i) traj.emplace_back(static_cast<double>(i), a[i], 0.0, 0.0);
        sgwb.setReferenceStrain(b);
        auto findings = sgwb.analyze(metric, location, traj);
        std::cout << "  analyze(reference) findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "analyze() with reference strain must detect SGWB");
    }

    // 7b. analyze() single-detector fallback (split-half) detects a signal
    //     whose correlation persists across the mid-point split.
    {
        SGWBBackgroundAnalyzer sgwb;
        auto traj = makeSlowCorrelatedTrajectory();
        auto findings = sgwb.analyze(metric, location, traj);
        std::cout << "  analyze(fallback) findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "analyze() fallback must detect persistent correlated signal");
    }

    std::cout << "All SGWBBackgroundAnalyzerTest checks passed." << std::endl;
    return 0;
}
