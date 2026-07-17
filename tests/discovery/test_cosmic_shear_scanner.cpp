// Phase 6.9: Cosmic Shear Scanner Test
// Validates CosmicShearScanner against synthetic shear correlation functions,
// injected anomalies, and robustness with degenerate inputs.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/CosmicShearScanner.h"
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

std::vector<Event4D> makeLambdaCDMShearTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        double theta = 1.0 + i * 0.5;                            // arcmin
        double theory = 1e-4 / (theta * theta);                  // simple power law
        double observed = theory + 0.01 * theory * (0.5 - std::sin(i)); // small noise
        traj.emplace_back(theta, observed, 0.05 * theory, theory);
    }
    return traj;
}

std::vector<Event4D> makeAnomalousShearTrajectory() {
    auto traj = makeLambdaCDMShearTrajectory();
    for (size_t i = 10; i < traj.size(); ++i) {
        traj[i].x *= 10.0; // 10x excess shear
    }
    return traj;
}
}

int main() {
    std::cout << "=== CosmicShearScannerTest ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Clean ΛCDM trajectory: no anomaly expected ---------------------------
    {
        CosmicShearScanner scanner;
        auto traj = makeLambdaCDMShearTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "Clean ΛCDM shear trajectory should not trigger an anomaly");
        std::cout << "  Clean ΛCDM shear trajectory: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Anomalous excess shear: should be flagged ------------------------------
    {
        CosmicShearScanner scanner;
        auto traj = makeAnomalousShearTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous shear trajectory should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        assert(findings[0].severity >= AlertSeverity::HIGH);
        std::cout << "  Anomalous shear trajectory: finding confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ------------------------
    {
        CosmicShearScanner scanner;
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash -----------------------------------
    {
        CosmicShearScanner scanner;
        std::vector<Event4D> nanTraj;
        for (int i = 0; i < 10; ++i) {
            nanTraj.emplace_back(1.0 + i * 0.5, kNaN, 0.01, 1e-4 / ((1.0 + i * 0.5) * (1.0 + i * 0.5)));
        }
        auto findings = scanner.analyze(metric, location, nanTraj);
        assert(findings.empty());
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        for (int i = 0; i < 10; ++i) {
            infTraj.emplace_back(1.0 + i * 0.5, 1e-4 / ((1.0 + i * 0.5) * (1.0 + i * 0.5)), kInf, 0.01);
        }
        auto findings2 = scanner.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Parameter tuning: raise chi2 threshold to suppress anomaly ------------
    {
        CosmicShearScanner scanner;
        scanner.setParameter("chi2_threshold", 100.0);
        auto traj = makeAnomalousShearTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "High chi2 threshold should suppress the anomaly");
        std::cout << "  High chi2 threshold suppressed anomaly correctly." << std::endl;
    }

    std::cout << "All CosmicShearScannerTest checks passed." << std::endl;
    return 0;
}
