// QuantumVerse Test: GWRingdownScanner
// TDD validation of GW ringdown QNM frequency analysis.
// Verifies GR-consistent ringdown, injected anomalies, empty input,
// NaN/Inf robustness, and suppression of tiny deviations below the 3σ threshold.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/GWRingdownScanner.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

std::vector<Event4D> makeGRConsistentTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    double predReal = 0.37367;
    double predImag = -0.0890;
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double obsReal = predReal + 0.01 * (static_cast<double>(i) / 10.0);
        double obsImag = predImag + 0.001 * (static_cast<double>(i) / 10.0);
        data.emplace_back(t, obsReal, obsImag, 0.1);
    }
    return data;
}

std::vector<Event4D> makeAnomalousRingdownTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    double predReal = 0.37367;
    double predImag = -0.0890;
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double obsReal = (i == 5) ? predReal + 0.5 : predReal;
        double obsImag = (i == 5) ? predImag - 0.2 : predImag;
        data.emplace_back(t, obsReal, obsImag, 0.01);
    }
    return data;
}

std::vector<Event4D> makeTinyDeviationTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    double predReal = 0.37367;
    double predImag = -0.0890;
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double obsReal = predReal + 0.001;
        double obsImag = predImag + 0.0001;
        data.emplace_back(t, obsReal, obsImag, 0.1);
    }
    return data;
}
}

int main() {
    std::cout << "=== GWRingdownScanner Test ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // --- 1. GR-consistent ringdown: no anomaly expected ---------------------------
    {
        GWRingdownScanner scanner;
        auto traj = makeGRConsistentTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "GR-consistent ringdown should not trigger anomaly");
        std::cout << "  GR-consistent ringdown: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Anomalous ringdown frequencies: should be flagged ---------------------
    {
        GWRingdownScanner scanner;
        auto traj = makeAnomalousRingdownTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous ringdown should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].isAnomaly);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        std::cout << "  Anomalous ringdown: confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty -----------------------
    {
        GWRingdownScanner scanner;
        std::vector<Event4D> empty;
        auto findings = scanner.analyze(metric, location, empty);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash ----------------------------------
    {
        GWRingdownScanner scanner;
        std::vector<Event4D> nanTraj;
        nanTraj.emplace_back(0.0, kNaN, kNaN, 0.1);
        auto findings = scanner.analyze(metric, location, nanTraj);
        assert(findings.empty());
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        infTraj.emplace_back(0.0, kInf, kInf, 0.1);
        auto findings2 = scanner.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Tiny deviations: should not trigger anomaly ---------------------------
    {
        GWRingdownScanner scanner;
        auto traj = makeTinyDeviationTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "Tiny deviations should not trigger anomaly");
        std::cout << "  Tiny deviations suppressed correctly." << std::endl;
    }

    // --- 6. Parameter ranges are sensible -----------------------------------------
    {
        GWRingdownScanner scanner;
        auto ranges = scanner.getParameterRanges();
        assert(ranges.count("sigma_threshold") > 0);
        assert(ranges.count("spin") > 0);
        assert(ranges.count("mass_solar") > 0);
        assert(ranges["sigma_threshold"].first < ranges["sigma_threshold"].second);
        assert(ranges["spin"].first < ranges["spin"].second);
        assert(ranges["mass_solar"].first < ranges["mass_solar"].second);
        std::cout << "  Parameter ranges are valid." << std::endl;
    }

    std::cout << "All GWRingdownScannerTest checks passed." << std::endl;
    return 0;
}
