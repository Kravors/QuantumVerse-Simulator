// Phase 6.8: Fast Radio Burst Dispersion Scanner Test
// Validates FRBDispersionScanner against synthetic DM measurements,
// injected excess-DM anomalies, and robustness with degenerate inputs.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/FRBDispersionScanner.h"
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

std::vector<Event4D> makeCleanDMTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        double expectedDM = 100.0 + 5.0 * i;
        double observedDM = expectedDM + 0.5 * expectedDM * (0.5 - std::sin(i * 0.5));
        traj.emplace_back(static_cast<double>(i), observedDM, 5.0, expectedDM);
    }
    return traj;
}

std::vector<Event4D> makeAnomalousDMTrajectory() {
    auto traj = makeCleanDMTrajectory();
    for (size_t i = 10; i < traj.size(); ++i) {
        traj[i].x += 40.0; // 8-sigma excess
    }
    return traj;
}
}

int main() {
    std::cout << "=== FRBDispersionScannerTest ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Clean DM trajectory: no anomaly expected ---------------------------
    {
        FRBDispersionScanner scanner;
        auto traj = makeCleanDMTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "Clean DM trajectory should not trigger an anomaly");
        std::cout << "  Clean DM trajectory: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Anomalous excess DM: should be flagged ------------------------------
    {
        FRBDispersionScanner scanner;
        auto traj = makeAnomalousDMTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous DM trajectory should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        assert(findings[0].severity >= AlertSeverity::HIGH);
        std::cout << "  Anomalous DM trajectory: finding confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ---------------------
    {
        FRBDispersionScanner scanner;
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash ---------------------------------
    {
        FRBDispersionScanner scanner;
        std::vector<Event4D> nanTraj;
        for (int i = 0; i < 10; ++i) {
            nanTraj.emplace_back(static_cast<double>(i), kNaN, 5.0, 100.0);
        }
        auto findings = scanner.analyze(metric, location, nanTraj);
        assert(findings.empty());
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        for (int i = 0; i < 10; ++i) {
            infTraj.emplace_back(static_cast<double>(i), 100.0, kInf, 50.0);
        }
        auto findings2 = scanner.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Parameter tuning: raise chi2 threshold to suppress anomaly ----------
    {
        FRBDispersionScanner scanner;
        scanner.setParameter("chi2_threshold", 100.0);
        auto traj = makeAnomalousDMTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "High chi2 threshold should suppress the anomaly");
        std::cout << "  High chi2 threshold suppressed anomaly correctly." << std::endl;
    }

    std::cout << "All FRBDispersionScannerTest checks passed." << std::endl;
    return 0;
}
