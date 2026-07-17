// Phase 6.7: Pulsar Timing Array Scanner Test
// Validates PTAScanner against synthetic Hellings–Downs correlations,
// injected GW anomalies, and robustness with degenerate inputs.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/PTAScanner.h"
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

std::vector<Event4D> makeHellingsDownsTrajectory(bool injectAnomaly = false) {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        double angle = 0.01 + i * 0.15; // avoid exact 0/pi
        double hd = 0.5 * (1.0 - std::cos(angle)) / 2.0 *
                    (3.0 * std::log(0.5 * (1.0 - std::cos(angle))) + 1.0);
        double observed = hd + 0.02 * hd * (0.5 - std::sin(i));
        if (injectAnomaly && i == 10) {
            observed += 5.0; // 5-sigma outlier
        }
        traj.emplace_back(static_cast<double>(i), observed, 0.1, hd);
    }
    return traj;
}
}

int main() {
    std::cout << "=== PTAScannerTest ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Clean GR trajectory: no anomaly expected ---------------------------
    {
        PTAScanner scanner;
        auto traj = makeHellingsDownsTrajectory(false);
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "Clean GR trajectory should not trigger an anomaly");
        std::cout << "  Clean GR trajectory: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Injected GW anomaly: should be flagged ------------------------------
    {
        PTAScanner scanner;
        auto traj = makeHellingsDownsTrajectory(true);
        auto findings = scanner.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous trajectory should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        assert(findings[0].severity >= AlertSeverity::HIGH);
        std::cout << "  Anomalous trajectory: finding confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ---------------------
    {
        PTAScanner scanner;
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash ---------------------------------
    {
        PTAScanner scanner;
        std::vector<Event4D> badTraj;
        for (int i = 0; i < 10; ++i) {
            badTraj.emplace_back(static_cast<double>(i), kNaN, 0.1, 0.5);
        }
        auto findings = scanner.analyze(metric, location, badTraj);
        assert(findings.empty());
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        for (int i = 0; i < 10; ++i) {
            infTraj.emplace_back(static_cast<double>(i), 0.5, 0.1, kInf);
        }
        auto findings2 = scanner.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Parameter tuning ----------------------------------------------------
    {
        PTAScanner scanner;
        scanner.setParameter("sigma_threshold", 10.0);
        auto traj = makeHellingsDownsTrajectory(true);
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "High threshold should suppress the anomaly");
        std::cout << "  High sigma threshold suppressed anomaly correctly." << std::endl;
    }

    std::cout << "All PTAScannerTest checks passed." << std::endl;
    return 0;
}
