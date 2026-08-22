/**
 * @file test_eco_ringdown_analyzer.cpp
 * @brief TDD tests for the Exotic Compact Object (ECO) ringdown analyzer
 *
 * Verifies:
 *   1. An ECO echo-train signal IS flagged as an anomaly.
 *   2. A pure Kerr black-hole ringdown is correctly REJECTED (no echo).
 *   3. Robustness to too-few samples and NaN/Inf strain values.
 *   4. Parameter ranges are well-formed.
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/ECORingdownAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {

// Synthetic Kerr BH ringdown: a single damped sinusoid, no echoes.
std::vector<Event4D> makeKerrRingdown(size_t n = 300, double dt = 0.1) {
    std::vector<Event4D> traj;
    traj.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double h = std::exp(-t / 12.0) * std::cos(2.0 * t);
        traj.emplace_back(t, h, 0.0, 0.0);
    }
    return traj;
}

// Synthetic ECO ringdown: Kerr-like ringdown PLUS a delayed, repeated echo
// pulse (the horizonless-compact-object signature).
std::vector<Event4D> makeECORingdown(size_t n = 300, double dt = 0.1,
                                     double echoDelaySamples = 60.0,
                                     double echoAmp = 0.8) {
    std::vector<Event4D> traj;
    traj.reserve(n);
    double tau = 12.0;
    double omega = 2.0;
    double delay = echoDelaySamples * dt;
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double h = std::exp(-t / tau) * std::cos(omega * t);
        if (t >= delay) {
            h += echoAmp * std::exp(-(t - delay) / tau) * std::cos(omega * (t - delay));
        }
        traj.emplace_back(t, h, 0.0, 0.0);
    }
    return traj;
}

} // namespace

int main() {
    std::cout << "=== ECORingdownAnalyzerTest ===" << std::endl;
    MetricTensor metric;
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. ECO echo train must be detected as an anomaly.
    {
        ECORingdownAnalyzer eco;
        eco.setParameter("echo_threshold", 0.05);
        auto findings = eco.analyze(metric, location, makeECORingdown());
        std::cout << "  ECO signal findings: " << findings.size() << std::endl;
        assert(!findings.empty() && "ECO echo train must be detected");
        assert(findings.front().isAnomaly && "Detected ECO must be flagged as anomaly");
        assert(findings.front().confidence > 0.05 && "ECO confidence must exceed threshold");
        auto it = findings.front().parameters.find("echo_significance");
        assert(it != findings.front().parameters.end() && it->second > 0.05);
    }

    // 2. Pure Kerr BH ringdown must NOT be flagged.
    {
        ECORingdownAnalyzer eco;
        eco.setParameter("echo_threshold", 0.05);
        auto findings = eco.analyze(metric, location, makeKerrRingdown());
        double kerrSig = findings.empty() ? 0.0 : findings.front().parameters.at("echo_significance");
        std::cout << "  Kerr signal findings: " << findings.size() << " sig=" << kerrSig << std::endl;
        if (!findings.empty()) {
            std::cerr << "FAIL: pure Kerr ringdown must NOT be flagged as ECO (sig=" << kerrSig << ")\n";
            return 1;
        }
    }

    // 3. Robustness: too few samples.
    {
        ECORingdownAnalyzer eco;
        std::vector<Event4D> tiny = makeECORingdown(10);
        auto findings = eco.analyze(metric, location, tiny);
        assert(findings.empty() && "Too-few-sample input must not crash or flag");
    }

    // 3b. Robustness: NaN / Inf strain values are skipped.
    {
        ECORingdownAnalyzer eco;
        auto traj = makeECORingdown();
        traj[5].x = std::numeric_limits<double>::quiet_NaN();
        traj[10].x = std::numeric_limits<double>::infinity();
        auto findings = eco.analyze(metric, location, traj);
        // Either rejected (too few valid) or, if still enough, must not be NaN.
        for (const auto& f : findings) {
            assert(std::isfinite(f.confidence));
        }
    }

    // 4. Parameter ranges are well-formed and finite.
    {
        ECORingdownAnalyzer eco;
        auto ranges = eco.getParameterRanges();
        assert(!ranges.empty() && "Parameter ranges must be defined");
        for (const auto& kv : ranges) {
            assert(std::isfinite(kv.second.first) && std::isfinite(kv.second.second));
            assert(kv.second.second > kv.second.first && "Range max must exceed min");
        }
    }

    // 5. Stronger echo amplitude yields higher significance.
    {
        ECORingdownAnalyzer eco;
        eco.setParameter("echo_threshold", 0.01);
        auto weak = eco.analyze(metric, location, makeECORingdown(300, 0.1, 60.0, 0.3));
        auto strong = eco.analyze(metric, location, makeECORingdown(300, 0.1, 60.0, 0.9));
        double sw = weak.empty() ? 0.0 : weak.front().parameters.at("echo_significance");
        double ss = strong.empty() ? 0.0 : strong.front().parameters.at("echo_significance");
        std::cout << "  weak sig=" << sw << " strong sig=" << ss << std::endl;
        assert(ss >= sw && "Stronger echo must give >= significance");
    }

    std::cout << "All ECORingdownAnalyzerTest checks passed." << std::endl;
    return 0;
}
