/**
 * @file test_merging_binary_inspiral_analyzer.cpp
 * @brief TDD tests for the MergingBinaryInspiralAnalyzer
 *
 * Verifies:
 *   - Chirp mass recovery from injected TaylorF2 signal
 *   - Mass ratio recovery
 *   - SNR cutoff for weak signals
 *   - Noise rejection
 *   - Robustness (NaN/Inf, empty, short data)
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/MergingBinaryInspiralAnalyzer.h"
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

double chirpFrequency(double t, double tc, double mc) {
    double tau = tc - t;
    if (tau <= 0.0) return 0.0;
    double mc58 = std::pow(mc, -5.0 / 8.0);
    double tau38 = std::pow(tau, -3.0 / 8.0);
    return (1.0 / (8.0 * kPi)) * mc58 * tau38;
}

std::vector<Event4D> makeInspiralWaveform(double chirpMass, double tc,
                                            double dt, size_t n,
                                            double noiseLevel = 0.0) {
    std::vector<Event4D> data;
    data.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double tau = tc - t;
        if (tau <= 0.0) {
            data.emplace_back(t, 0.0, 0.0, 0.0);
            continue;
        }
        double f = chirpFrequency(t, tc, chirpMass);
        double env = std::pow(tau / tc, -1.0 / 4.0);
        double h = env * std::sin(2.0 * kPi * f * t);
        double noise = noiseLevel * (static_cast<double>(rand()) / RAND_MAX - 0.5);
        data.emplace_back(t, h + noise, 0.0, 0.0);
    }
    return data;
}
} // namespace

int main() {
    std::cout << "=== MergingBinaryInspiralAnalyzerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Chirp mass recovery from injected TaylorF2 signal
    {
        MergingBinaryInspiralAnalyzer analyzer;
        double mcTrue = 10.0;
        double tc = 10.0;
        double dt = 0.001;
        auto traj = makeInspiralWaveform(mcTrue, tc, dt, 128, 0.01);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = !findings.empty();
        assert(ok && "Injected chirp signal should be detected");
        if (!findings.empty()) {
            double mcRecovered = findings[0].parameters.at("chirp_mass");
            double relErr = std::abs(mcRecovered - mcTrue) / mcTrue;
            [[maybe_unused]] bool okMc = relErr < 0.1;
            assert(okMc && "Recovered chirp mass should be within 10% of true value");
            [[maybe_unused]] bool okConf = findings[0].confidence > 0.5;
            assert(okConf && "Confidence should be reasonable");
        }
        std::cout << "  Chirp mass recovery: OK." << std::endl;
    }

    // 2. Mass ratio parameter present and in valid range
    {
        MergingBinaryInspiralAnalyzer analyzer;
        double mcTrue = 10.0;
        double tc = 10.0;
        double dt = 0.001;
        auto traj = makeInspiralWaveform(mcTrue, tc, dt, 128, 0.01);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = !findings.empty();
        assert(ok && "Signal should be detected");
        if (!findings.empty()) {
            double q = findings[0].parameters.at("mass_ratio");
            [[maybe_unused]] bool okQ = q >= 0.1 && q <= 1.0;
            assert(okQ && "Mass ratio should be in [0.1, 1.0]");
        }
        std::cout << "  Mass ratio recovery: OK." << std::endl;
    }

    // 3. Weak signal (high noise) should not trigger detection
    {
        MergingBinaryInspiralAnalyzer analyzer;
        double mcTrue = 10.0;
        double tc = 10.0;
        double dt = 0.001;
        auto traj = makeInspiralWaveform(mcTrue, tc, dt, 128, 5.0);
        auto findings = analyzer.analyze(metric, location, traj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "High-noise signal should not be detected");
        std::cout << "  SNR cutoff: OK." << std::endl;
    }

    // 4. Random noise should not trigger detection
    {
        MergingBinaryInspiralAnalyzer analyzer;
        std::vector<Event4D> noiseTraj;
        for (size_t i = 0; i < 128; ++i) {
            double t = static_cast<double>(i) * 0.001;
            double h = static_cast<double>(rand()) / RAND_MAX - 0.5;
            noiseTraj.emplace_back(t, h, 0.0, 0.0);
        }
        auto findings = analyzer.analyze(metric, location, noiseTraj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "Random noise should not be detected");
        std::cout << "  Noise rejection: OK." << std::endl;
    }

    // 5. Empty trajectory: must not crash
    {
        MergingBinaryInspiralAnalyzer analyzer;
        std::vector<Event4D> empty;
        auto findings = analyzer.analyze(metric, location, empty);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "Empty trajectory must yield no findings");
        std::cout << "  Empty trajectory handled." << std::endl;
    }

    // 6. NaN/Inf in waveform: must not crash
    {
        MergingBinaryInspiralAnalyzer analyzer;
        std::vector<Event4D> nanTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            nanTraj.emplace_back(t, kNaN, 0.0, 0.0);
        }
        auto findings = analyzer.analyze(metric, location, nanTraj);
        [[maybe_unused]] bool ok = findings.empty();
        assert(ok && "NaN waveform must yield no findings");

        std::vector<Event4D> infTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            infTraj.emplace_back(t, kInf, 0.0, 0.0);
        }
        auto findings2 = analyzer.analyze(metric, location, infTraj);
        [[maybe_unused]] bool ok2 = findings2.empty();
        assert(ok2 && "Inf waveform must yield no findings");
        std::cout << "  NaN/Inf handled." << std::endl;
    }

    // 7. Parameter ranges well-formed
    {
        MergingBinaryInspiralAnalyzer analyzer;
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

    std::cout << "  All MergingBinaryInspiralAnalyzer checks passed." << std::endl;
    return 0;
}
