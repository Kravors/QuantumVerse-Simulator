// QuantumVerse Test: FineStructureConstantDriftObservatory
// TDD validation of fine-structure-constant drift detection. Generates a
// synthetic time series with a linear drift α(t) = α₀ + δα·t and verifies the
// detector recovers the drift rate. Convention: t = time, z = measured α.

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/FineStructureConstantDriftObservatory.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

int main() {
    std::cout << "=== FineStructureConstantDriftObservatory Test ===" << std::endl;

    // --- Synthetic drift: α(t) = α₀ + δα·t --------------------------------
    const double alpha0 = 1.0 / 137.035999084;
    const double drift = 1e-6;   // dα/dt
    const double tMax = 10.0;
    const int N = 100;

    std::vector<quantumverse::Event4D> trajectory;
    for (int i = 0; i < N; ++i) {
        double t = tMax * i / (N - 1);
        double alpha = alpha0 + drift * t;
        trajectory.emplace_back(t, 0.0, 0.0, alpha); // t=time, z=measured α
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::FineStructureConstantDriftObservatory obs;
    auto findings = obs.analyze(metric, location, trajectory);

    QV_CHECK(!findings.empty());

    const auto& f = findings.front();
    auto dit = f.parameters.find("drift_rate");
    QV_CHECK(dit != f.parameters.end());
    QV_CHECK_NEAR(dit->second, drift, 1e-9);

    std::cout << "Detected α drift: dα/dt=" << dit->second
              << " (expected " << drift << ")" << std::endl;

    // --- Negative case: constant α (no drift) -> no finding ---------------
    std::vector<quantumverse::Event4D> steady;
    for (int i = 0; i < N; ++i) {
        double t = tMax * i / (N - 1);
        steady.emplace_back(t, 0.0, 0.0, alpha0);
    }
    auto none = obs.analyze(metric, location, steady);
    QV_CHECK(none.empty());

    // --- Edge case: too few points ----------------------------------------
    std::vector<quantumverse::Event4D> tiny = {
        quantumverse::Event4D(0.0, 0.0, 0.0, alpha0),
        quantumverse::Event4D(1.0, 0.0, 0.0, alpha0 + drift),
    };
    auto few = obs.analyze(metric, location, tiny);
    QV_CHECK(few.empty());

    std::cout << "All FineStructureConstantDriftObservatory tests passed." << std::endl;
    return 0;
}
