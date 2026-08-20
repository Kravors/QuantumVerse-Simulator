// QuantumVerse Test: GWEchoHunter
// TDD validation of post-ringdown gravitational-wave echo detection.
// Verifies GR-consistent ringdown (no echo), injected horizon-echo anomaly,
// empty input, NaN/Inf robustness, and suppression of tiny deviations.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/GWEchoHunter.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

// Pure GR ringdown strain: h(t) = A0 * exp(-t/tau) * cos(2*pi*f*t)
double ringdown(double t, double a0, double tau, double f) {
    return a0 * std::exp(-t / tau) * std::cos(2.0 * kPi * f * t);
}

std::vector<Event4D> makePureRingdown() {
    std::vector<Event4D> data;
    data.reserve(60);
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double h = ringdown(t, 1.0, 0.8, 1.2);
        data.emplace_back(t, h, 0.0, 0.0);
    }
    return data;
}

std::vector<Event4D> makeEchoRingdown() {
    std::vector<Event4D> data;
    data.reserve(60);
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double h = ringdown(t, 1.0, 0.8, 1.2);
        // Horizon echo: delayed, damped replica peaking at t = 0.8
        double dt = t - 0.8;
        h += 0.9 * std::exp(-(dt * dt) / (2.0 * 0.04 * 0.04));
        data.emplace_back(t, h, 0.0, 0.0);
    }
    return data;
}

std::vector<Event4D> makeNoisyRingdown() {
    std::vector<Event4D> data;
    data.reserve(60);
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double h = ringdown(t, 1.0, 0.8, 1.2) + 0.01 * std::sin(7.0 * t);
        data.emplace_back(t, h, 0.0, 0.0);
    }
    return data;
}
} // namespace

int main() {
    std::cout << "=== GWEchoHunter Test ===" << std::endl;

    MetricTensor metric;
    metric.g[0][0] = -1.0; metric.g[1][1] = 1.0;
    metric.g[2][2] = 1.0; metric.g[3][3] = 1.0;
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // --- 1. GR-consistent ringdown: no echo expected -----------------------
    {
        GWEchoHunter hunter;
        auto findings = hunter.analyze(metric, location, makePureRingdown());
        assert(findings.empty() && "Pure ringdown must not trigger echo anomaly");
        std::cout << "  Pure ringdown: no echo (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Horizon echo injected: should be flagged ------------------------
    {
        GWEchoHunter hunter;
        auto findings = hunter.analyze(metric, location, makeEchoRingdown());
        assert(!findings.empty() && "Echo ringdown should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].isAnomaly);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        std::cout << "  Echo ringdown: confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ----------------
    {
        GWEchoHunter hunter;
        std::vector<Event4D> empty;
        auto findings = hunter.analyze(metric, location, empty);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. Too-few samples: must not crash, returns empty ------------------
    {
        GWEchoHunter hunter;
        std::vector<Event4D> small;
        for (int i = 0; i < 5; ++i) small.emplace_back(i * 0.05, 0.1, 0.0, 0.0);
        auto findings = hunter.analyze(metric, location, small);
        assert(findings.empty());
        std::cout << "  Small trajectory handled safely." << std::endl;
    }

    // --- 5. NaN/Inf observations: must not crash ----------------------------
    {
        GWEchoHunter hunter;
        auto traj = makePureRingdown();
        traj[10] = Event4D(kNaN, traj[10].x, 0.0, 0.0);
        auto f1 = hunter.analyze(metric, location, traj);
        assert(f1.empty());
        std::cout << "  NaN observation handled safely." << std::endl;

        auto traj2 = makePureRingdown();
        traj2[20] = Event4D(traj2[20].t, kInf, 0.0, 0.0);
        auto f2 = hunter.analyze(metric, location, traj2);
        assert(f2.empty());
        std::cout << "  Inf observation handled safely." << std::endl;
    }

    // --- 6. Tiny deviations: should not trigger anomaly --------------------
    {
        GWEchoHunter hunter;
        auto findings = hunter.analyze(metric, location, makeNoisyRingdown());
        assert(findings.empty() && "Tiny noise should not trigger echo anomaly");
        std::cout << "  Tiny deviations suppressed correctly." << std::endl;
    }

    // --- 7. Parameter ranges are sensible -----------------------------------
    {
        GWEchoHunter hunter;
        auto ranges = hunter.getParameterRanges();
        assert(ranges.count("echo_delay") > 0);
        assert(ranges.count("echo_ratio_threshold") > 0);
        assert(ranges.count("ringdown_fraction") > 0);
        assert(ranges["echo_delay"].first < ranges["echo_delay"].second);
        assert(ranges["echo_ratio_threshold"].first < ranges["echo_ratio_threshold"].second);
        assert(ranges["ringdown_fraction"].first < ranges["ringdown_fraction"].second);
        std::cout << "  Parameter ranges are valid." << std::endl;
    }

    std::cout << "All GWEchoHunterTest checks passed." << std::endl;
    return 0;
}
