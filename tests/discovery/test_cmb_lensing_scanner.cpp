// Phase 6.6: CMB Lensing Scanner Test
// Validates CMBLensingScanner against synthetic ΛCDM power spectra,
// anomaly injection, and robustness with degenerate inputs.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/CMBLensingScanner.h"
#include "discovery/DiscoveryPanelManager.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kEps = 1e-9;

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

std::vector<Event4D> makeLambdaCDMTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        double ell = 10.0 + i * 100.0;              // multipole
        double predictedCl = 1000.0 * 200.0 * 201.0 / (ell * (ell + 1.0));
        double noise = 0.01 * predictedCl * (0.5 - std::sin(i)); // small noise
        traj.emplace_back(ell, predictedCl + noise, 0.0, 0.0);
    }
    return traj;
}

std::vector<Event4D> makeAnomalousTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        double ell = 10.0 + i * 100.0;
        double predictedCl = 1000.0 * 200.0 * 201.0 / (ell * (ell + 1.0));
        double anomalous = predictedCl * 10.0;     // 900% excess power -> strong anomaly
        traj.emplace_back(ell, anomalous, 0.0, 0.0);
    }
    return traj;
}
}

int main() {
    std::cout << "=== CMBLensingScannerTest ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Clean ΛCDM trajectory: no anomaly expected -------------------------
    {
        CMBLensingScanner scanner;
        auto traj = makeLambdaCDMTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "Clean ΛCDM trajectory should not trigger an anomaly");
        std::cout << "  Clean ΛCDM trajectory: no anomaly (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Anomalous trajectory: excess power should be flagged -----------------
    {
        CMBLensingScanner scanner;
        scanner.setParameter("cl_threshold_sigma", 4.0); // Lower threshold for test
        auto traj = makeAnomalousTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(!findings.empty() && "Anomalous trajectory should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        assert(findings[0].severity >= AlertSeverity::HIGH);
        std::cout << "  Anomalous trajectory: finding confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 3. Empty trajectory: must not crash, returns empty ----------------------
    {
        CMBLensingScanner scanner;
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 4. NaN/Inf observations: must not crash ---------------------------------
    {
        CMBLensingScanner scanner;
        std::vector<Event4D> badTraj;
        for (int i = 0; i < 10; ++i) {
            badTraj.emplace_back(10.0 + i * 100.0, kNaN, 0.0, 0.0);
        }
        auto findings = scanner.analyze(metric, location, badTraj);
        (void)findings;
        std::cout << "  NaN observations handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        for (int i = 0; i < 10; ++i) {
            infTraj.emplace_back(10.0 + i * 100.0, kInf, 0.0, 0.0);
        }
        auto findings2 = scanner.analyze(metric, location, infTraj);
        (void)findings2;
        std::cout << "  Inf observations handled safely." << std::endl;
    }

    // --- 5. Parameter tuning ----------------------------------------------------
    {
        CMBLensingScanner scanner;
        scanner.setParameter("cl_threshold_sigma", 10.0);
        auto traj = makeAnomalousTrajectory();
        auto findings = scanner.analyze(metric, location, traj);
        assert(findings.empty() && "High threshold should suppress the anomaly");
        std::cout << "  High sigma threshold suppressed anomaly correctly." << std::endl;
    }

    std::cout << "All CMBLensingScannerTest checks passed." << std::endl;
    return 0;
}
