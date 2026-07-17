// QuantumVerse Test: GWMemoryDetector
// TDD validation of GW memory effect detection on synthetic strain trajectories.
// Verifies clean memory, injected anomalies, empty input, NaN/Inf robustness,
// and suppression of tiny deviations below the 3σ threshold.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/GWMemoryDetector.h"
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

std::vector<Event4D> makeCleanMemoryTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double theory = 1e-22 * (1.0 - std::exp(-t));
        data.emplace_back(t, theory, 1e-23, theory);
    }
    return data;
}

std::vector<Event4D> makeAnomalousMemoryTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double theory = 1e-22 * (1.0 - std::exp(-t));
        double observed = (i == 5) ? theory + 5e-22 : theory;
        data.emplace_back(t, observed, 1e-23, theory);
    }
    return data;
}

std::vector<Event4D> makeTinyDeviationTrajectory() {
    std::vector<Event4D> data;
    data.reserve(10);
    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1;
        double theory = 1e-22 * (1.0 - std::exp(-t));
        data.emplace_back(t, theory * 1.001, 1e-22, theory);
    }
    return data;
}
}

int main() {
    std::cout << "=== GWMemoryDetector Test ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // --- 1. Clean memory trajectory: no anomaly expected --------------------------
    {
        GWMemoryDetector detector;
        auto traj = makeCleanMemoryTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(findings.empty() && "Clean memory trajectory should not trigger anomaly");
        std::cout << "  Clean memory trajectory: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Injected excess memory: should be flagged ------------------------------
    {
        GWMemoryDetector detector;
        auto traj = makeAnomalousMemoryTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous memory trajectory should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].isAnomaly);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        std::cout << "  Anomalous memory trajectory: confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ------------------------
    {
        GWMemoryDetector detector;
        std::vector<Event4D> empty;
        auto findings = detector.analyze(metric, location, empty);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash -----------------------------------
    {
        GWMemoryDetector detector;
        std::vector<Event4D> nanTraj;
        nanTraj.emplace_back(0.0, kNaN, 1e-23, 1e-22);
        auto findings = detector.analyze(metric, location, nanTraj);
        assert(findings.empty());
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        infTraj.emplace_back(0.0, 1e-22, 1e-23, kInf);
        auto findings2 = detector.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Tiny deviations: should not trigger anomaly ---------------------------
    {
        GWMemoryDetector detector;
        auto traj = makeTinyDeviationTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(findings.empty() && "Tiny deviations should not trigger anomaly");
        std::cout << "  Tiny deviations suppressed correctly." << std::endl;
    }

    std::cout << "All GWMemoryDetectorTest checks passed." << std::endl;
    return 0;
}
