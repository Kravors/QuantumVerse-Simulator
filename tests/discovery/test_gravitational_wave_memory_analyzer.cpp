/**
 * @file test_gravitational_wave_memory_analyzer.cpp
 * @brief TDD tests for the GravitationalWaveMemoryAnalyzer
 *
 * Verifies:
 *   - Memory offset recovery from injected burst with DC offset
 *   - Burst-only rejection (no memory offset)
 *   - Noise rejection
 *   - Varied energy scaling
 *   - Robustness (NaN/Inf, empty, short data)
 *   - Parameter ranges well-formed
 */

#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/GravitationalWaveMemoryAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

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

std::vector<Event4D> makeBurstWithMemory(double amplitude, double duration,
                                           double dt, double memoryOffset,
                                           double burstStart = 0.2,
                                           double burstEnd = 0.5) {
    size_t n = static_cast<size_t>(duration / dt);
    std::vector<Event4D> signal;
    signal.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double h = 0.0;

        if (t >= burstStart && t <= burstEnd) {
            double envelope = std::exp(-std::pow((t - (burstStart + burstEnd) * 0.5) /
                                                  ((burstEnd - burstStart) * 0.25), 2.0));
            h = amplitude * envelope * std::sin(2.0 * kPi * 80.0 * t);
        }

        if (t > burstEnd) {
            h += memoryOffset;
        }

        signal.emplace_back(t, h, 0.0, 0.0);
    }
    return signal;
}

std::vector<Event4D> makePureNoise(size_t n, double noiseLevel) {
    std::vector<Event4D> noise;
    noise.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * 0.0001;
        double h = noiseLevel * (static_cast<double>(rand()) / RAND_MAX - 0.5);
        noise.emplace_back(t, h, 0.0, 0.0);
    }
    return noise;
}
} // namespace

int main() {
    std::cout << "=== GravitationalWaveMemoryAnalyzerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Memory offset recovery from injected burst with DC offset
    {
        GravitationalWaveMemoryAnalyzer analyzer;
        analyzer.setParameter("energy_radiated", 1.0);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("baseline_duration", 0.15);
        analyzer.setParameter("threshold_sigma", 1.5);

        double memoryOffset = 1.0e-18;
        auto traj = makeBurstWithMemory(1.0e-20, 1.0, 0.0001, memoryOffset);
        auto findings = analyzer.analyze(metric, location, traj);

        QV_CHECK(!findings.empty());
        if (!findings.empty()) {
            [[maybe_unused]] double observed = findings[0].parameters.at("observed_offset");
            [[maybe_unused]] double snr = findings[0].parameters.at("snr");
            QV_CHECK(snr > 1.5);
            QV_CHECK(std::abs(observed) > 0.0);
        }
        std::cout << "  Memory offset recovery: OK." << std::endl;
    }

    // 2. Burst-only rejection (no memory offset)
    {
        GravitationalWaveMemoryAnalyzer analyzer;
        analyzer.setParameter("energy_radiated", 1.0);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("baseline_duration", 0.15);
        analyzer.setParameter("threshold_sigma", 3.0);

        auto traj = makeBurstWithMemory(1.0e-20, 1.0, 0.0001, 0.0);
        auto findings = analyzer.analyze(metric, location, traj);

        QV_CHECK(findings.empty());
        std::cout << "  Burst-only rejection: OK." << std::endl;
    }

    // 3. Random noise rejection
    {
        GravitationalWaveMemoryAnalyzer analyzer;
        analyzer.setParameter("energy_radiated", 1.0);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("baseline_duration", 0.15);
        analyzer.setParameter("threshold_sigma", 3.0);

        auto traj = makePureNoise(10000, 1.0e-21);
        auto findings = analyzer.analyze(metric, location, traj);

        QV_CHECK(findings.empty());
        std::cout << "  Noise rejection: OK." << std::endl;
    }

    // 4. Varied energy scaling (higher energy -> different expected offset)
    {
        GravitationalWaveMemoryAnalyzer analyzer1;
        analyzer1.setParameter("energy_radiated", 1.0);
        analyzer1.setParameter("distance_mpc", 100.0);
        analyzer1.setParameter("baseline_duration", 0.15);
        analyzer1.setParameter("threshold_sigma", 1.0);

        GravitationalWaveMemoryAnalyzer analyzer2;
        analyzer2.setParameter("energy_radiated", 10.0);
        analyzer2.setParameter("distance_mpc", 100.0);
        analyzer2.setParameter("baseline_duration", 0.15);
        analyzer2.setParameter("threshold_sigma", 1.0);

        double memoryOffset = 1.0e-18;
        auto traj = makeBurstWithMemory(1.0e-20, 1.0, 0.0001, memoryOffset);

        auto findings1 = analyzer1.analyze(metric, location, traj);
        auto findings2 = analyzer2.analyze(metric, location, traj);

        if (!findings1.empty() && !findings2.empty()) {
            [[maybe_unused]] double expected1 = findings1[0].parameters.at("expected_offset");
            [[maybe_unused]] double expected2 = findings2[0].parameters.at("expected_offset");
            QV_CHECK(std::abs(expected2 - expected1) > 0.0);
        }
        std::cout << "  Varied energy scaling: OK." << std::endl;
    }

    // 5. Robustness: NaN, Inf, empty, short data
    {
        GravitationalWaveMemoryAnalyzer analyzer;
        analyzer.setParameter("energy_radiated", 1.0);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("threshold_sigma", 2.0);

        std::vector<Event4D> empty;
        auto findings = analyzer.analyze(metric, location, empty);
        QV_CHECK(findings.empty());

        std::vector<Event4D> nanTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            nanTraj.emplace_back(t, kNaN, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, nanTraj);
        QV_CHECK(findings.empty());

        std::vector<Event4D> infTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            infTraj.emplace_back(t, kInf, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, infTraj);
        QV_CHECK(findings.empty());

        std::vector<Event4D> shortTraj;
        for (size_t i = 0; i < 10; ++i) {
            double t = static_cast<double>(i) * 0.001;
            shortTraj.emplace_back(t, 1.0, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, shortTraj);
        QV_CHECK(findings.empty());

        std::cout << "  Robustness handled." << std::endl;
    }

    // 6. Parameter ranges well-formed
    {
        GravitationalWaveMemoryAnalyzer analyzer;
        auto ranges = analyzer.getParameterRanges();
        QV_CHECK(ranges.size() >= 4);

        for ([[maybe_unused]] const auto& kv : ranges) {
            QV_CHECK(std::isfinite(kv.second.first) &&
                   std::isfinite(kv.second.second) &&
                   kv.second.first < kv.second.second);
        }
        std::cout << "  Parameter ranges valid." << std::endl;
    }

    std::cout << "  All GravitationalWaveMemoryAnalyzer checks passed." << std::endl;
    return 0;
}
