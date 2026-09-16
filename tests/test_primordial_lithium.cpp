// QuantumVerse Test: PrimordialLithiumCrisisSolver
// TDD validation of the primordial lithium-7 crisis detector. Generates a
// synthetic Li-7/H abundance series that drifts below the BBN prediction and
// verifies the detector flags the crisis and reports the deviation.

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/PrimordialLithiumCrisisSolver.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

int main() {
    std::cout << "=== PrimordialLithiumCrisisSolver Test ===" << std::endl;

    const double expected_li = 5.0e-10;
    const double observed_li = 4.5e-10; // 10% lower than BBN

    // --- Synthetic Li-7 abundance drifting below BBN prediction ----------
    std::vector<quantumverse::Event4D> trajectory;
    for (double t = 0.0; t < 10.0; t += 0.1) {
        double li = expected_li - (expected_li - observed_li) * (t / 10.0);
        trajectory.emplace_back(t, li, 0.0, 0.0); // x = Li/H, t = time
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::PrimordialLithiumCrisisSolver solver;
    auto findings = solver.analyze(metric, location, trajectory);

    QV_CHECK(!findings.empty());

    const auto& f = findings.front();
    bool mentions = f.description.find("lithium") != std::string::npos;
    QV_CHECK(mentions);
    (void)mentions;

    auto mit = f.parameters.find("measured_abundance");
    QV_CHECK(mit != f.parameters.end());
    QV_CHECK_NEAR(mit->second, 4.75e-10, 1e-10);

    std::cout << "Detected lithium-7 crisis: measured " << mit->second
              << " vs expected " << expected_li << std::endl;

    // --- Negative case: constant expected abundance -> no finding --------
    std::vector<quantumverse::Event4D> constant;
    for (double t = 0.0; t < 10.0; t += 0.1) {
        constant.emplace_back(t, expected_li, 0.0, 0.0);
    }
    auto none = solver.analyze(metric, location, constant);
    QV_CHECK(none.empty());

    // --- Edge case: too few points ---------------------------------------
    std::vector<quantumverse::Event4D> tiny(3);
    auto few = solver.analyze(metric, location, tiny);
    QV_CHECK(few.empty());

    std::cout << "All PrimordialLithiumCrisisSolver tests passed." << std::endl;
    return 0;
}
