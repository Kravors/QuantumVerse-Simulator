/**
 * @file test_kerr_no_hair_violation_analyzer.cpp
 * @brief TDD tests for the KerrNoHairViolationAnalyzer
 *
 * Verifies:
 *   - Static Kerr QNM frequency functions return finite values
 *   - A Kerr-consistent ringdown (single dominant mode) yields no violation
 *   - An injected spin mismatch between waveform halves triggers a violation
 *   - Empty/short/NaN/Inf inputs are handled robustly
 *   - Parameter ranges are well-formed
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/KerrNoHairViolationAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kPi = 3.14159265358979323846;

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

std::vector<Event4D> makeKerrConsistentWaveform(double freq, double dt, size_t n) {
    std::vector<Event4D> data;
    data.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double env = std::exp(-0.5 * t);
        double h = env * std::sin(2.0 * kPi * freq * t);
        data.emplace_back(t, h, 0.0, 0.0);
    }
    return data;
}

std::vector<Event4D> makeNoHairViolationWaveform(double freq1, double freq2,
                                                   double dt, size_t n) {
    std::vector<Event4D> data;
    data.reserve(n);
    size_t half = n / 2;
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double env = std::exp(-0.5 * t);
        double f = (i < half) ? freq1 : freq2;
        double h = env * std::sin(2.0 * kPi * f * t);
        data.emplace_back(t, h, 0.0, 0.0);
    }
    return data;
}
} // namespace

int main() {
    std::cout << "=== KerrNoHairViolationAnalyzerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Static QNM functions return finite values
    for (double a : {0.0, 0.3, 0.6, 0.9}) {
        [[maybe_unused]] bool ok =
            std::isfinite(KerrNoHairViolationAnalyzer::kerrQNM220Real(a)) &&
            std::isfinite(KerrNoHairViolationAnalyzer::kerrQNM220Imag(a)) &&
            std::isfinite(KerrNoHairViolationAnalyzer::kerrQNM221Real(a)) &&
            std::isfinite(KerrNoHairViolationAnalyzer::kerrQNM221Imag(a));
        assert(ok && "Kerr QNM functions must return finite values");
    }
    std::cout << "  Static QNM functions: finite." << std::endl;

    // 2. Kerr-consistent ringdown (single mode): no violation
    {
        KerrNoHairViolationAnalyzer analyzer;
        double dt = 0.01;
        auto traj = makeKerrConsistentWaveform(0.5, dt, 64);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "Kerr-consistent ringdown should not trigger violation");
        std::cout << "  Kerr-consistent ringdown: no violation." << std::endl;
    }

    // 3. No-hair violation (frequency mismatch between halves): should flag
    {
        KerrNoHairViolationAnalyzer analyzer;
        double dt = 0.01;
        auto traj = makeNoHairViolationWaveform(0.3, 0.8, dt, 64);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = !findings.empty();
        assert(ok && "Frequency mismatch should trigger a violation finding");
        if (!findings.empty()) {
            [[maybe_unused]] bool okAnomaly = findings[0].isAnomaly;
            assert(okAnomaly && "Violation finding must be flagged as anomaly");
            [[maybe_unused]] bool okConf = findings[0].confidence > 0.0 && findings[0].confidence <= 1.0;
            assert(okConf && "Confidence must be in (0, 1]");
            [[maybe_unused]] bool okSigma = findings[0].parameters.at("deviation_sigma") > 0.0;
            assert(okSigma && "Deviation sigma must be positive");
        }
        std::cout << "  No-hair violation detected." << std::endl;
    }

    // 4. Empty trajectory: must not crash
    {
        KerrNoHairViolationAnalyzer analyzer;
        std::vector<Event4D> empty;
        auto findings = analyzer.analyze(metric, location, empty);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "Empty trajectory must yield no findings");
        std::cout << "  Empty trajectory handled." << std::endl;
    }

    // 5. Short trajectory (below min_samples): must not crash
    {
        KerrNoHairViolationAnalyzer analyzer;
        auto traj = makeKerrConsistentWaveform(0.5, 0.01, 4);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "Short trajectory must yield no findings");
        std::cout << "  Short trajectory handled." << std::endl;
    }

    // 6. NaN/Inf in waveform: must not crash
    {
        KerrNoHairViolationAnalyzer analyzer;
        std::vector<Event4D> nanTraj;
        for (size_t i = 0; i < 32; ++i) {
            double t = static_cast<double>(i) * 0.01;
            nanTraj.emplace_back(t, kNaN, 0.0, 0.0);
        }
        auto findings = analyzer.analyze(metric, location, nanTraj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "NaN waveform must yield no findings");

        std::vector<Event4D> infTraj;
        for (size_t i = 0; i < 32; ++i) {
            double t = static_cast<double>(i) * 0.01;
            infTraj.emplace_back(t, kInf, 0.0, 0.0);
        }
        auto findings2 = analyzer.analyze(metric, location, infTraj);
        [[maybe_unused]] bool ok2 = findings2.empty();
        assert(ok2 && "Inf waveform must yield no findings");
        std::cout << "  NaN/Inf handled." << std::endl;
    }

    // 7. Parameter ranges well-formed
    {
        KerrNoHairViolationAnalyzer analyzer;
        auto ranges = analyzer.getParameterRanges();
        [[maybe_unused]] bool ok = ranges.size() >= 3;
        assert(ok && "Must have at least 3 parameter ranges");
        for ([[maybe_unused]] const auto& kv : ranges) {
            [[maybe_unused]] bool okRange = std::isfinite(kv.second.first) &&
                                           std::isfinite(kv.second.second) &&
                                           kv.second.first < kv.second.second;
            assert(okRange && "Range bounds must be finite and ordered");
        }
        std::cout << "  Parameter ranges valid." << std::endl;
    }

    std::cout << "  All KerrNoHairViolationAnalyzer checks passed." << std::endl;
    return 0;
}
